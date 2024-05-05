idf_component_register(SRCS "ble_hidd_demo_main.c"
                            "esp_hidd_prf_api.c"
                            "hid_dev.c"
                            "hid_device_le_prf.c"
                            "managed_components/espressif__keyboard_button/keyboard_button.c"
                            "managed_components/espressif__keyboard_button/src/kbd_gpio.c"
                            "managed_components/espressif__keyboard_button/src/kbd_gptimer.c"
                    INCLUDE_DIRS "."
                                "managed_components/espressif__keyboard_button/include"
                                "managed_components/espressif__esp_tinyusb/include"
                                "managed_components/espressif__tinyusb/src"
                                "components/freertos/FreeRTOS-Kernel/include/freertos"
                        )

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-const-variable)
