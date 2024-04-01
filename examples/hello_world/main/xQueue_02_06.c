#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static void vSenderTask( void *pvParameters );

/* handle of Task */
QueueHandle_t xQueue;

void app_main(void)
{
    /* Create Queue */
    xQueue = xQueueCreate(5, sizeof(int32_t));

    if (xQueue != NULL)
    {
        /* Create the thread(s) */
        xTaskCreate(vSenderTask, "Sender1", 2048, (void *)100, 1, NULL);
    }
    else
    {
        printf("Create Queue failed\r\n");
    }

    int32_t lReceivedValue;
    BaseType_t xStatus;
    const TickType_t xTicksToWait = pdMS_TO_TICKS(500UL);

    for (;;)
    {
        if (uxQueueMessagesWaiting(xQueue) != 0)
        {
            printf("Queue should have been empty!\r\n");
        }

        xStatus = xQueueReceive(xQueue, &lReceivedValue, xTicksToWait);

        if (xStatus == pdPASS)
        {
            printf("Received = %ld\r\n", lReceivedValue);
        }
        else
        {
            printf("Could not receive from the queue\r\n");
        }
    }
}

static void vSenderTask( void *pvParameters )
{
    int32_t lValueToSend;
    BaseType_t xStatus;

    lValueToSend = (int32_t)pvParameters;

    for (;;)
    {
        xStatus = xQueueSendToBack(xQueue, &lValueToSend, 0); // xQueueSendToBack is same with xQueueSend
        if (xStatus != pdPASS)
        {
            printf("Failed to post the message, after 10 ticks.\r\n");
        }
        else
        {
            printf("Message posted: %ld\r\n", lValueToSend);
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay 1 second == vTaskDelay(1000/portTICK_PERIOD_MS) == vTaskDelay(1000/portTickRateMS) ?copilot == vTaskDelay(1000) ?copilot
    }
}