#ifndef H_BLE
#define H_BLE

// TODO: Surely this is in the ESP-IDF libraries somewhere...
// Standard characteristic user description descriptor UUID
#define BLE_UUID_DESC_CUSTOM_CHAR_NAME (0x2901) 

void ble_init(void);

#endif
