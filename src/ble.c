#include "esp_log.h"
#include "nvs_flash.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/util/util.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "services/dis/ble_svc_dis.h"
#include "services/bas/ble_svc_bas.h"
#include "ble.h"

extern volatile bool is_json_modified;
extern const char *json_path;
extern SemaphoreHandle_t file_mutex;

struct ble_data {
    size_t length;
    char *data;
};

/* Static variables */
static const char *tag = "BLE";
static uint8_t own_addr_type;
static QueueHandle_t ble_queue;

/* Staic function declaration */
// TODO: Maybe rename these to GATT and place them in a new file?
static int ble_gap_event(struct ble_gap_event *e, void *arg);
static int gatt_svr_chr_access_custom(uint16_t conn_handle, uint16_t attr_handle,
                                      struct ble_gatt_access_ctxt *ctxt, void *arg);
static int gatt_svr_dsc_access_custom(uint16_t conn_handle, uint16_t attr_handle,
                                      struct ble_gatt_access_ctxt *ctxt, void *arg);
static int gatt_svr_svc_bas_access(uint16_t conn_handle, uint16_t attr_handle,
                              struct ble_gatt_access_ctxt *ctxt, void *arg);
static int gatt_svr_svc_dis_access(uint16_t conn_handle, uint16_t attr_handle,
                              struct ble_gatt_access_ctxt *ctxt, void *arg);

void ble_store_config_init(void);

static uint16_t ble_svc_chr_custom_val_handle;

// Custom UUIDs obtained from: https://www.uuidgenerator.net/
// Service UUID128: 6ff79d5c-e899-4531-90d8-5cc8adcf65a2
static const ble_uuid128_t gatt_svr_svc_custom_uuid = 
    BLE_UUID128_INIT(0xa2,0x65,0xcf,0xad,0xc8,0x5c,0xd8,0x90,
                     0x31,0x45,0x99,0xe8,0x5c,0x9d,0xf7,0x6f);
// Characteristic UUID128: 6ff79d5d-e899-4531-90d8-5cc8adcf65a2
static const ble_uuid128_t gatt_svr_chr_custom_uuid = 
    BLE_UUID128_INIT(0xa2,0x65,0xcf,0xad,0xc8,0x5c,0xd8,0x90,
                     0x31,0x45,0x99,0xe8,0x5d,0x9d,0xf7,0x6f);

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    // Device Information Service (supported by ble_svc_dis.c)
    // Battery service (supported by ble_svc_bas.c)
    // Custom service for HA to send data to display
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svr_svc_custom_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &gatt_svr_chr_custom_uuid.u,
                .access_cb = gatt_svr_chr_access_custom,
                // TODO: Encrypted writing does not work
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
                .val_handle = &ble_svc_chr_custom_val_handle,
                .descriptors = (struct ble_gatt_dsc_def[]) {
                    {
                        .uuid = BLE_UUID16_DECLARE(BLE_UUID_DESC_CUSTOM_CHAR_NAME),
                        .access_cb = gatt_svr_dsc_access_custom,
                        .att_flags = BLE_ATT_F_READ,
                    },
                    // No more descriptors in this characteristic
                    {0}
                },
            },
            // No more characteristics in this service
            {0}
        }
    },
    // No more services
    {0}
};

static int gatt_svr_dsc_access_custom(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    static const char *custom_descriptor = "HA Interface";
    if(ble_uuid_cmp(ctxt->dsc->uuid, BLE_UUID16_DECLARE(BLE_UUID_DESC_CUSTOM_CHAR_NAME)) == 0) {
        return os_mbuf_append(ctxt->om, custom_descriptor, strlen(custom_descriptor));
    }
    return BLE_ATT_ERR_UNLIKELY;
}

static int gatt_svr_chr_access_custom(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            const uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
            const struct ble_data msg = {
                .data   = malloc(len+1), // Max 64kB
                .length = len,
            };
            if(!msg.data){
                ESP_LOGE(tag, "Insufficient memory to load JSON");
                return BLE_ATT_ERR_INSUFFICIENT_RES;
            }

            int rc = os_mbuf_copydata(ctxt->om, 0, len, msg.data);
            if(rc == 0) {
                msg.data[len] = '\0';
                ESP_LOGI(tag, "Received data: %s", msg.data);

                // Pass in the message in the queue. This is done by copy so the 
                // dequeuing thread will have a pointer to the malloc'd data.
                // THE CONSUMER THREAD IS RESPONSIBLE FOR FREEING THIS RESOURCE!
                if(!xQueueSend(ble_queue, &msg, 0)) {
                    ESP_LOGE(tag, "Failed to queue message");
                    free(msg.data);
                    return BLE_ATT_ERR_INSUFFICIENT_RES;
                }

                return rc;
            } else {
                free(msg.data);
                ESP_LOGE(tag, "Failed to copy data. Requested len: %d", len);
            }
            return rc;
        default:
            ESP_LOGE(tag, "How did you get here?");
            return BLE_ATT_ERR_UNLIKELY;
    }
}

