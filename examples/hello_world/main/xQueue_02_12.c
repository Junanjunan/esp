#include <stdio.h>
#include <string.h>

#include "sdkconfig.h"
#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define GPIO_INPUT_USER_BUTTON  0
#define GPIO_INPUT_BUTTON_SEL   (1ULL << GPIO_INPUT_USER_BUTTON)
#define ESP_INTR_FLAG_DEFAULT   0

/* Declare a variable of type SemapthoreHandle_t */
SemaphoreHandle_t xCountingSemaphore;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    /* 'Give' the semaphore to unblock the task */
    xSemaphoreGiveFromISR(xCountingSemaphore, NULL);
    xSemaphoreGiveFromISR(xCountingSemaphore, NULL);
    xSemaphoreGiveFromISR(xCountingSemaphore, NULL);
}

void app_main(void)
{
    gpio_config_t io_conf;

    // interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_POSEDGE;

    // bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_BUTTON_SEL;

    // set as input mode
    io_conf.mode = GPIO_MODE_INPUT;

    // enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    // install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    // hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_USER_BUTTON, gpio_isr_handler, (void*) GPIO_INPUT_USER_BUTTON);

    /* create counting semaphore */
    xCountingSemaphore = xSemaphoreCreateCounting(10, 2);

    uint32_t count = 0;

    while (1)
    {
        xSemaphoreTake(xCountingSemaphore, portMAX_DELAY);
        count++;
        printf("Handler task - Processing event count: %ld. \r\n", count);
    }
}