file(GLOB_RECURSE MANAGED_COMPONENTS_SOURCES 
     "${CMAKE_CURRENT_SOURCE_DIR}/managed_components/**/*.c")
idf_component_register(SRCS "ble_hidd_demo_main.c"
                            "esp_hidd_prf_api.c"
                            "hid_dev.c"
                            "hid_device_le_prf.c"
                            "${MANAGED_COMPONENTS_SOURCES}"
                        #     "managed_components/espressif__keyboard_button/keyboard_button.c"
                        #     "managed_components/espressif__keyboard_button/src/kbd_gpio.c"
                        #     "managed_components/espressif__keyboard_button/src/kbd_gptimer.c"
                        #     "managed_components/espressif__esp_tinyusb/tinyusb.c"
                        #     "managed_components/espressif__esp_tinyusb/descriptors_control.c"
                        #     "managed_components/espressif__esp_tinyusb/tusb_tasks.c"
                        #     "managed_components/espressif__tinyusb/src/tusb.c"
                        #     "managed_components/espressif__tinyusb/src/device/usbd.c"
                        #     "managed_components/espressif__tinyusb/src/portable/raspberrypi/rp2040/dcd_rp2040.c"
                    INCLUDE_DIRS "."
                                "managed_components/espressif__keyboard_button/include"
                                "managed_components/espressif__esp_tinyusb/include"
                                "managed_components/espressif__esp_tinyusb/include_private"
                                "managed_components/espressif__tinyusb/src"
                                "managed_components/espressif__tinyusb/src/device"
                                "components/freertos/FreeRTOS-Kernel/include/freertos"
                        # REQUIRES managed_components components
                        )

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-const-variable)
