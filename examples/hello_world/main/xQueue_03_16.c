#include <stdio.h>
#include <string.h>

#include "sdkconfig.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#define GPIO_INPUT_USER_BUTTON      0
#define GPIO_INPUT_BUTTON_SEL      (1ULL<<GPIO_INPUT_USER_BUTTON)
#define ESP_INTR_FLAG_DEFAULT       0
#define mainBACKLIGHT_TIMER_PERIOD  pdMS_TO_TICKS( 5000UL )

uint8_t count_i;
static BaseType_t xSimulatedBacklightOn = pdFALSE;
static TimerHandle_t xBacklightTimer = NULL;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    count_i = (count_i + 1) % 2;
}

/* The Software timer callback functions */
static void prvBacklightTimerCallback( TimerHandle_t xTimer )
{
    TickType_t xTimeNow = xTaskGetTickCount();

    /* The backlight timer expired, turn the backlight off */
    xSimulatedBacklightOn = pdFALSE;
    /* Print the time at which the backlight was turned off */
    printf( "Backlight turned off at time %lu\n", xTimeNow );
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
    gpio_isr_handler_add(GPIO_INPUT_USER_BUTTON, gpio_isr_handler, (void *) GPIO_INPUT_USER_BUTTON);


    BaseType_t xTimerStarted;

    /* Create the auto-reload timer, storing the handle in xAutoReloadTimer */
    xAutoReloadTimer = xTimerCreate("AutoReload", HEALTY_PERIOD, pdTRUE, 0, prvTimerCallback);

    /* Check the timers were created. */
    if ( xAutoReloadTimer != NULL )
    {
        /* Start the timers. The timers are created in the dormant state, so must be started before they can be used. */
        xTimerStarted = xTimerStart( xAutoReloadTimer, 0 );

        if ( xTimerStarted == pdPASS )
        {
            /* The timers were created and started successfully. */
            printf("Timers created and started.\n");
        }
    }
}