idf_component_register(
    SRCS    "ble_hidd_demo_main.c"
            "components/esp_hidd_prf_api/esp_hidd_prf_api.c"
            "components/hid_dev/hid_dev.c"
            "components/hidd_le_prf/hid_device_le_prf.c"
            "components/matrix_gpio/matrix_gpio.c"
    INCLUDE_DIRS    "."
                    "components/esp_hidd_prf_api"
                    "components/hid_dev"
                    "components/hidd_le_prf"
                    "components/matrix_gpio"
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-const-variable)
