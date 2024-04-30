#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_gap_bt_api.h"

#include "esp_hidd_prf_api.h"
#include "hid_dev.h"

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

/**
* @brief Get the HID keycode from the matrix at the given row and column.
* @param row: Row index of the key in the matrix
* @param col: Column index of the key in the matrix
* @return HID keycode at the given row and column
**/
key_mask_t get_keycode_only_exists(uint8_t row, uint8_t col) {
    // Check the bounds of row and col to ensure they are within the matrix size
    if (row >= ROW_NUM || col >= COL_NUM) {
        ESP_LOGW(__func__, "Row or column out of bounds: row=%d, col=%d", row, col);
        return HID_KEY_RESERVED;  // Return 0x00 for no event if out of bounds
    }
    return keycodes[row][col];  // Return the HID keycode from the matrix
}

// Function to send a keyboard report
// void send_keyboard_report(key_mask_t keycode, uint16_t hid_conn_id) {
//     esp_hidd_send_keyboard_value(hid_conn_id, 0, &keycode, 1);
// }

/**
* @brief Send HID reports with delay for key press, repeat, and release events.
* @param is_key_pressed: Boolean indicating if the key is currently pressed
* @param row: Row index of the key in the matrix
* @param col: Column index of the key in the matrix
* @param hid_conn_id: HID connection ID
* @param current_time: Current time in milliseconds
* @return None
* @note This function handles key debouncing, repeat delay, and key release events.
**/
void send_report_with_delay(bool is_key_pressed, int row, int col, uint16_t hid_conn_id, uint32_t current_time) {
    if (is_key_pressed)
    {
        key_mask_t keycode = get_keycode_only_exists(row, col);
        if (!last_key_state[row][col])
        {
            if (current_time - last_key_time[row][col] > DEBOUNCE_TIME) {
            last_key_state[row][col] = true;
            last_key_time[row][col] = current_time;
            // send_keyboard_report(keycode, hid_conn_id);  // Initial key press
            esp_hidd_send_keyboard_value(hid_conn_id, 0, &keycode, 1);  // Initial key press
            }
        }
        else
        {
            if (current_time - last_key_time[row][col] > REPEAT_DELAY) {
                last_key_time[row][col] = current_time;
                // send_keyboard_report(keycode, hid_conn_id);  // Repeat key press
                esp_hidd_send_keyboard_value(hid_conn_id, 0, &keycode, 1);  // Repeat key press
            }
        }
    }
    else
    {
        if (last_key_state[row][col]) {
            last_key_state[row][col] = false;
            // send_keyboard_report(HID_KEY_RESERVED, hid_conn_id);  // Key release
            esp_hidd_send_keyboard_value(hid_conn_id, 0, HID_KEY_RESERVED, 1);  // Key release
        }
    }
}

/**
* @brief Scan the matrix keypad for key presses and send HID reports.
* @param hid_conn_id: HID connection ID
* @return None
* @note This function use column pullup mode,
*   So when key is pressed, current flows from column to row.
*   Then the column pin will be pulled down to 0.
**/
void matrix_keypad_scan(uint16_t hid_conn_id) {
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;  // Get the current tick count in ms
    for (int row = 0; row < ROW_NUM; ++row) {
        // Set current row low
        gpio_set_level(row_pins[row], 0);

        for (int col = 0; col < COL_NUM; ++col) {
            bool is_key_pressed = (gpio_get_level(col_pins[col]) == 0);  // Read the current key state (pressed is low)
            send_report_with_delay(is_key_pressed, row, col, hid_conn_id, current_time);
        }

        // Set current row high again
        gpio_set_level(row_pins[row], 1);
    }
}

/*
  Initialize the matrix keypad
  by setting the row pins as outputs
  and the column pins as inputs with pull-up resistors.
*/
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

/**
* @brief Task to simulate key press 
* @param pvParameters: Task parameter
* @return None
* @note  This is task function to be inserted in xTaskCreate
**/
void keyboard_task(void *pvParameters) {
    uint16_t hid_conn_id = (uint16_t)(uintptr_t)pvParameters;
    ESP_LOGI(__func__, "Starting keyboard task");
    matrix_keypad_init();
    for (;;) {
        matrix_keypad_scan(hid_conn_id);
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}