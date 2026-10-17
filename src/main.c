/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/devicetree.h"
#include <sample_usbd.h>

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

#include <zephyr/usb/class/usbd_hid.h>
#include <zephyr/usb/usbd.h>

#include <zephyr/logging/log.h>

#include "hid.h"

#define COEF 10

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define STRIP_NODE DT_ALIAS(led_strip)

#if DT_NODE_HAS_PROP(DT_ALIAS(led_strip), chain_length)
#define STRIP_NUM_PIXELS DT_PROP(DT_ALIAS(led_strip), chain_length)
#else
#error Unable to determine length of LED strip
#endif

static const struct device *const strip = DEVICE_DT_GET(STRIP_NODE);

static const struct device *hid_dev;

static uint32_t idle_duration;
static bool dial_ready;

static uint8_t button_state = 0;
static int last_position = 0;

static bool isWheel = true;

static feature_report_t mult_state = {
    .report_id = REPORT_ID_MULTIPLIER, .wheel = 0, .ac = 0, .reserved = 0};

static void button_input_cb(struct input_event *evt, void *user_data) {
  if (evt->sync == 0) {
    return;
  }

  printk("Button %d %s at %" PRIu32 "\n", evt->code,
         evt->value ? "pressed" : "released", k_cycle_get_32());

  button_state = evt->value ? 1 : 0;
}

INPUT_CALLBACK_DEFINE(NULL, button_input_cb, NULL);

static void dial_iface_ready(const struct device *dev, const bool ready) {
  LOG_INF("HID device %s interface is %s", dev->name,
          ready ? "ready" : "not ready");
  dial_ready = ready;
}

static int dial_get_report(const struct device *dev, const uint8_t type,
                           const uint8_t id, const uint16_t len,
                           uint8_t *const buf) {
  if (type != HID_REPORT_TYPE_FEATURE) {
    LOG_WRN("Unsupported report type");
    return -ENOTSUP;
  }

  if (id != REPORT_ID_MULTIPLIER) {
    LOG_WRN("Unsupported report id");
    return -ENOTSUP;
  }

  memcpy(buf, &mult_state, sizeof(mult_state));

  return sizeof(mult_state);
}

static int dial_verify_set_report(const struct device *dev, const uint8_t type,
                                  const uint8_t id, const uint16_t len) {
  if (type != HID_REPORT_TYPE_FEATURE) {
    LOG_WRN("Unsupported report type");
    return -ENOTSUP;
  }

  if (id != REPORT_ID_MULTIPLIER) {
    LOG_WRN("Unsupported report id");
    return -ENOTSUP;
  }

  return 0;
}

static int dial_set_report(const struct device *dev, const uint8_t type,
                           const uint8_t id, const uint16_t len,
                           const uint8_t *const buf) {

  if (type != HID_REPORT_TYPE_FEATURE) {
    LOG_WRN("Unsupported report type");
    return -ENOTSUP;
  }

  if (id != REPORT_ID_MULTIPLIER) {
    LOG_WRN("Unsupported report id");
    return -ENOTSUP;
  }

  memcpy(&mult_state, buf, sizeof(mult_state));
  mult_state.report_id = REPORT_ID_MULTIPLIER;
  mult_state.reserved = 0;

  return 0;
}

/* Idle duration is stored but not used to calculate idle reports. */
static void dial_set_idle(const struct device *dev, const uint8_t id,
                          const uint32_t duration) {
  LOG_INF("Set Idle %u to %u", id, duration);
  idle_duration = duration;
}

static uint32_t dial_get_idle(const struct device *dev, const uint8_t id) {
  LOG_INF("Get Idle %u to %u", id, idle_duration);
  return idle_duration;
}

static void dial_set_protocol(const struct device *dev, const uint8_t proto) {
  LOG_INF("Protocol changed to %s",
          proto == 0U ? "Boot Protocol" : "Report Protocol");
}

static void dial_output_report(const struct device *dev, const uint16_t len,
                               const uint8_t *const buf) {
  LOG_HEXDUMP_DBG(buf, len, "o.r.");
}

struct hid_device_ops dial_ops = {
    .iface_ready = dial_iface_ready,
    .get_report = dial_get_report,
    .verify_set_report = dial_verify_set_report,
    .set_report = dial_set_report,
    .set_idle = dial_set_idle,
    .get_idle = dial_get_idle,
    .set_protocol = dial_set_protocol,
    .output_report = dial_output_report,
};

