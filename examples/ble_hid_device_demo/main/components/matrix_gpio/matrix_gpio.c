#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_gap_bt_api.h"

#include "esp_hidd_prf_api.h"
#include "hid_dev.h"
#include "matrix_gpio.h"

#define ROW_NUM 2
#define COL_NUM 2

int row_pins[ROW_NUM] = {15, 16};
int col_pins[COL_NUM] = {17, 18};

// Define a 2x2 array corresponding to the matrix layout with HID keycodes
uint8_t keycodes[ROW_NUM][COL_NUM] = {
    {HID_KEY_A, HID_KEY_B},  // HID keycodes for 'a', 'b'
    {HID_KEY_C, HID_KEY_D}   // HID keycodes for 'c', 'd'
};

// Last recorded state of each key
bool last_key_state[ROW_NUM][COL_NUM] = {0};

// Time since the last repeat was sent for each key
uint32_t last_key_time[ROW_NUM][COL_NUM] = {0};

// Debounce time and repeat delay
const uint32_t DEBOUNCE_TIME = 50; // 50 ms debounce period
const uint32_t REPEAT_DELAY = 500; // 500 ms repeat delay


key_mask_t get_keycode_only_exists(uint8_t row, uint8_t col) {
    // Check the bounds of row and col to ensure they are within the matrix size
    if (row >= ROW_NUM || col >= COL_NUM) {
        ESP_LOGW(__func__, "Row or column out of bounds: row=%d, col=%d", row, col);
        return HID_KEY_RESERVED;  // Return 0x00 for no event if out of bounds
    }
    return keycodes[row][col];
}

void send_report_with_delay(bool is_key_pressed, int row, int col, uint16_t hid_conn_id, uint32_t current_time) {
    if (is_key_pressed)
    {
        key_mask_t keycode = get_keycode_only_exists(row, col);
        if (!last_key_state[row][col])
        {
            if (current_time - last_key_time[row][col] > DEBOUNCE_TIME) {
            last_key_state[row][col] = true;
            last_key_time[row][col] = current_time;
            esp_hidd_send_keyboard_value(hid_conn_id, 0, &keycode, 1);  // Initial key press
            }
        }
        else
        {
            if (current_time - last_key_time[row][col] > REPEAT_DELAY) {
                last_key_time[row][col] = current_time;
                esp_hidd_send_keyboard_value(hid_conn_id, 0, &keycode, 1);  // Repeat key press
            }
        }
    }
    else
    {
        if (last_key_state[row][col]) {
            last_key_state[row][col] = false;
            esp_hidd_send_keyboard_value(hid_conn_id, 0, HID_KEY_RESERVED, 1);  // Key release
        }
    }
}

void matrix_keypad_scan(uint16_t hid_conn_id) {
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;  // Get the current tick count in ms
    for (int row = 0; row < ROW_NUM; ++row) {
        gpio_set_level(row_pins[row], 0);   // Set current row low
        for (int col = 0; col < COL_NUM; ++col) {
            bool is_key_pressed = (gpio_get_level(col_pins[col]) == 0);  // Read the current key state (pressed is low)
            send_report_with_delay(is_key_pressed, row, col, hid_conn_id, current_time);
        }
        gpio_set_level(row_pins[row], 1);   // Set current row high again
    }
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

void keyboard_task(void *pvParameters) {
    uint16_t hid_conn_id = (uint16_t)(uintptr_t)pvParameters;
    ESP_LOGI(__func__, "Starting keyboard task");
    matrix_keypad_init();
    for (;;) {
        matrix_keypad_scan(hid_conn_id);
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}