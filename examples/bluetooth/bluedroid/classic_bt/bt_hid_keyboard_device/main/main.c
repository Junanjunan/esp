#include "esp_log.h"
#include "esp_hidd_api.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_bt.h"
#include "esp_err.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_gap_bt_api.h"
#include <string.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "driver/gpio.h"

#define REPORT_PROTOCOL_KEYBOARD_REPORT_SIZE    (8)
#define REPORT_BUFFER_SIZE                      REPORT_PROTOCOL_KEYBOARD_REPORT_SIZE

#define ROW_NUM 2
#define COL_NUM 2

int row_pins[ROW_NUM] = {18, 19};
int col_pins[COL_NUM] = {32, 33};

// Define a 2x2 array corresponding to the matrix layout with HID keycodes
uint8_t keycodes[ROW_NUM][COL_NUM] = {
    {0x04, 0x05},  // HID keycodes for 'a', 'b'
    {0x06, 0x07}   // HID keycodes for 'c', 'd'
};

// Last recorded state of each key
bool last_key_state[ROW_NUM][COL_NUM] = {0};

// Time since the last repeat was sent for each key
uint32_t last_key_time[ROW_NUM][COL_NUM] = {0}; 

// Debounce time and repeat delay
const uint32_t DEBOUNCE_TIME = 50; // 50 ms debounce period
const uint32_t REPEAT_DELAY = 500; // 500 ms repeat delay

typedef struct {
    esp_hidd_app_param_t app_param;
    esp_hidd_qos_param_t both_qos;
    uint8_t protocol_mode;
    SemaphoreHandle_t keyboard_mutex;
    TaskHandle_t keyboard_task_hdl;
    uint8_t buffer[REPORT_BUFFER_SIZE];
} local_param_t;

static local_param_t s_local_param = {0};

// HID report descriptor for a simple keyboard with only the 'a' key.
static const uint8_t hid_keyboard_descriptor[] = {
    0x05, 0x01, // USAGE_PAGE (Generic Desktop)
    0x09, 0x06, // USAGE (Keyboard)
    0xa1, 0x01, // COLLECTION (Application)
    0x05, 0x07, // USAGE_PAGE (Keyboard)
    0x19, 0xe0, // USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7, // USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00, // LOGICAL_MINIMUM (0)
    0x25, 0x01, // LOGICAL_MAXIMUM (1)
    0x75, 0x01, // REPORT_SIZE (1)
    0x95, 0x08, // REPORT_COUNT (8)
    0x81, 0x02, // INPUT (Data,Var,Abs)
    0x95, 0x01, // REPORT_COUNT (1)
    0x75, 0x08, // REPORT_SIZE (8)
    0x81, 0x03, // INPUT (Cnst,Var,Abs)
    0x95, 0x05, // REPORT_COUNT (5)
    0x75, 0x01, // REPORT_SIZE (1)
    0x05, 0x08, // USAGE_PAGE (LEDs)
    0x19, 0x01, // USAGE_MINIMUM (Num Lock)
    0x29, 0x05, // USAGE_MAXIMUM (Kana)
    0x91, 0x02, // OUTPUT (Data,Var,Abs)
    0x95, 0x01, // REPORT_COUNT (1)
    0x75, 0x03, // REPORT_SIZE (3)
    0x91, 0x03, // OUTPUT (Cnst,Var,Abs)
    0x95, 0x06, // REPORT_COUNT (6)
    0x75, 0x08, // REPORT_SIZE (8)
    0x15, 0x00, // LOGICAL_MINIMUM (0)
    0x25, 0x65, // LOGICAL_MAXIMUM (101)
    0x05, 0x07, // USAGE_PAGE (Keyboard)
    0x19, 0x00, // USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65, // USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00, // INPUT (Data,Ary,Abs)
    0xc0        // END_COLLECTION
};

const int hid_keyboard_descriptor_len = sizeof(hid_keyboard_descriptor);

