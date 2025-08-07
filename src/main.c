#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/hid.h>
#include <string.h>
#include <stdio.h>

#define MAX_GPIO_PINS 48 // Adjust as needed (P0.00 ~ P0.31 and P1.00 ~ P1.15)
#define DELAY_MS 100

const struct device *gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
const struct device *gpio1 = DEVICE_DT_GET(DT_NODELABEL(gpio1));
const struct device *hid_dev = DEVICE_DT_GET_ONE(zephyr_hid_device);

static char *get_pin_name(uint8_t port, uint8_t pin, char *buf) {
    snprintf(buf, 8, "P%d.%02d", port, pin);
    return buf;
}

static void send_hid_string(const char *str) {
    if (!device_is_ready(hid_dev)) return;

    struct hid_keyboard_packet packet = { 0 };

    while (*str) {
        packet.modifiers = 0;
        packet.reserved = 0;
        packet.keys[0] = zmk_hid_usage_from_char(*str);
        hid_send(hid_dev, (uint8_t *)&packet, sizeof(packet));
        k_msleep(10);
        packet.keys[0] = 0;
        hid_send(hid_dev, (uint8_t *)&packet, sizeof(packet));
        k_msleep(10);
        str++;
    }
}

void main(void) {
    usb_enable(NULL);

    printk("GPIO Short Detector started\n");

    if (!device_is_ready(gpio0) || !device_is_ready(gpio1)) {
        printk("GPIO devices not ready!\n");
        return;
    }

    // 配置所有 GPIO 为输入 + 上拉
    for (int pin = 0; pin < 32; pin++) {
        gpio_pin_configure(gpio0, pin, GPIO_INPUT | GPIO_PULL_UP);
    }
    for (int pin = 0; pin < 16; pin++) {
        gpio_pin_configure(gpio1, pin, GPIO_INPUT | GPIO_PULL_UP);
    }

    while (1) {
        for (int p0 = 0; p0 < 32; p0++) {
            for (int p1 = 0; p1 < 16; p1++) {
                int val0 = gpio_pin_get(gpio0, p0);
                int val1 = gpio_pin_get(gpio1, p1);
                if (val0 == 0 && val1 == 0) {
                    char name0[8], name1[8], result[32];
                    get_pin_name(0, p0, name0);
                    get_pin_name(1, p1, name1);
                    snprintf(result, sizeof(result), "%s&%s\n", name0, name1);
                    send_hid_string(result);
                    k_msleep(1000); // 防止重复发送
                }
            }
        }
        k_msleep(DELAY_MS);
    }
}
