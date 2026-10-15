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
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_data_types.h>
#include <zephyr/rtio/rtio.h>

#include <zephyr/usb/class/usbd_hid.h>
#include <zephyr/usb/usbd.h>

#include <zephyr/logging/log.h>

#define DIAL_REPORT_ID 0x01

#define COEF 10

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static const uint8_t hid_report_desc[] = {
    0x05, 0x01,                 // USAGE_PAGE (Generic Desktop)
    0x09, 0x0e,                 // USAGE (System Multi-Axis Controller)
    0xa1, 0x01,                 // COLLECTION (Application)
    0x85, DIAL_REPORT_ID,       //   REPORT_ID (Radial Controller)
    0x05, 0x0d,                 //   USAGE_PAGE (Digitizers)
    0x09, 0x21,                 //   USAGE (Puck)
    0xa1, 0x00,                 //   COLLECTION (Physical)
    0x05, 0x09,                 //     USAGE_PAGE (Buttons)
    0x09, 0x01,                 //     USAGE (Button 1)
    0x95, 0x01,                 //     REPORT_COUNT (1)
    0x75, 0x01,                 //     REPORT_SIZE (1)
    0x15, 0x00,                 //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                 //     LOGICAL_MAXIMUM (1)
    0x81, 0x02,                 //     INPUT (Data,Var,Abs)
    0x05, 0x01,                 //     USAGE_PAGE (Generic Desktop)
    0x09, 0x37,                 //     USAGE (Dial)
    0x95, 0x01,                 //     REPORT_COUNT (1)
    0x75, 0x0f,                 //     REPORT_SIZE (15)
    0x55, 0x0f,                 //     UNIT_EXPONENT (-1)
    0x65, 0x14,                 //     UNIT (Degrees, English Rotation)
    0x36, 0xf0,           0xf1, //     PHYSICAL_MINIMUM (-3600)
    0x46, 0x10,           0x0e, //     PHYSICAL_MAXIMUM (3600)
    0x16, 0xf0,           0xf1, //     LOGICAL_MINIMUM (-3600)
    0x26, 0x10,           0x0e, //     LOGICAL_MAXIMUM (3600)
    0x81, 0x06,                 //     INPUT (Data,Var,Rel)
    0xc0,                       //   END_COLLECTION
    0xc0,                       // END_COLLECTION
};

static uint32_t idle_duration;
static bool dial_ready;

static uint8_t button_state = 0;
static int last_position = 0;

typedef struct __attribute__ ((packed)) {
    uint8_t report_id:8;
    uint16_t button:1;
    int16_t rotation:15;
} hid_report_t;

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
  LOG_WRN("Get Report not implemented, Type %u ID %u", type, id);

  return 0;
}

static int dial_verify_set_report(const struct device *dev, const uint8_t type,
                                  const uint8_t id, const uint16_t len) {
  if (type != HID_REPORT_TYPE_OUTPUT) {
    LOG_WRN("Unsupported report type");
    return -ENOTSUP;
  }

  if (id != 0) {
    LOG_ERR("Unsupported report id %d", id);
    return -ENOTSUP;
  }

  if (len != 1) {
    LOG_WRN("Unsupported report length %d", len);
    return -ENOTSUP;
  }

  return 0;
}

static int dial_set_report(const struct device *dev, const uint8_t type,
                           const uint8_t id, const uint16_t len,
                           const uint8_t *const buf) {
  if (type != HID_REPORT_TYPE_OUTPUT) {
    LOG_WRN("Unsupported report type");
    return -ENOTSUP;
  }

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
  dial_set_report(dev, HID_REPORT_TYPE_OUTPUT, 0U, len, buf);
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
  const struct device *hid_dev;
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

    last_position = rotation;

    UDC_STATIC_BUF_DEFINE(report_buf, sizeof(hid_report_t));

    hid_report_t *report = (hid_report_t *)report_buf;

    // 3. Split the 1 + 15 bits into two 8-bit variables
    // val1 gets: [ 1-bit Button ] + [ Top 7 bits of Rotation ]
    // val2 gets: [ Bottom 8 bits of Rotation ]
    report->report_id = DIAL_REPORT_ID;
    report->button = button_state & 0x01;
    report->rotation = (diff * COEF) & 0x7FFF;

    ret = hid_device_submit_report(hid_dev, sizeof(hid_report_t),
                                   report_buf);
    if (ret != 0) {
      printk("Failed to submit HID report: %d\n", ret);
    }

    k_sleep(K_MSEC(4));
  }

  return 0;
}
