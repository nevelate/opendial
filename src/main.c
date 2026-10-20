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

#define MODE_SWITCH_DELAY 5000
#define DIRECTION_SWITCH_DELAY 10000
#define DEAD_ZONE 4 // 0.4 degrees
#define BRIGHTNESS 0x7F

#if DT_NODE_HAS_PROP(DT_ALIAS(led_strip), chain_length)
#define STRIP_NUM_PIXELS DT_PROP(DT_ALIAS(led_strip), chain_length)
#else
#error Unable to determine length of LED strip
#endif

static const struct device *strip = DEVICE_DT_GET(DT_ALIAS(led_strip));
static const struct device *magnet_dev = DEVICE_DT_GET_ONE(ams_as5600);

static uint8_t button_state = 0;
static int last_position = 0;
static int64_t last_pressed_time;
static int64_t delta;

static int last_mode = -1;

static struct led_rgb wheel_mode_led = {.r = 0x00, .g = BRIGHTNESS, .b = 0x00};
static struct led_rgb dial_mode_led = {.r = 0x00, .g = 0x00, .b = BRIGHTNESS};

static struct led_rgb direction_led = {.r = BRIGHTNESS, .g = 0x00, .b = 0x00};

static void button_input_cb(struct input_event *evt, void *user_data) {
  if (evt->sync == 0) {
    return;
  }

  printk("Button %d %s at %" PRIu32 "\n", evt->code,
         evt->value ? "pressed" : "released", k_cycle_get_32());

  button_state = evt->value ? 1 : 0;

  if (evt->value) {
    last_pressed_time = k_uptime_get();
  } else {
    int64_t elapsed_time = k_uptime_delta(&last_pressed_time);
    if (elapsed_time > DIRECTION_SWITCH_DELAY) {
      get_mode() == DIAL_MODE ? set_dial_direction(get_dial_direction() * -1)
                              : set_wheel_direction(get_wheel_direction() * -1);
    } else if (elapsed_time > MODE_SWITCH_DELAY) {
      set_mode(get_mode() == DIAL_MODE ? WHEEL_MODE : DIAL_MODE);
    }
  }
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

  struct sensor_value temp;

  while (true) {
    if (!get_hid_ready()) {
      LOG_INF("USB HID device is not ready");
      continue;
    }

    if (button_state) {
      delta = k_uptime_get() - last_pressed_time;
      if (delta > DIRECTION_SWITCH_DELAY) {
        led_strip_update_rgb(strip, &direction_led, STRIP_NUM_PIXELS);
      } else if (delta > MODE_SWITCH_DELAY) {
        led_strip_update_rgb(
            strip, get_mode() == DIAL_MODE ? &wheel_mode_led : &dial_mode_led,
            STRIP_NUM_PIXELS);
      }
    } else {
      led_strip_update_rgb(
          strip, get_mode() == WHEEL_MODE ? &wheel_mode_led : &dial_mode_led,
          STRIP_NUM_PIXELS);
    }

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

    int rotation = (temp.val1 * 1000000 + temp.val2) / 100000;

    if (last_position == 0)
      last_position = rotation;

    int16_t dif1 = 0;
    if (rotation > last_position) {
      dif1 = rotation - last_position - 3600;
    } else {
      dif1 = 3600 - last_position + rotation;
    }
    int16_t dif2 = rotation - last_position;

    int16_t diff = abs(dif1) > abs(dif2) ? dif2 : dif1;

    printk("Diff: %d\n", diff);

    if (abs(diff) <= DEAD_ZONE)
      continue;

    last_position = rotation;

    if (get_mode() == WHEEL_MODE) {
      hid_device_send_wheel_report(diff * get_wheel_direction());

    } else {
      hid_device_send_dial_report(diff * get_dial_direction(), button_state);
    }

    k_sleep(K_MSEC(4));
  }

  return 0;
}
