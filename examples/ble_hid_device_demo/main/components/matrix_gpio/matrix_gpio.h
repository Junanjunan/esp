#include "esp_hidd_prf_api.h"


/**
* @brief Get the HID keycode from the matrix at the given row and column.
* @param row: Row index of the key in the matrix
* @param col: Column index of the key in the matrix
* @return HID keycode at the given row and column
**/
key_mask_t get_keycode_only_exists(uint8_t row, uint8_t col);


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
void send_report_with_delay(bool is_key_pressed, int row, int col, uint16_t hid_conn_id, uint32_t current_time);


/**
* @brief Scan the matrix keypad for key presses and send HID reports.
* @param hid_conn_id: HID connection ID
* @return None
* @note This function use column pullup mode,
*   So when key is pressed, current flows from column to row.
*   Then the column pin will be pulled down to 0.
**/
void matrix_keypad_scan(uint16_t hid_conn_id);


/*
  Initialize the matrix keypad
  by setting the row pins as outputs
  and the column pins as inputs with pull-up resistors.
*/
void matrix_keypad_init(void);


/**
* @brief Task to simulate key press 
* @param pvParameters: Task parameter
* @return None
* @note  This is task function to be inserted in xTaskCreate
**/
void keyboard_task(void *pvParameters);