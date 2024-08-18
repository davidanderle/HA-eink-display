#ifndef H_BLE
#define H_BLE

#define MAX_BLE_MSG_SIZE (64*1024) //64kB

// TODO: Surely this is in the ESP-IDF libraries somewhere...
// Standard characteristic user description descriptor UUID
#define BLE_UUID_DESC_CUSTOM_CHAR_NAME (0x2901) 

void ble_init(void);

#endif
