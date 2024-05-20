#include <stdio.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "led_strip.h"

#define ESP_CHANNEL         1
#define LED_STRIP           8
#define LED_STRIP_MAX_LEDS  1

led_strip_handle_t led_strip;

static const char * TAG = "esp_now_init";

static esp_err_t init_wifi(void)
{
    wifi_init_config_t wifi_init_config =  WIFI_INIT_CONFIG_DEFAULT();
    esp_netif_init();
    esp_event_loop_create_default();
    nvs_flash_init();
    esp_wifi_init(&wifi_init_config);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_storage(WIFI_STORAGE_FLASH);
    esp_wifi_start();
    ESP_LOGI(TAG, "wifi init completed");
    return ESP_OK;
}

void recv_cb(const esp_now_recv_info_t * esp_now_info, const uint8_t *data, int data_len)
{
    ESP_LOGI(TAG, "Data received: " MACSTR "%s", MAC2STR(esp_now_info->src_addr), data);
    ESP_LOGI(TAG, "Data is %s", data);
    vTaskDelay(pdMS_TO_TICKS(10));
}

static esp_err_t init_esp_now(void)
{
    esp_now_init();
    esp_now_register_recv_cb(recv_cb);

    ESP_LOGI(TAG, "esp now init completed");
    return ESP_OK;
}

void app_main(void)
{
    ESP_ERROR_CHECK(init_wifi());
    ESP_ERROR_CHECK(init_esp_now());
}
