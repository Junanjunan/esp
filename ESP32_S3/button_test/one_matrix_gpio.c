/*
    test code for esp32_s3_button_test.txt
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define ROW_GPIO    5  // Change this according to your setup
#define COLUMN_GPIO 18  // Change this according to your setup

static const char* TAG = "Matrix Keyboard";

void app_main(void) {
    // Configure ROW as output
    gpio_set_direction(ROW_GPIO, GPIO_MODE_OUTPUT);
    // Set ROW high
    gpio_set_level(ROW_GPIO, 1);

    // Configure COLUMN as input
    gpio_set_direction(COLUMN_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(COLUMN_GPIO, GPIO_PULLDOWN_ONLY);

    while (1) {
        // Read the COLUMN state
        int col_state = gpio_get_level(COLUMN_GPIO);
        if (col_state == 1) {
            ESP_LOGI(TAG, "Button Press Detected!");
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Check every 100ms
    }
}
