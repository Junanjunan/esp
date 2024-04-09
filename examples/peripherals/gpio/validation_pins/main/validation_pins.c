#include "stdio.h"
#include "driver/gpio.h"
#include "esp_log.h"

void app_main(void)
{
    // Initialize any necessary components here (e.g., UART for printf)
    
    printf("Listing all GPIO pins:\n");

    for (int pin = 0; pin < GPIO_NUM_MAX; pin++) {
        // Skip pins that are not valid on ESP32
        if (!GPIO_IS_VALID_GPIO(pin)){
            printf("GPIO %d is not a valid pin\n", pin);
        }
        else {
            // For simplicity, we're just printing all valid GPIO pins.
            printf("GPIO %d is a valid pin\n", pin);
        }

        
    }
}

/*
W (291) spi_flash: Detected size(4096k) larger than the size in the binary image header(2048k). Using the size in the binary image header.
I (305) main_task: Started on CPU0
I (315) main_task: Calling app_main()
Listing all GPIO pins:
GPIO 0 is a valid pin
GPIO 1 is a valid pin
GPIO 2 is a valid pin
GPIO 3 is a valid pin
GPIO 4 is a valid pin
GPIO 5 is a valid pin
GPIO 6 is a valid pin
GPIO 7 is a valid pin
GPIO 8 is a valid pin
GPIO 9 is a valid pin
GPIO 10 is a valid pin
GPIO 11 is a valid pin
GPIO 12 is a valid pin
GPIO 13 is a valid pin
GPIO 14 is a valid pin
GPIO 15 is a valid pin
GPIO 16 is a valid pin
GPIO 17 is a valid pin
GPIO 18 is a valid pin
GPIO 19 is a valid pin
GPIO 20 is a valid pin
GPIO 21 is a valid pin
GPIO 22 is a valid pin
GPIO 23 is a valid pin
GPIO 24 is not a valid pin
GPIO 25 is a valid pin
GPIO 26 is a valid pin
GPIO 27 is a valid pin
GPIO 28 is not a valid pin
GPIO 29 is not a valid pin
GPIO 30 is not a valid pin
GPIO 31 is not a valid pin
GPIO 32 is a valid pin
GPIO 33 is a valid pin
GPIO 34 is a valid pin
GPIO 35 is a valid pin
GPIO 36 is a valid pin
GPIO 37 is a valid pin
GPIO 38 is a valid pin
GPIO 39 is a valid pin
I (395) main_task: Returned from app_main()
*/