bool check_report_id_type(uint8_t report_id, uint8_t report_type)
{
    bool ret = false;
    xSemaphoreTake(s_local_param.keyboard_mutex, portMAX_DELAY);
    do {
        if (report_type != ESP_HIDD_REPORT_TYPE_INPUT) {
            break;
        }
        if (s_local_param.protocol_mode == ESP_HIDD_BOOT_MODE) {
            if (report_id == ESP_HIDD_BOOT_REPORT_ID_MOUSE) {
                ret = true;
                break;
            }
        } else {
            if (report_id == 0) {
                ret = true;
                break;
            }
        }
    } while (0);

    if (!ret) {
        if (s_local_param.protocol_mode == ESP_HIDD_BOOT_MODE) {
            esp_bt_hid_device_report_error(ESP_HID_PAR_HANDSHAKE_RSP_ERR_INVALID_REP_ID);
        } else {
            esp_bt_hid_device_report_error(ESP_HID_PAR_HANDSHAKE_RSP_ERR_INVALID_REP_ID);
        }
    }
    xSemaphoreGive(s_local_param.keyboard_mutex);
    return ret;
}

void matrix_keypad_init(void) {
    // Initialize row pins
    for (int i = 0; i < ROW_NUM; ++i) {
        gpio_set_direction(row_pins[i], GPIO_MODE_OUTPUT);
    }

    // Initialize column pins with pull-up resistors
    for (int i = 0; i < COL_NUM; ++i) {
        gpio_set_direction(col_pins[i], GPIO_MODE_INPUT);
        gpio_set_pull_mode(col_pins[i], GPIO_PULLUP_ONLY);
    }
}

uint8_t get_hid_keycode_from_matrix(uint8_t row, uint8_t col) {
    // Check the bounds of row and col to ensure they are within the matrix size
    if (row >= 2 || col >= 2) {
        return 0x00;  // Return 0x00 for no event if out of bounds
    }

    return keycodes[row][col];  // Return the HID keycode from the matrix
}

// Function to send a keyboard report
void send_keyboard_report(uint8_t keycode) {
    xSemaphoreTake(s_local_param.keyboard_mutex, portMAX_DELAY);
    memset(s_local_param.buffer, 0, REPORT_BUFFER_SIZE);
    s_local_param.buffer[2] = keycode; // Set keycode in the buffer from the matrix gpio input
    esp_bt_hid_device_send_report(ESP_HIDD_REPORT_TYPE_INTRDATA, 0, REPORT_BUFFER_SIZE, s_local_param.buffer);
    xSemaphoreGive(s_local_param.keyboard_mutex);
}

void send_report_with_delay(bool is_key_pressed, int row, int col, uint32_t current_time) {
    if (is_key_pressed)
    {
        if (!last_key_state[row][col]) 
        {
            if (current_time - last_key_time[row][col] > DEBOUNCE_TIME) {
            last_key_state[row][col] = true;
            last_key_time[row][col] = current_time;
            send_keyboard_report(keycodes[row][col]);  // Initial key press
            }
        }
        else
        {
            if (current_time - last_key_time[row][col] > REPEAT_DELAY) {
                last_key_time[row][col] = current_time;
                send_keyboard_report(keycodes[row][col]);  // Repeat key press
            }
        }
    }
    else
    {
        if (last_key_state[row][col]) {
            last_key_state[row][col] = false;
            send_keyboard_report(0x00);  // Key release
        }
    }
}

void matrix_keypad_scan(void) {
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;  // Get the current tick count in ms
    for (int row = 0; row < ROW_NUM; ++row) {
        // Set current row low
        gpio_set_level(row_pins[row], 0);

        for (int col = 0; col < COL_NUM; ++col) {
            bool is_key_pressed = (gpio_get_level(col_pins[col]) == 0);  // Read the current key state (pressed is low)
            send_report_with_delay(is_key_pressed, row, col, current_time);
        }

        // Set current row high again
        gpio_set_level(row_pins[row], 1);
    }
}

