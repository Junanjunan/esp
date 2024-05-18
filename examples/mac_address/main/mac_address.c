#include "esp_system.h"
#include "esp_mac.h"
#include "esp_log.h"

void app_main(void)
{
    uint8_t mac[6];  // Buffer to hold MAC address
    esp_err_t ret = esp_efuse_mac_get_default(mac);

    if (ret == ESP_OK)
    {
        // Print the MAC address
        ESP_LOGI("MAC", "MAC Address: %02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    else
    {
        ESP_LOGE("MAC", "Failed to get MAC address");
    }
}
