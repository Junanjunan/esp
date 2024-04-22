/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"

#include "esp_hidd.h"
#include "esp_hid_gap.h"

#include "driver/gpio.h"

#define REPORT_PROTOCOL_KEYBOARD_REPORT_SIZE    (8)
#define REPORT_BUFFER_SIZE                      REPORT_PROTOCOL_KEYBOARD_REPORT_SIZE

#define ROW_NUM 2
#define COL_NUM 2

static const char *TAG = "HID_DEV_DEMO";

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

typedef struct
{
    SemaphoreHandle_t keyboard_mutex;
    TaskHandle_t task_hdl;
    esp_hidd_dev_t *hid_dev;
    uint8_t protocol_mode;
    //uint8_t *buffer;
    uint8_t buffer[REPORT_BUFFER_SIZE];
} local_param_t;

static local_param_t s_ble_hid_param = {0};

#define CASE(a, b, c)  \
                case a: \
				buffer[0] = b;  \
				buffer[2] = c; \
                break;\

// USB keyboard codes
#define USB_HID_MODIFIER_LEFT_CTRL      0x01
#define USB_HID_MODIFIER_LEFT_SHIFT     0x02
#define USB_HID_MODIFIER_LEFT_ALT       0x04
#define USB_HID_MODIFIER_RIGHT_CTRL     0x10
#define USB_HID_MODIFIER_RIGHT_SHIFT    0x20
#define USB_HID_MODIFIER_RIGHT_ALT      0x40

#define USB_HID_SPACE                   0x2C
#define USB_HID_DOT                     0x37
#define USB_HID_NEWLINE                 0x28
#define USB_HID_FSLASH                  0x38
#define USB_HID_BSLASH                  0x31
#define USB_HID_COMMA                   0x36
#define USB_HID_DOT                     0x37

const unsigned char keyboardReportMap[] = { //7 bytes input (modifiers, resrvd, keys*5), 1 byte output
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0xE0,        //   Usage Minimum (0xE0)
    0x29, 0xE7,        //   Usage Maximum (0xE7)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x05,        //   Report Count (5)
    0x75, 0x01,        //   Report Size (1)
    0x05, 0x08,        //   Usage Page (LEDs)
    0x19, 0x01,        //   Usage Minimum (Num Lock)
    0x29, 0x05,        //   Usage Maximum (Kana)
    0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x03,        //   Report Size (3)
    0x91, 0x03,        //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x05,        //   Report Count (5)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x65,        //   Logical Maximum (101)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0x00,        //   Usage Minimum (0x00)
    0x29, 0x65,        //   Usage Maximum (0x65)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection

    // 65 bytes
};

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
    xSemaphoreTake(s_ble_hid_param.keyboard_mutex, portMAX_DELAY);
    ESP_LOGI(__func__, "Sending keyboard report!!!!!!: %d", keycode);
    memset(s_ble_hid_param.buffer, 0, REPORT_BUFFER_SIZE);
    s_ble_hid_param.buffer[2] = keycode; // Set keycode in the buffer from the matrix gpio input
    //esp_ble_gatts_send_indicate();
    xSemaphoreGive(s_ble_hid_param.keyboard_mutex);
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
            if (!is_key_pressed) {
                ESP_LOGI(__func__, "Row: %d, Col: %d, Key pressed: %d", row, col, is_key_pressed);
            }
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