// Task to simulate key press
void keyboard_task(void *pvParameters) {
    const char *TAG = "keyboard_task";
    ESP_LOGI(TAG, "Starting keyboard task");
    for (;;) {
        matrix_keypad_scan();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// Store the remote bluetooth device address in non volatile storage
static void store_remote_bda(const uint8_t* remote_bda) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE("NVS_OPEN", "Error (%s) opening NVS handle in store_remote_bda()", esp_err_to_name(err));
        return;
    }

    err = nvs_set_blob(my_handle, "remote_bda", remote_bda, ESP_BD_ADDR_LEN);
    if (err != ESP_OK) {
        ESP_LOGE("NVS_SET", "Error (%s) storing remote BDA in NVS in nvs_set_blob()", esp_err_to_name(err));
    }

    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        ESP_LOGE("NVS_COMMIT", "Error (%s) committing changes to NVS in nvs_commit()", esp_err_to_name(err));
    }

    nvs_close(my_handle);
}

// Retrieve the remote bluetooth device address from non volatile storage
static esp_err_t get_stored_remote_bda(uint8_t *remote_bda) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE("NVS_OPEN", "Error (%s) opening NVS handle in get_stored_remote_bda()", esp_err_to_name(err));
        return err;
    }

    size_t length = ESP_BD_ADDR_LEN;
    err = nvs_get_blob(my_handle, "remote_bda", remote_bda, &length);
    if (err != ESP_OK) {
        ESP_LOGE("NVS_GET", "Error (%s) reading remote BDA from NVS in nvs_get_blob()", esp_err_to_name(err));
    }

    nvs_close(my_handle);
    return err;
}

// Attempt to reconnect to the last connected host
void reconnect_to_host() {
    uint8_t remote_bda[ESP_BD_ADDR_LEN];
    if (get_stored_remote_bda(remote_bda) == ESP_OK) {
        esp_bt_hid_device_connect(remote_bda);
    } else {
        ESP_LOGE("BT_RECONNECT", "Failed to get stored remote BDA");
    }
}

