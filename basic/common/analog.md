ADC (Analog to Digital Converter)

ChatGPT

    The ESP32 is equipped with a versatile ADC (Analog to Digital Converter) that allows certain GPIO pins to read analog voltages.
    Analog input capabilities are crucial for interfacing with a wide range of analog sensors (like temperature, light, or pressure sensors)
    by reading voltage levels and converting them into digital values that the processor can understand and use.

    For the ESP32, the available ADC channels are distributed across a subset of the GPIOs.
    Here's a summary of the GPIOs that can be used for analog input:

    ADC1 Channels:
    GPIO36 (ADC1_CH0)
    GPIO37 (ADC1_CH1)
    GPIO38 (ADC1_CH2)
    GPIO39 (ADC1_CH3)
    GPIO32 (ADC1_CH4)
    GPIO33 (ADC1_CH5)
    GPIO34 (ADC1_CH6)
    GPIO35 (ADC1_CH7)

    ADC2 Channels:
    GPIO4 (ADC2_CH0)
    GPIO0 (ADC2_CH1)
    GPIO2 (ADC2_CH2)
    GPIO15 (ADC2_CH3)
    GPIO13 (ADC2_CH4)
    GPIO12 (ADC2_CH5)
    GPIO14 (ADC2_CH6)
    GPIO27 (ADC2_CH7)
    GPIO25 (ADC2_CH8)
    GPIO26 (ADC2_CH9)

    It's important to note that when using Wi-Fi,
    the ADC2 channels cannot be used for analog reading because they are used internally by the Wi-Fi driver.
    Therefore, for applications that require analog inputs alongside Wi-Fi functionality, it's best to use pins from ADC1.

    Also, the actual availability of these pins for analog input in your specific ESP32-based development board (like the ESP-WROOM-32)
    may vary depending on the board's design and any other functions assigned to those pins.
    It's always a good idea to refer to the specific documentation for your development board for the most accurate and detailed information.