void ble_msg_prcessing_task(void *param) {
    struct ble_data msg;
    while(true) {
        // Wait for a message from the queue
        if(xQueueReceive(ble_queue, &msg, portMAX_DELAY) && 
           xSemaphoreTake(file_mutex, portMAX_DELAY)) {

            FILE *f = fopen(json_path, "w");
            if(f == NULL) {
                ESP_LOGE(tag, "Failed to open file %s", json_path);
                xSemaphoreGive(file_mutex);
                continue;
            }

            // Write the received data to SPIFFS in a JSON file
            if(fwrite(msg.data, 1, msg.length, f) == msg.length) {
                // Ensure that the write is completed to non-volatile memory
                fflush(f);
                ESP_LOGI(tag, "Data successfully written to %s", json_path);
                is_json_modified = true;
            } else {
                ESP_LOGE(tag, "Failed to write all data to the file");
            }
            fclose(f);
            xSemaphoreGive(file_mutex);
            free(msg.data);
        }
    }
}

/// @brief Enables advertising
static void ble_advertise(void) {
    const char *name = ble_svc_gap_device_name();
    // Set the advertisement data (31 bytes max)
    int rc = ble_gap_adv_set_fields(&(struct ble_hs_adv_fields){
        // Discoverability in forthcoming advertisement and BLE-only (no
        // Bluetooth Basic Rate (BR) or Enhanced Data Rate (EDR))
        .flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP,
        // Ensure that the TX power level is advertised
        .tx_pwr_lvl_is_present = true,
        // Let the BLE stack fill in this adv field.
        .tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO,
        .name = (uint8_t*)name,
        .name_len = strlen(name),
        .name_is_complete = true,
        .uuids16 = (ble_uuid16_t[]) {
            BLE_UUID16_INIT(BLE_SVC_DIS_UUID16),
            BLE_UUID16_INIT(BLE_SVC_BAS_UUID16),
        },
        .num_uuids16 = 2,
        .uuids16_is_complete = true,
    });
    if(rc != 0) {
        ESP_LOGE(tag, "error setting advertisement data; rc=%d\n", rc);
        return;
    }

    // Set the scan response data (doesn't fit inside the advertisement data)
    rc = ble_gap_adv_rsp_set_fields(&(struct ble_hs_adv_fields){
        .uuids128 = &gatt_svr_svc_custom_uuid,
        .num_uuids128 = 1,
        .uuids128_is_complete = true,
    });

    // Begin advertising
    rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER, &(struct ble_gap_adv_params){
        // Accept connections from any device (undirected)
        .conn_mode = BLE_GAP_CONN_MODE_UND,
        // Use general discovery mode.
        .disc_mode = BLE_GAP_DISC_MODE_GEN,
    }, ble_gap_event, NULL);
    if(rc != 0) {
        ESP_LOGE(tag, "error enabling advertisement; rc=%d\n", rc);
    }
}

/// @brief The NimBLE host executes this callback when a GAP event occurs. The
/// application associates a GAP event callback with each connection that forms.
/// @param e The type of event being signalled.
/// @param arg Application-specified argument; unused by ble.
/// @return 0 if the application successfully handled the event; nonzero on 
/// failure. The semantics of the return code is specific to the particular GAP
/// event being signalled.
static int ble_gap_event(struct ble_gap_event *e, void *arg) {
    switch(e->type) {
        // New connection established, or connection attempt failed
        case BLE_GAP_EVENT_CONNECT:
            ESP_LOGI(tag, "connection %s; status=%d",
                     e->connect.status == 0 ? "OK" : "failed",
                     e->connect.status);
            if(e->connect.status == 0) {
                // TODO: Print connection descriptor
                struct ble_gap_conn_desc descriptor; (void)descriptor;
                int rc = ble_gap_conn_find(e->connect.conn_handle, &descriptor);
                //ble_gap_adv_stop();
                //ble_print_conn_desc(&descriptor);
            // If the connection failed, restart the advertisement
            } else {
                ble_advertise();
            }
            return 0;
        
        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(tag, "disconnect; reason=%d", e->disconnect.reason);
            ble_advertise();
            return 0;
        
        default:
            // TODO: Can we stringify the event?
            ESP_LOGI(tag, "Unhandled event: type=%d", e->type);
            return 0;
    }
}

