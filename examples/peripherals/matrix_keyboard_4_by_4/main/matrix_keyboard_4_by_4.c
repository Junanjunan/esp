#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"  // Include for FreeRTOS functions
#include "freertos/task.h"      // Include for task functions like vTaskDelay
#include <stdio.h>

#define ROW_NUM 4
#define COL_NUM 4

int row_pins[ROW_NUM] = {18, 19, 21, 22};
int col_pins[COL_NUM] = {32, 33, 25, 26};

void app_main(void) {
    // Initialize row pins
    for (int i = 0; i < ROW_NUM; ++i) {
        // No need for gpio_pad_select_gpio here; just set the pin direction
        gpio_set_direction(row_pins[i], GPIO_MODE_OUTPUT);
    }

    // Initialize column pins with pull-up resistors
    for (int i = 0; i < COL_NUM; ++i) {
        gpio_set_direction(col_pins[i], GPIO_MODE_INPUT);
        gpio_set_pull_mode(col_pins[i], GPIO_PULLUP_ONLY);
    }

    while (1) {
        for (int row = 0; row < ROW_NUM; ++row) {
            // Set current row low
            gpio_set_level(row_pins[row], 0);

            for (int col = 0; col < COL_NUM; ++col) {
                if (gpio_get_level(col_pins[col]) == 0) {
                    printf("Button pressed at row %d, column %d\n", row, col);
                    // Handle button press
                }
            }

            // Set current row high again
            gpio_set_level(row_pins[row], 1);
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // Delay for debouncing
    }
}
