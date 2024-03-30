/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"

/* handle of Task */
TaskHandle_t xTaskHandle;

void vTask1(void *pvParameters)
{
    int i = 0;
    for (;;)
    {
        printf("Task1 is running and will be deleted after 5 times \r\n");

        if ( i == 4 ) {
            vTaskDelete(xTaskHandle);
        }

        i++;
    }
}


void app_main(void)
{
    int i = 0;

    printf("Hello world!\n");

    xTaskCreate(vTask1, "Task 1", 2048, NULL, 4, &xTaskHandle);

    while (1) {
        printf("print! in %d seconds...\n", i);
        i++;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}