// Used for debugging to track what BLE services/characteristics are reg'd
static void gatt_svr_register(struct ble_gatt_register_ctxt *ctxt, void *arg) {
    char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op) {
        case BLE_GATT_REGISTER_OP_SVC:
            ESP_LOGI(tag, "registered service %s with handle=%d\n",
                     ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                     ctxt->svc.handle);
            break;

        case BLE_GATT_REGISTER_OP_CHR:
            ESP_LOGI(tag, "registering characteristic %s with "
                        "def_handle=%d val_handle=%d\n",
                        ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                        ctxt->chr.def_handle,
                        ctxt->chr.val_handle);
            break;

        case BLE_GATT_REGISTER_OP_DSC:
            ESP_LOGI(tag, "registering descriptor %s with handle=%d\n",
                        ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                        ctxt->dsc.handle);
            break;

        default:
            assert(0);
            break;
    }
}

// Executed on stack sync
static void ble_on_sync(void) {
    // TODO: Should I use random address?
    // Set a repeatable identity public address (not a random address)
    int rc = ble_hs_util_ensure_addr(false);
    assert(rc == 0);

    // Figure out address to use while advertising (no privacy for now)
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if(rc != 0) {
        ESP_LOGE(tag, "error determining address type; rc=%d", rc);
        return;
    }

    uint8_t addr_val[6] = {0};
    rc = ble_hs_id_copy_addr(own_addr_type, addr_val, NULL);
    ESP_LOGI(tag, "Device address: %02x:%02x:%02x:%02x:%02x:%02x", 
             addr_val[5], addr_val[4], addr_val[3], addr_val[2], addr_val[1], addr_val[0]);

    // Start advertising
    ble_advertise();
}

void ble_host_task(void *param) {
    ESP_LOGI(tag, "BLE host task started");
    // This function only returns when nimble_port_stop() is executed.
    nimble_port_run();
    // ... if NimBLE is stopped, we need to deinit in FreeRTOS!
    nimble_port_freertos_deinit();
}

void ble_init(void) {
    ble_queue = xQueueCreate(8, sizeof(struct ble_data));
    xTaskCreate(ble_msg_prcessing_task, "ble data handler task", 4096, NULL, 5, NULL);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(tag, "Failed to init NimBLE %d", ret);
        return;
    }

    ble_hs_cfg.reset_cb = NULL;
    ble_hs_cfg.sync_cb = ble_on_sync;
    ble_hs_cfg.gatts_register_cb = gatt_svr_register;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
    // Security Manager (SM) configuration is set in the menuconfig

    // In Turkic mythology, Tengri is the sky god who watches over the world 
    int rc = ble_svc_gap_device_name_set("Tengri");
    if(rc != 0) {
        ESP_LOGE(tag, "Error setting device name; rc=%d", rc);
    }

    // Generic display
    rc = ble_svc_gap_device_appearance_set(0x0140);
    if(rc != 0) {
        ESP_LOGE(tag, "Error setting appearance; rc=%d", rc);
    }

    ble_svc_dis_init();
    rc = ble_svc_dis_model_number_set("HA Display");
    if(rc != 0) {
        ESP_LOGE(tag, "Error setting model number; rc=%d", rc);
    }

    ble_svc_dis_manufacturer_name_set("David Anderle");
    if(rc != 0) {
        ESP_LOGE(tag, "Error setting manufacturer name; rc=%d", rc);
    }

    // TODO: Add version number to compilation flags
    ble_svc_dis_software_revision_set("1.0.0");
    if(rc != 0) {
        ESP_LOGE(tag, "Error setting software revision; rc=%d", rc);
    }

    ble_svc_bas_init();
    // TODO: This should be adjusted periodically by the code
    rc = ble_svc_bas_battery_level_set(100);
    if(rc != 0) {
        ESP_LOGE(tag, "Error setting battery level; rc=%d", rc);
    }

    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    assert(rc == 0);
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    assert(rc == 0);

    // XXX Need to have template for store
    ble_store_config_init();
    // Link the task to FreeRTOS
    nimble_port_freertos_init(ble_host_task);
}