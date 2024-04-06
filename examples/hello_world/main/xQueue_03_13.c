#include <stdio.h>
#include <string.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#define mainONE_SHOT_TIMER_PERIOD       pdMS_TO_TICKS( 3333UL )
#define mainAUTO_RELOAD_TIMER_PERIOD    pdMS_TO_TICKS( 500UL )

/* The Software timer callback functions */
static void prvOneShotTimerCallback( TimerHandle_t xTimer );
static void prvAutoReloadTimerCallback( TimerHandle_t xTimer );

static void prvOneShotTimerCallback( TimerHandle_t xTimer )
{
    static TickType_t xTimeNow;

    /* Obtain the current tick count */
    xTimeNow = xTaskGetTickCount();

    printf("One-shot timer callback executing %lu\n", xTimeNow);
    // printf("One-shot timer callback executing. Time since the timer was last set to the active state: %lu\n", xTimeNow - xTimerGetPeriod(xTimer));
}

static void prvAutoReloadTimerCallback( TimerHandle_t xTimer )
{
    static TickType_t xTimeNow;

    /* Obtain the current tick count */
    xTimeNow = xTaskGetTickCount();

    printf("Auto-reload timer callback executing %lu\n", xTimeNow);
    // printf("Auto-reload timer callback executing. Time since the timer was last set to the active state: %lu\n", xTimeNow - xTimerGetPeriod(xTimer));
}

void app_main(void)
{
    TimerHandle_t xAutoReloadTimer, xOneShotTimer;
    BaseType_t xTimer1Started, xTimer2Started;

    /* Create the one-shot timer, storing the handle in xOneShotTimer */
    xOneShotTimer = xTimerCreate("OneShot", mainONE_SHOT_TIMER_PERIOD, pdFALSE, 0, prvOneShotTimerCallback);

    /* Create the auto-reload timer, storing the handle in xAutoReloadTimer */
    xAutoReloadTimer = xTimerCreate("AutoReload", mainAUTO_RELOAD_TIMER_PERIOD, pdTRUE, 0, prvAutoReloadTimerCallback);

    /* Check the timers were created. */
    if ( ( xOneShotTimer != NULL ) && ( xAutoReloadTimer != NULL ) )
    {
        /* Start the timers. The timers are created in the dormant state, so must be started before they can be used. */
        xTimer1Started = xTimerStart( xOneShotTimer, 0 );
        xTimer2Started = xTimerStart( xAutoReloadTimer, 0 );

        if ( ( xTimer1Started == pdPASS ) && ( xTimer2Started == pdPASS ) )
        {
            /* The timers were created and started successfully. */
            printf("Timers created and started.\n");
        }
    }

    while (1)
    {
        /* The timers have already been started, and will run until the program is stopped. */
        vTaskDelay(pdMS_TO_TICKS(1000UL)); // or vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}