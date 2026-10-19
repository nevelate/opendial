/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/devicetree.h"

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_data_types.h>
#include <zephyr/rtio/rtio.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#include "hid.h"
#include "settings.h"
#include "usb_device.h"

#define COEF 10

#if DT_NODE_HAS_PROP(DT_ALIAS(led_strip), chain_length)
#define STRIP_NUM_PIXELS DT_PROP(DT_ALIAS(led_strip), chain_length)
#else
#error Unable to determine length of LED strip
#endif

static const struct device *strip = DEVICE_DT_GET(DT_ALIAS(led_strip));
static const struct device *magnet_dev = DEVICE_DT_GET_ONE(ams_as5600);

static uint8_t button_state = 0;
static int last_position = 0;

static void button_input_cb(struct input_event *evt, void *user_data) {
  if (evt->sync == 0) {
    return;
  }

  printk("Button %d %s at %" PRIu32 "\n", evt->code,
         evt->value ? "pressed" : "released", k_cycle_get_32());

  button_state = evt->value ? 1 : 0;
}

INPUT_CALLBACK_DEFINE(NULL, button_input_cb, NULL);

int main(void) {
  int res;

  hid_device_init();
  usb_device_init();

  init_settings();

  if (!device_is_ready(magnet_dev)) {
    LOG_ERR("AS5600 is not ready");
    return -EIO;
  }

  if (!device_is_ready(strip)) {
    LOG_ERR("LED is not ready");
    return -EIO;
  }

  struct led_rgb pixels = {
      .r = 0x7f, .g = 0x00, .b = get_mode() == WHEEL_MODE ? 0x00 : 0x7f};

  struct sensor_value temp;

  while (true) {
    if (!get_hid_ready()) {
      LOG_INF("USB HID device is not ready");
      continue;
    }

    led_strip_update_rgb(strip, &pixels, STRIP_NUM_PIXELS);

    res = sensor_sample_fetch(magnet_dev);
    if (res != 0) {
      printk("sample_fetch() failed: %d\n", res);
      return res;
    }

    res = sensor_channel_get(magnet_dev, SENSOR_CHAN_ROTATION, &temp);
    if (res != 0) {
      printk("channel_get() failed: %d\n", res);
      return res;
    }

    printk("Rotation: %d.%d\n", temp.val1, temp.val2);

    int rotation = temp.val1;

    if (last_position == 0)
      last_position = rotation;

    int16_t dif1 = 0;
    if (rotation > last_position) {
      dif1 = rotation - last_position - 360;
    } else {
      dif1 = 360 - last_position + rotation;
    }
    int16_t dif2 = rotation - last_position;

    int16_t diff = abs(dif1) > abs(dif2) ? dif2 : dif1;

    printk("Diff: %d\n", diff);

    if (diff == 0)
      continue;

    last_position = rotation;

    if (get_mode() == WHEEL_MODE) {
      hid_device_send_wheel_report(diff * get_wheel_direction());

    } else {
      hid_device_send_dial_report(diff * COEF * get_dial_direction(),
                                  button_state);
    }

    k_sleep(K_MSEC(4));
  }

  return 0;
}
