#include "hid.h"

#include <zephyr/logging/log.h>
#include <zephyr/usb/class/usbd_hid.h>
#include <zephyr/usb/usbd.h>

LOG_MODULE_REGISTER(hid);

UDC_STATIC_BUF_DEFINE(wheel_report_buf, sizeof(wheel_report_t));
UDC_STATIC_BUF_DEFINE(dial_report_buf, sizeof(dial_report_t));

static const struct device *hid_dev = DEVICE_DT_GET_ONE(zephyr_hid_device);

static feature_report_t mult_state = {
    .report_id = REPORT_ID_MULTIPLIER, .wheel = 0, .ac_pan = 0, .reserved = 0};

static bool hid_ready = false;

static void iface_ready(const struct device *dev, const bool ready) {
  LOG_INF("HID device %s interface is %s", dev->name,
          ready ? "ready" : "not ready");
  hid_ready = ready;
}

static int get_report(const struct device *dev, const uint8_t type,
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

static int verify_set_report(const struct device *dev, const uint8_t type,
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

static int set_report(const struct device *dev, const uint8_t type,
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

struct hid_device_ops device_ops = {
    .iface_ready = iface_ready,
    .get_report = get_report,
    .verify_set_report = verify_set_report,
    .set_report = set_report,
};

int hid_device_init(void) {
  if (!device_is_ready(hid_dev)) {
    LOG_ERR("HID Device is not ready");
    return -EIO;
  }

  int err = hid_device_register(hid_dev, hid_report_desc,
                                sizeof(hid_report_desc), &device_ops);
  if (err) {
    LOG_ERR("Failed to register HID Device, %d", err);
    return err;
  }

  return 0;
}

bool get_hid_ready(){
    return hid_ready;
}

int hid_device_send_wheel_report(int value) {
  wheel_report_t *report = (wheel_report_t *)wheel_report_buf;

  report->report_id = REPORT_ID_MOUSE;
  report->buttons = 0;
  report->x_axis = 0;
  report->y_axis = 0;
  report->wheel = value;
  report->ac_pan = 0;

  return hid_device_submit_report(hid_dev, sizeof(wheel_report_t),
                                  wheel_report_buf);
}

int hid_device_send_dial_report(int rotation, bool button_state) {
  dial_report_t *report = (dial_report_t *)dial_report_buf;

  report->report_id = REPORT_ID_DIAL;
  report->button = button_state & 0x01;
  report->rotation = rotation & 0x7FFF;

  return hid_device_submit_report(hid_dev, sizeof(dial_report_t),
                                  dial_report_buf);
}
