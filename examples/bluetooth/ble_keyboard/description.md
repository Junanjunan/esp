
## Send report function of BLE

### function 
```c
esp_err_t esp_ble_gatts_send_indicate(
    esp_gatt_if_t gatts_if,
    uint16_t conn_id,
    uint16_t attr_handle,
    uint16_t value_len,
    uint8_t *value,
    bool need_confirm
);
```
<hr>

### Record

gatts_if

  - 4, 5, 6

  - ChatGPT said 6 is for HID to my case

conn_id

  - from param->connect.conn_id
  
  - 0

<hr>

### TO do

sending report example

```c
uint8_t hid_report[] = {0x0, 0x0, 0x04};  // Example HID report with 'a' key pressed
esp_ble_gatts_send_indicate(6, conn_id, hid_report_handle, sizeof(hid_report), hid_report, false);
```

<hr>