static void char_to_code(uint8_t *buffer, char ch)
{
	// Check if lower or upper case
	if(ch >= 'a' && ch <= 'z')
	{
		buffer[0] = 0;
		// convert ch to HID letter, starting at a = 4
		buffer[2] = (uint8_t)(4 + (ch - 'a'));
	}
	else if(ch >= 'A' && ch <= 'Z')
	{
		// Add left shift
		buffer[0] = USB_HID_MODIFIER_LEFT_SHIFT;
		// convert ch to lower case
		ch = ch - ('A'-'a');
		// convert ch to HID letter, starting at a = 4
		buffer[2] = (uint8_t)(4 + (ch - 'a'));
	}
	else if(ch >= '0' && ch <= '9') // Check if number
	{
		buffer[0] = 0;
		// convert ch to HID number, starting at 1 = 30, 0 = 39
		if(ch == '0')
		{
			buffer[2] = 39;
		}
		else
		{
			buffer[2] = (uint8_t)(30 + (ch - '1'));
		}
	}
	else // not a letter nor a number
	{
		switch(ch)
		{
            CASE(' ', 0, USB_HID_SPACE);
			CASE('.', 0, USB_HID_DOT);
            CASE('\n', 0, USB_HID_NEWLINE);
			CASE('?', USB_HID_MODIFIER_LEFT_SHIFT, USB_HID_FSLASH);
			CASE('/', 0 , USB_HID_FSLASH);
			CASE('\\', 0, USB_HID_BSLASH);
			CASE('|', USB_HID_MODIFIER_LEFT_SHIFT, USB_HID_BSLASH);
			CASE(',', 0, USB_HID_COMMA);
			CASE('<', USB_HID_MODIFIER_LEFT_SHIFT, USB_HID_COMMA);
			CASE('>', USB_HID_MODIFIER_LEFT_SHIFT, USB_HID_COMMA);
			CASE('@', USB_HID_MODIFIER_LEFT_SHIFT, 31);
			CASE('!', USB_HID_MODIFIER_LEFT_SHIFT, 30);
			CASE('#', USB_HID_MODIFIER_LEFT_SHIFT, 32);
			CASE('$', USB_HID_MODIFIER_LEFT_SHIFT, 33);
			CASE('%', USB_HID_MODIFIER_LEFT_SHIFT, 34);
			CASE('^', USB_HID_MODIFIER_LEFT_SHIFT, 35);
			CASE('&', USB_HID_MODIFIER_LEFT_SHIFT, 36);
			CASE('*', USB_HID_MODIFIER_LEFT_SHIFT, 37);
			CASE('(', USB_HID_MODIFIER_LEFT_SHIFT, 38);
			CASE(')', USB_HID_MODIFIER_LEFT_SHIFT, 39);
			CASE('-', 0, 0x2D);
			CASE('_', USB_HID_MODIFIER_LEFT_SHIFT, 0x2D);
			CASE('=', 0, 0x2E);
			CASE('+', USB_HID_MODIFIER_LEFT_SHIFT, 39);
			CASE(8, 0, 0x2A); // backspace
			CASE('\t', 0, 0x2B);
			default:
				buffer[0] = 0;
				buffer[2] = 0;
		}
	}
}

void send_keyboard(char c)
{
    ESP_LOGI("sned_keyboard", "Sending key: %c", c);
    static uint8_t buffer[8] = {0};
    char_to_code(buffer, c);
    esp_hidd_dev_input_set(s_ble_hid_param.hid_dev, 0, 1, buffer, 8);
    /* send the keyrelease event with sufficient delay */
    vTaskDelay(50 / portTICK_PERIOD_MS);
    memset(buffer, 0, sizeof(uint8_t) * 8);
    esp_hidd_dev_input_set(s_ble_hid_param.hid_dev, 0, 1, buffer, 8);
}

