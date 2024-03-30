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
TaskHandle_t xTask2Handle;

void vTask1(void *pvParameters)
{
    UBaseType_t uxPriority;
    uxPriority = uxTaskPriorityGet( NULL );
    
    for(;;)
    {
        printf("Task1 is running \r\n");
        vTaskDelay(2000 / portTICK_PERIOD_MS);

        /* Setting the Task2 priority */
        printf("About to raise the Task2 priority\r\n");
        vTaskPrioritySet( xTask2Handle, ( uxPriority + 2) );

        // Getting the current task's name
        const char *pcTaskName = pcTaskGetName(NULL); // NULL means the current task

        // Printing the current task's name and priority
        printf("Current Task Name 01: %s, Priority: %u\n", pcTaskName, uxPriority);
    }
}

void vTask2(void *pvParameters)
{
    UBaseType_t uxPriority;
    uxPriority = uxTaskPriorityGet( NULL );
    
    for(;;)
    {
        printf("Task2 is running \r\n");
        vTaskDelay(2000 / portTICK_PERIOD_MS);

        /* Setting the Task2 priority */
        printf("About to decrease the Task2 priority\r\n");
        vTaskPrioritySet( NULL, ( uxPriority - 1) );

        // Getting the current task's name
        const char *pcTaskName = pcTaskGetName(NULL); // NULL means the current task

        // Printing the current task's name and priority
        printf("Current Task Name 02: %s, Priority: %u\n", pcTaskName, uxPriority);
    }
}

void app_main(void)
{
    int i = 0;

    printf("Hello world!\n");

    xTaskCreate(vTask1, "Task 1", 2048, NULL, 4, NULL);
    xTaskCreate(vTask2, "Task 2", 2048, NULL, 3, &xTask2Handle);

    while (1) {
        printf("print! in %d seconds...\n", i);
        i++;
        vTaskDelay(4000 / portTICK_PERIOD_MS);
    }
}