void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    const char *TAG = "esp_bt_gap_cb";
    switch (event) {
    case ESP_BT_GAP_AUTH_CMPL_EVT: {
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(TAG, "authentication success: %s", param->auth_cmpl.device_name);
            esp_log_buffer_hex(TAG, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
            store_remote_bda(param->auth_cmpl.bda);
        } else {
            ESP_LOGE(TAG, "authentication failed, status:%d", param->auth_cmpl.stat);
        }
        break;
    }
    case ESP_BT_GAP_PIN_REQ_EVT: {
        ESP_LOGI(TAG, "ESP_BT_GAP_PIN_REQ_EVT min_16_digit:%d", param->pin_req.min_16_digit);
        if (param->pin_req.min_16_digit) {
            ESP_LOGI(TAG, "Input pin code: 0000 0000 0000 0000");
            esp_bt_pin_code_t pin_code = {0};
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
        } else {
            ESP_LOGI(TAG, "Input pin code: 1234");
            esp_bt_pin_code_t pin_code;
            pin_code[0] = '1';
            pin_code[1] = '2';
            pin_code[2] = '3';
            pin_code[3] = '4';
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
        }
        break;
    }
    case ESP_BT_GAP_ACL_CONN_CMPL_STAT_EVT: {
        ESP_LOGI(TAG, "ACL connected - ESP_BT_GAP_ACL_CONN_CMPL_STAT_EVT");
        break;
    }
    case ESP_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT: {
        ESP_LOGI(TAG, "ACL disconnected - ESP_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT");
        break;
    }

#if (CONFIG_EXAMPLE_SSP_ENABLED == true)
    case ESP_BT_GAP_CFM_REQ_EVT:
        ESP_LOGI(TAG, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %"PRIu32, param->cfm_req.num_val);
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
        ESP_LOGI(TAG, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%"PRIu32, param->key_notif.passkey);
        break;
    case ESP_BT_GAP_KEY_REQ_EVT:
        ESP_LOGI(TAG, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
        break;
#endif
    case ESP_BT_GAP_MODE_CHG_EVT:
        ESP_LOGI(TAG, "ESP_BT_GAP_MODE_CHG_EVT mode:%d", param->mode_chg.mode);
        break;
    default:
        ESP_LOGI(TAG, "event: %d", event);
        break;
    }
    return;
}

void bt_app_task_start_up(void)
{
    s_local_param.keyboard_mutex = xSemaphoreCreateMutex();
    memset(s_local_param.buffer, 0, REPORT_BUFFER_SIZE);
    xTaskCreate(keyboard_task, "keyboard_task", 2 * 1024, NULL, configMAX_PRIORITIES - 3, &s_local_param.keyboard_task_hdl);
    return;
}

void bt_app_task_shut_down(void)
{
    if (s_local_param.keyboard_task_hdl) {
        vTaskDelete(s_local_param.keyboard_task_hdl);
        s_local_param.keyboard_task_hdl = NULL;
    }

    if (s_local_param.keyboard_mutex) {
        vSemaphoreDelete(s_local_param.keyboard_mutex);
        s_local_param.keyboard_mutex = NULL;
    }
    return;
}

void esp_bt_hidd_cb(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param)
{
    static const char *TAG = "esp_bt_hidd_cb";
    switch (event) {
    case ESP_HIDD_INIT_EVT:
        if (param->init.status == ESP_HIDD_SUCCESS) {
            ESP_LOGI(TAG, "setting hid parameters");
            esp_bt_hid_device_register_app(&s_local_param.app_param, &s_local_param.both_qos, &s_local_param.both_qos);
        } else {
            ESP_LOGE(TAG, "init hidd failed!");
        }
        break;
    case ESP_HIDD_DEINIT_EVT:
        break;
    case ESP_HIDD_REGISTER_APP_EVT:
        if (param->register_app.status == ESP_HIDD_SUCCESS) {
            ESP_LOGI(TAG, "setting hid parameters success!");
            ESP_LOGI(TAG, "setting to connectable, discoverable");
            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
            // if (param->register_app.in_use) {
            //     ESP_LOGI(TAG, "start virtual cable plug!");
            //     esp_bt_hid_device_connect(param->register_app.bd_addr);
            // }
            reconnect_to_host();
        } else {
            ESP_LOGE(TAG, "setting hid parameters failed!");
        }
        break;
    case ESP_HIDD_UNREGISTER_APP_EVT:
        if (param->unregister_app.status == ESP_HIDD_SUCCESS) {
            ESP_LOGI(TAG, "unregister app success!");
        } else {
            ESP_LOGE(TAG, "unregister app failed!");
        }
        break;
    case ESP_HIDD_OPEN_EVT:
        if (param->open.status == ESP_HIDD_SUCCESS) {
            if (param->open.conn_status == ESP_HIDD_CONN_STATE_CONNECTING) {
                ESP_LOGI(TAG, "connecting...");
            } else if (param->open.conn_status == ESP_HIDD_CONN_STATE_CONNECTED) {
                ESP_LOGI(TAG, "connected to %02x:%02x:%02x:%02x:%02x:%02x", param->open.bd_addr[0],
                        param->open.bd_addr[1], param->open.bd_addr[2], param->open.bd_addr[3], param->open.bd_addr[4],
                        param->open.bd_addr[5]);
                bt_app_task_start_up();
                ESP_LOGI(TAG, "making self non-discoverable and non-connectable.");
                esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
            } else {
                ESP_LOGE(TAG, "unknown connection status");
            }
        } else {
            ESP_LOGE(TAG, "open failed!");
        }
        break;
    case ESP_HIDD_CLOSE_EVT:    // normal disconnection
        ESP_LOGI(TAG, "ESP_HIDD_CLOSE_EVT");
        if (param->close.status == ESP_HIDD_SUCCESS) {
            if (param->close.conn_status == ESP_HIDD_CONN_STATE_DISCONNECTING) {
                ESP_LOGI(TAG, "disconnecting...");
            } else if (param->close.conn_status == ESP_HIDD_CONN_STATE_DISCONNECTED) {
                ESP_LOGI(TAG, "disconnected!");
                bt_app_task_shut_down();
                ESP_LOGI(TAG, "making self discoverable and connectable again.");
                esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
            } else {
                ESP_LOGE(TAG, "unknown connection status");
            }
        } else {
            ESP_LOGE(TAG, "close failed!");
        }
        break;
    case ESP_HIDD_SEND_REPORT_EVT:
        if (param->send_report.status == ESP_HIDD_SUCCESS) {
            ESP_LOGI(TAG, "ESP_HIDD_SEND_REPORT_EVT id:0x%02x, type:%d", param->send_report.report_id,
                    param->send_report.report_type);
        } else {
            ESP_LOGE(TAG, "ESP_HIDD_SEND_REPORT_EVT id:0x%02x, type:%d, status:%d, reason:%d",
                    param->send_report.report_id, param->send_report.report_type, param->send_report.status,
                    param->send_report.reason);
        }
        break;
    case ESP_HIDD_REPORT_ERR_EVT:
        ESP_LOGI(TAG, "ESP_HIDD_REPORT_ERR_EVT");
        break;
    case ESP_HIDD_GET_REPORT_EVT:
        ESP_LOGI(TAG, "ESP_HIDD_GET_REPORT_EVT id:0x%02x, type:%d, size:%d", param->get_report.report_id,
                param->get_report.report_type, param->get_report.buffer_size);
        if (check_report_id_type(param->get_report.report_id, param->get_report.report_type)) {
            uint8_t report_id;
            uint16_t report_len;
            if (s_local_param.protocol_mode == ESP_HIDD_REPORT_MODE) {
                report_id = 0;
                report_len = REPORT_PROTOCOL_KEYBOARD_REPORT_SIZE;
            } else {
                // Boot Mode
                report_id = ESP_HIDD_BOOT_REPORT_ID_KEYBOARD;
                report_len = ESP_HIDD_BOOT_REPORT_SIZE_KEYBOARD - 1;
            }
            xSemaphoreTake(s_local_param.keyboard_mutex, portMAX_DELAY);
            esp_bt_hid_device_send_report(param->get_report.report_type, report_id, report_len, s_local_param.buffer);
            xSemaphoreGive(s_local_param.keyboard_mutex);
        } else {
            ESP_LOGE(TAG, "check_report_id failed!");
        }
        break;
    case ESP_HIDD_SET_REPORT_EVT:
        ESP_LOGI(TAG, "ESP_HIDD_SET_REPORT_EVT");
        break;
    case ESP_HIDD_SET_PROTOCOL_EVT:
        ESP_LOGI(TAG, "ESP_HIDD_SET_PROTOCOL_EVT");
        if (param->set_protocol.protocol_mode == ESP_HIDD_BOOT_MODE) {
            ESP_LOGI(TAG, "  - boot protocol");
            xSemaphoreTake(s_local_param.keyboard_mutex, portMAX_DELAY);
            // s_local_param.x_dir = -1;
            xSemaphoreGive(s_local_param.keyboard_mutex);
        } else if (param->set_protocol.protocol_mode == ESP_HIDD_REPORT_MODE) {
            ESP_LOGI(TAG, "  - report protocol");
        }
        xSemaphoreTake(s_local_param.keyboard_mutex, portMAX_DELAY);
        s_local_param.protocol_mode = param->set_protocol.protocol_mode;
        xSemaphoreGive(s_local_param.keyboard_mutex);
        break;
    case ESP_HIDD_INTR_DATA_EVT:
        ESP_LOGI(TAG, "ESP_HIDD_INTR_DATA_EVT");
        break;
    case ESP_HIDD_VC_UNPLUG_EVT:    // 'remove device' event in Windows - when device unpluged -> CLOSE_EVT and UNPLUG_EVT occur in sequence
        ESP_LOGI(TAG, "ESP_HIDD_VC_UNPLUG_EVT");
        if (param->vc_unplug.status == ESP_HIDD_SUCCESS) {
            if (param->close.conn_status == ESP_HIDD_CONN_STATE_DISCONNECTED) {
                ESP_LOGI(TAG, "disconnected!");
                bt_app_task_shut_down();
                ESP_LOGI(TAG, "making self discoverable and connectable again.");
                esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
            } else {
                ESP_LOGE(TAG, "unknown connection status");
            }
        } else {
            ESP_LOGE(TAG, "close failed!");
        }
        break;
    case ESP_HIDD_API_ERR_EVT:
        ESP_LOGI(TAG, "ESP_HIDD_API_ERR_EVT");
        break;
    default:
        break;
    }
}

// Main application entry point and setup (remains mostly unchanged)
void app_main(void) {
    // Initialization code here...
    const char *TAG = "app_main";
    esp_err_t ret;
    matrix_keypad_init();

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        ESP_LOGE(TAG, "initialize controller failed: %s", esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) {
        ESP_LOGE(TAG, "enable controller failed: %s", esp_err_to_name(ret));
        return;
    }

    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
#if (CONFIG_EXAMPLE_SSP_ENABLED == false)
    bluedroid_cfg.ssp_en = false;
#endif
    if ((ret = esp_bluedroid_init_with_cfg(&bluedroid_cfg)) != ESP_OK) {
        ESP_LOGE(TAG, "%s initialize bluedroid failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bluedroid_enable()) != ESP_OK) {
        ESP_LOGE(TAG, "enable bluedroid failed: %s", esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bt_gap_register_callback(esp_bt_gap_cb)) != ESP_OK) {
        ESP_LOGE(TAG, "gap register failed: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "setting device name");
    esp_bt_dev_set_device_name("HID Keyboard Example");

    ESP_LOGI(TAG, "setting cod major, peripheral");
    esp_bt_cod_t cod;
    cod.major = ESP_BT_COD_MAJOR_DEV_PERIPHERAL;
    esp_bt_gap_set_cod(cod, ESP_BT_SET_COD_MAJOR_MINOR);

    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    // Update HID descriptor and tasks for keyboard functionality
    do {
        s_local_param.app_param.name = "Keyboard";
        s_local_param.app_param.description = "Keyboard Example";
        s_local_param.app_param.provider = "ESP32";
        s_local_param.app_param.subclass = ESP_HID_CLASS_KBD;
        s_local_param.app_param.desc_list = hid_keyboard_descriptor;
        s_local_param.app_param.desc_list_len = hid_keyboard_descriptor_len;

        memset(&s_local_param.both_qos, 0, sizeof(esp_hidd_qos_param_t)); // don't set the qos parameters
    } while (0);
    
    // Report Protocol Mode is the default mode, according to Bluetooth HID specification
    s_local_param.protocol_mode = ESP_HIDD_REPORT_MODE;

    ESP_LOGI(TAG, "register hid device callback");
    esp_bt_hid_device_register_callback(esp_bt_hidd_cb);

    ESP_LOGI(TAG, "starting hid device");
    esp_bt_hid_device_init();

#if (CONFIG_EXAMPLE_SSP_ENABLED == true)
    /* Set default parameters for Secure Simple Pairing */
    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE;
    esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));
#endif
    /*
     * Set default parameters for Legacy Pairing
     * Use variable pin, input pin code when pairing
     */
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
    esp_bt_pin_code_t pin_code;
    esp_bt_gap_set_pin(pin_type, 0, pin_code);

    ESP_LOGI(TAG, "exiting");
}