void ble_hid_demo_task_kbd(void *pvParameters)
{
    static const char* help_string = "########################################################################\n"\
                                      "BT hid keyboard demo usage:\n"\
                                      "########################################################################\n";
                                    /* TODO : Add support for function keys and ctrl, alt, esc, etc. */
    printf("%s\n", help_string);
    char c;
    while (1) {
        c = 6;

        if(c != 255) {
            send_keyboard(c);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

static esp_hid_raw_report_map_t ble_report_maps[] = {
    {
        .data = keyboardReportMap,
        .len = sizeof(keyboardReportMap)
    }
};

static esp_hid_device_config_t ble_hid_config = {
    .vendor_id          = 0x16C0,
    .product_id         = 0x05DF,
    .version            = 0x0100,
    .device_name        = "ESP Keyboard",
    .manufacturer_name  = "Espressif",
    .serial_number      = "1234567890",
    .report_maps        = ble_report_maps,
    .report_maps_len    = 1
};

void ble_hid_task_start_up(void)
{
    if (s_ble_hid_param.task_hdl) {
        // Task already exists
        return;
    }
    /* Nimble Specific */
    s_ble_hid_param.keyboard_mutex = xSemaphoreCreateMutex();
    xTaskCreate(keyboard_task, "keyboard_task", 4 * 1024, NULL, configMAX_PRIORITIES - 3,
                &s_ble_hid_param.task_hdl);
}

void ble_hid_task_shut_down(void)
{
    if (s_ble_hid_param.task_hdl) {
        vTaskDelete(s_ble_hid_param.task_hdl);
        s_ble_hid_param.task_hdl = NULL;
    }

    if (s_ble_hid_param.keyboard_mutex) {
        vSemaphoreDelete(s_ble_hid_param.keyboard_mutex);
        s_ble_hid_param.keyboard_mutex = NULL;
    }
    return;
}

static void ble_hidd_event_callback(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    esp_hidd_event_t event = (esp_hidd_event_t)id;
    esp_hidd_event_data_t *param = (esp_hidd_event_data_t *)event_data;
    static const char *TAG = "HID_DEV_BLE";

    switch (event) {
    case ESP_HIDD_ANY_EVENT: {
        ESP_LOGI(TAG, "ESP_HIDD_ANY_EVENT");
        break;
    }
    case ESP_HIDD_START_EVENT: {
        ESP_LOGI(TAG, "START");
        esp_hid_ble_gap_adv_start();
        break;
    }
    case ESP_HIDD_CONNECT_EVENT: {
        ESP_LOGI(TAG, "CONNECT");
        break;
    }
    case ESP_HIDD_PROTOCOL_MODE_EVENT: {
        ESP_LOGI(TAG, "ESP_HIDD_PROTOCOL_MODE_EVENT");
        ESP_LOGI(TAG, "PROTOCOL MODE[%u]: %s", param->protocol_mode.map_index, param->protocol_mode.protocol_mode ? "REPORT" : "BOOT");
        break;
    }
    case ESP_HIDD_CONTROL_EVENT: {
        ESP_LOGI(TAG, "ESP_HIDD_CONTROL_EVENT");
        ESP_LOGI(TAG, "CONTROL[%u]: %sSUSPEND", param->control.map_index, param->control.control ? "EXIT_" : "");
        if (param->control.control)
        {
            // exit suspend
            ble_hid_task_start_up();
        } else {
            // suspend
            ble_hid_task_shut_down();
        }
    break;
    }
    case ESP_HIDD_OUTPUT_EVENT: {
        ESP_LOGI(TAG, "ESP_HIDD_OUTPUT_EVENT");
        ESP_LOGI(TAG, "OUTPUT[%u]: %8s ID: %2u, Len: %d, Data:", param->output.map_index, esp_hid_usage_str(param->output.usage), param->output.report_id, param->output.length);
        ESP_LOG_BUFFER_HEX(TAG, param->output.data, param->output.length);
        ESP_LOG_BUFFER_HEX(TAG, param->output.data, param->output.length);
        ESP_LOG_BUFFER_HEX(TAG, param->output.data, param->output.length);
        ESP_LOG_BUFFER_HEX(TAG, param->output.data, param->output.length);
        break;
    }
    case ESP_HIDD_FEATURE_EVENT: {
        ESP_LOGI(TAG, "ESP_HIDD_FEATURE_EVENT");
        ESP_LOGI(TAG, "FEATURE[%u]: %8s ID: %2u, Len: %d, Data:", param->feature.map_index, esp_hid_usage_str(param->feature.usage), param->feature.report_id, param->feature.length);
        ESP_LOG_BUFFER_HEX(TAG, param->feature.data, param->feature.length);
        break;
    }
    case ESP_HIDD_DISCONNECT_EVENT: {
        ESP_LOGI(TAG, "ESP_HIDD_DISCONNECT_EVENT");
        ESP_LOGI(TAG, "DISCONNECT: %s", esp_hid_disconnect_reason_str(esp_hidd_dev_transport_get(param->disconnect.dev), param->disconnect.reason));
        ble_hid_task_shut_down();
        esp_hid_ble_gap_adv_start();
        break;
    }
    case ESP_HIDD_STOP_EVENT: {
        ESP_LOGI(TAG, "STOP");
        break;
    }
    case ESP_HIDD_MAX_EVENT: {
        ESP_LOGI(TAG, "ESP_HIDD_MAX_EVENT");
        break;
    }
    default:
        break;
    }
    return;
}

void app_main(void)
{
    esp_err_t ret;
#if HID_DEV_MODE == HIDD_IDLE_MODE
    ESP_LOGE(TAG, "Please turn on BT HID device or BLE!");
    return;
#endif
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_LOGI(TAG, "setting hid gap, mode:%d", HID_DEV_MODE);
    ret = esp_hid_gap_init(HID_DEV_MODE);
    ESP_ERROR_CHECK( ret );

    ret = esp_hid_ble_gap_adv_init(ESP_HID_APPEARANCE_KEYBOARD, ble_hid_config.device_name);
    ESP_ERROR_CHECK( ret );
    if ((ret = esp_ble_gatts_register_callback(esp_hidd_gatts_event_handler)) != ESP_OK) {
        ESP_LOGE(TAG, "GATTS register callback failed: %d", ret);
        return;
    }

    ESP_LOGI(TAG, "setting ble device");
    ESP_ERROR_CHECK(
        esp_hidd_dev_init(&ble_hid_config, ESP_HID_TRANSPORT_BLE, ble_hidd_event_callback, &s_ble_hid_param.hid_dev));
}
