#include <stdio.h>
#include <string.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#define mainONE_SHOT_TIMER_PERIOD       pdMS_TO_TICKS( 5000UL )
#define mainAUTO_RELOAD_TIMER_PERIOD    pdMS_TO_TICKS( 1000UL )


TimerHandle_t xAutoReloadTimer, xOneShotTimer;

/* The Software timer callback functions */
static void prvTimerCallback( TimerHandle_t xTimer )
{
    static TickType_t xTimeNow;
    uint32_t ulExecutionCount = 0;

    /* Ontain the ID */
    ulExecutionCount = ( uint32_t ) pvTimerGetTimerID( xTimer );
    ulExecutionCount++;
    vTimerSetTimerID( xTimer, ( void * ) ulExecutionCount );

    /* Obtain the current tick count */
    xTimeNow = xTaskGetTickCount();

    if (xTimer == xOneShotTimer)
    {
        printf("One-shot timer callback executing %lu\n", xTimeNow);
    }
    else if (xTimer == xAutoReloadTimer)
    {
        printf("Auto-reload timer callback executing %lu\n", xTimeNow);

        if (ulExecutionCount == 10)
        {
            xTimerStop(xTimer, 0);
        }
    }
}


void app_main(void)
{
    
    BaseType_t xTimer1Started, xTimer2Started;

    /* Create the one-shot timer, storing the handle in xOneShotTimer */
    xOneShotTimer = xTimerCreate("OneShot", mainONE_SHOT_TIMER_PERIOD, pdFALSE, 0, prvTimerCallback);

    /* Create the auto-reload timer, storing the handle in xAutoReloadTimer */
    xAutoReloadTimer = xTimerCreate("AutoReload", mainAUTO_RELOAD_TIMER_PERIOD, pdTRUE, 0, prvTimerCallback);

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
}