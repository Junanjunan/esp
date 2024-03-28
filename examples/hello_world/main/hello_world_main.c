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

const char *pcTextForTask1 = "vTask1";
const char *pcTextForTask2 = "vTask2";

void vTask1(void *pvParameters)
{
    char *pcTaskName;
    pcTaskName = (char *)pvParameters;

    for(;;)
    {
        printf("%s\n", pcTaskName);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    int i = 0;

    printf("Hello world!\n");

    xTaskCreate(vTask1, "Task 1", 1024, (void*)pcTextForTask1, 3, NULL);
    xTaskCreate(vTask1, "Task 2", 1024, (void*)pcTextForTask2, 3, NULL);

    while (1) {
        printf("print! in %d seconds...\n", i);
        i++;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
