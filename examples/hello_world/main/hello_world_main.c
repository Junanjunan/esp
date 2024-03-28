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

void vTask1(void *pvParameters)
{
    while (1) { // or for(;;)
        printf("vTask1");
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    int i = 0;

    printf("Hello world!\n");

    xTaskCreate(vTask1, "Task 1", 1024, NULL, 3, NULL);    

    while (1) {
        printf("print! in %d seconds...\n", i++);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