/* doc device msg-cb start */
static void msg_cb(struct usbd_context *const usbd_ctx,
                   const struct usbd_msg *const msg) {
  LOG_INF("USBD message: %s", usbd_msg_type_string(msg->type));

  if (msg->type == USBD_MSG_CONFIGURATION) {
    LOG_INF("\tConfiguration value %d", msg->status);
  }

  if (usbd_can_detect_vbus(usbd_ctx)) {
    if (msg->type == USBD_MSG_VBUS_READY) {
      if (usbd_enable(usbd_ctx)) {
        LOG_ERR("Failed to enable device support");
      }
    }

    if (msg->type == USBD_MSG_VBUS_REMOVED) {
      if (usbd_disable(usbd_ctx)) {
        LOG_ERR("Failed to disable device support");
      }
    }
  }
}
/* doc device msg-cb end */

int main(void) {
  struct usbd_context *sample_usbd;
  const struct device *magnet_dev;

  int ret;
  int res;

  hid_dev = DEVICE_DT_GET_ONE(zephyr_hid_device);
  magnet_dev = DEVICE_DT_GET_ONE(ams_as5600);
  if (!device_is_ready(hid_dev)) {
    LOG_ERR("HID Device is not ready");
    return -EIO;
  }

  if (!device_is_ready(magnet_dev)) {
    LOG_ERR("AS5600 is not ready");
    return -EIO;
  }

  if (!device_is_ready(strip)) {
    LOG_ERR("LED is not ready");
    return -EIO;
  }

  struct led_rgb pixels = {.r = 0x7f, .g = 0x00, .b = isWheel ? 0x00 : 0x7f};

  led_strip_update_rgb(strip, &pixels, STRIP_NUM_PIXELS);

  ret = hid_device_register(hid_dev, hid_report_desc, sizeof(hid_report_desc),
                            &dial_ops);
  if (ret != 0) {
    LOG_ERR("Failed to register HID Device, %d", ret);
    return ret;
  }

  if (IS_ENABLED(CONFIG_USBD_HID_SET_POLLING_PERIOD)) {
    ret = hid_device_set_in_polling(hid_dev, 1000);
    if (ret) {
      LOG_WRN("Failed to set IN report polling period, %d", ret);
    }

    ret = hid_device_set_out_polling(hid_dev, 1000);
    if (ret != 0 && ret != -ENOTSUP) {
      LOG_WRN("Failed to set OUT report polling period, %d", ret);
    }
  }

  sample_usbd = sample_usbd_init_device(msg_cb);
  if (sample_usbd == NULL) {
    LOG_ERR("Failed to initialize USB device");
    return -ENODEV;
  }

  if (!usbd_can_detect_vbus(sample_usbd)) {
    /* doc device enable start */
    ret = usbd_enable(sample_usbd);
    if (ret) {
      LOG_ERR("Failed to enable device support");
      return ret;
    }
    /* doc device enable end */
  }

  LOG_INF("HID keyboard sample is initialized");

  struct sensor_value temp;

  UDC_STATIC_BUF_DEFINE(report_buf, sizeof(mouse_report_t));

  while (true) {
    if (!dial_ready) {
      LOG_INF("USB HID device is not ready");
      continue;
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

    if(diff == 0) continue;

    last_position = rotation;

    if (isWheel) {
      mouse_report_t *report = (mouse_report_t *)report_buf;

      report->report_id = REPORT_ID_MOUSE;
      report->buttons = 0;
      report->x_axis = 0;
      report->y_axis = 0;
      report->wheel = -diff;
      report->ac_pan = 0;

      ret = hid_device_submit_report(hid_dev, sizeof(mouse_report_t), report_buf);
      if (ret != 0) {
        printk("Failed to submit HID report: %d\n", ret);
      }

    } else {
      UDC_STATIC_BUF_DEFINE(report_buf, sizeof(dial_report_t));

      dial_report_t *report = (dial_report_t *)report_buf;

      report->report_id = REPORT_ID_DIAL;
      report->button = button_state & 0x01;
      report->rotation = (diff * COEF) & 0x7FFF;

      ret =
          hid_device_submit_report(hid_dev, sizeof(dial_report_t), report_buf);
      if (ret != 0) {
        printk("Failed to submit HID report: %d\n", ret);
      }
    }

    k_sleep(K_MSEC(4));
  }

  return 0;
}
