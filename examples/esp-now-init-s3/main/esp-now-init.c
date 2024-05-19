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

// static uint8_t peer_mac [ESP_NOW_ETH_ALEN] = {0x24, 0x58, 0x7C, 0xCD, 0x93, 0x30};
static uint8_t peer_mac [ESP_NOW_ETH_ALEN] = {0x08, 0xD1, 0xF9, 0x27, 0x75, 0x30};

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
    vTaskDelay(pdMS_TO_TICKS(10));
}

void send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if(status == ESP_NOW_SEND_SUCCESS)
    {
        ESP_LOGI(TAG, "ESP_NOW_SEND_SUCCESS");
    }
    else
    {
        ESP_LOGE(TAG, "ESP_NOW_SEND_FAIL");
    }
    vTaskDelay(pdMS_TO_TICKS(10));
}

static esp_err_t init_esp_now(void)
{
    esp_now_init();
    esp_now_register_recv_cb(recv_cb);
    esp_now_register_send_cb(send_cb);

    ESP_LOGI(TAG, "esp now init completed");
    return ESP_OK;
}

static esp_err_t register_peer(uint8_t *peer_addr)
{
    esp_now_peer_info_t esp_now_peer_info = {};
    memcpy(esp_now_peer_info.peer_addr, peer_addr, ESP_NOW_ETH_ALEN);
    esp_now_peer_info.channel = ESP_CHANNEL;
    esp_now_peer_info.ifidx = ESP_IF_WIFI_STA;
    esp_now_add_peer(&esp_now_peer_info);
    return ESP_OK;
}

static esp_err_t esp_now_send_data(const uint8_t *peer_addr, const uint8_t *data, size_t len)
{
    esp_now_send(peer_addr, data, len);
    return ESP_OK;
}

static esp_err_t init_led_strip(void)
{
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP, // The GPIO that connected to the LED strip's data line
        .max_leds = LED_STRIP_MAX_LEDS, // The number of LEDs in the strip,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB, // Pixel format of your LED strip
        .led_model = LED_MODEL_WS2812, // LED strip model
        .flags.invert_out = false, // whether to invert the output signal (useful when your hardware has a level inverter)
    };

    led_strip_rmt_config_t rmt_config = {
    #if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
        .rmt_channel = 0,
    #else
        .clk_src = RMT_CLK_SRC_DEFAULT, // different clock source can lead to different power consumption
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false, // whether to enable the DMA feature
    #endif
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));

    return ESP_OK;

}

void app_main(void)
{
    ESP_ERROR_CHECK(init_wifi());
    ESP_ERROR_CHECK(init_esp_now());
    ESP_ERROR_CHECK(register_peer(peer_mac));
    ESP_ERROR_CHECK(init_led_strip());

    uint8_t dataR [] = "255|0|0";
    uint8_t dataG [] = "0|255|0";
    uint8_t dataB [] = "0|0|255";

    uint8_t count = 0;

    while (1)
    {
        count ++;
        if (count > 2)
        {
            count = 0;
        }

        switch (count)
        {
            case 0:
                esp_now_send_data(peer_mac, dataR, 32);
                led_strip_set_pixel(led_strip, 0, 255, 0, 0);
                break;
            case 1:
                esp_now_send_data(peer_mac, dataG, 32);
                led_strip_set_pixel(led_strip, 0, 0, 255, 0);
                break;
            case 2:
                esp_now_send_data(peer_mac, dataB, 32);
                led_strip_set_pixel(led_strip, 0, 0, 0, 255);
                break;
            default:
                break;
        }

        led_strip_refresh(led_strip);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
