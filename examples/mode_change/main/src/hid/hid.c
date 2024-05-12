#include "esp_event.h"
#include "esp_log.h"
#include "hid_dev.h"
#include "keyboard_button.h"


static uint16_t hid_conn_id = 0;

keyboard_btn_config_t cfg = {
    .output_gpios = (int[])
    {
        40, 39, 38, 45, 48, 47
    },
    .output_gpio_num = 6,
    .input_gpios = (int[])
    {
        21, 14, 13, 12, 11, 10, 9, 7, 15, 16, 17, 18
    },
    .input_gpio_num = 15,
    .active_level = 1,
    .debounce_ticks = 2,
    .ticks_interval = 500,      // us
    .enable_power_save = false, // enable power save
};

keyboard_btn_handle_t kbd_handle = NULL;

uint8_t keycodes[2][2] = {
    {HID_KEY_A, HID_KEY_B},  // HID keycodes for 'a', 'b'
    {HID_KEY_C, HID_KEY_D}   // HID keycodes for 'c', 'd'
};

void keyboard_cb(keyboard_btn_handle_t kbd_handle, keyboard_btn_report_t kbd_report, void *user_data)
{
    uint8_t keycode = 0;
    if (kbd_report.key_pressed_num == 0)
    {
        ESP_LOGI(__func__, "All keys released\n\n");
        esp_hidd_send_keyboard_value(hid_conn_id, 0, &keycode, 1);
        return;
    }
    printf("pressed: ");
    for (int i = 0; i < kbd_report.key_pressed_num; i++) {
        printf("(%d,%d) ", kbd_report.key_data[i].output_index, kbd_report.key_data[i].input_index);
        keycode = keycodes[kbd_report.key_data[i].output_index][kbd_report.key_data[i].input_index];
        esp_hidd_send_keyboard_value(hid_conn_id, 0, &keycode, 1);
    }
    printf("\n\n");
}

keyboard_btn_cb_config_t cb_cfg = {
    .event = KBD_EVENT_PRESSED,
    .callback = keyboard_cb,
};

void keyboard_task(void) {
    keyboard_button_create(&cfg, &kbd_handle);
    keyboard_button_register_cb(kbd_handle, cb_cfg, NULL);
}