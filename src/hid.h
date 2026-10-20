#pragma once

#include <stdint.h>
#include <stdbool.h>

#define REPORT_ID_DIAL 0x01
#define REPORT_ID_MOUSE 0x02
#define REPORT_ID_MULTIPLIER 0x03

#define RESOLUTION_MULTIPLIER 80

static const uint8_t hid_report_desc[] = {
    0x05, 0x01,                 // USAGE_PAGE (Generic Desktop)
    0x09, 0x0e,                 // USAGE (System Multi-Axis Controller)
    0xa1, 0x01,                 // COLLECTION (Application)
    0x85, REPORT_ID_DIAL,       //   REPORT_ID (Radial Controller)
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
    0x36, 0xf0, 0xf1,           //     PHYSICAL_MINIMUM (-3600)
    0x46, 0x10, 0x0e,           //     PHYSICAL_MAXIMUM (3600)
    0x16, 0xf0, 0xf1,           //     LOGICAL_MINIMUM (-3600)
    0x26, 0x10, 0x0e,           //     LOGICAL_MAXIMUM (3600)
    0x81, 0x06,                 //     INPUT (Data,Var,Rel)
    0xc0,                       //   END_COLLECTION
    0xc0,                       // END_COLLECTION

    // Reset globals
    0x55, 0x00,                   // UNIT_EXPONENT (0)
    0x65, 0x00,                   // UNIT (None)
    0x35, 0x00,                   // PHYSICAL_MINIMUM (0)
    0x45, 0x00,                   // PHYSICAL_MAXIMUM (0)

    0x05, 0x01,                   // Usage Page (Generic Desktop Ctrls)
    0x09, 0x02,                   // Usage (Mouse)
    0xA1, 0x01,                   // Collection (Application)
    0x05, 0x01,                   //   Usage Page (Generic Desktop Ctrls)
    0x09, 0x02,                   //   Usage (Mouse)
    0xA1, 0x02,                   //   Collection (Logical)
    0x85, REPORT_ID_MOUSE,        //     Report ID (REPORT_ID_MOUSE)
    0x09, 0x01,                   //     Usage (Pointer)
    0xA1, 0x00,                   //     Collection (Physical)
    0x05, 0x09,                   //       Usage Page (Button)
    0x19, 0x01,                   //       Usage Minimum (0x01)
    0x29, 0x08,                   //       Usage Maximum (0x08)
    0x95, 0x08,                   //       Report Count (8)
    0x75, 0x01,                   //       Report Size (1)
    0x15, 0x00,                   //       Logical Minimum (0)
    0x25, 0x01,                   //       Logical Maximum (1)
    0x81, 0x02,                   //       Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,                   //       Usage Page (Generic Desktop Ctrls)
    0x09, 0x30,                   //       Usage (X)
    0x09, 0x31,                   //       Usage (Y)
    0x95, 0x02,                   //       Report Count (2)
    0x75, 0x10,                   //       Report Size (16)
    0x16, 0x00, 0x80,             //       Logical Minimum (-32768)
    0x26, 0xFF, 0x7F,             //       Logical Maximum (32767)
    0x81, 0x06,                   //       Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xA1, 0x02,                   //       Collection (Logical)
    0x85, REPORT_ID_MULTIPLIER,   //         Report ID (REPORT_ID_MULTIPLIER)
    0x09, 0x48,                   //         Usage (Resolution Multiplier)
    0x95, 0x01,                   //         Report Count (1)
    0x75, 0x02,                   //         Report Size (2)
    0x15, 0x00,                   //         Logical Minimum (0)
    0x25, 0x01,                   //         Logical Maximum (1)
    0x35, 0x01,                   //         Physical Minimum (1)
    0x45, RESOLUTION_MULTIPLIER,  //         Physical Maximum (RESOLUTION_MULTIPLIER)
    0xB1, 0x02,                   //         Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, REPORT_ID_MOUSE,        //         Report ID (REPORT_ID_MOUSE)
    0x09, 0x38,                   //         Usage (Wheel)
    0x35, 0x00,                   //         Physical Minimum (0)
    0x45, 0x00,                   //         Physical Maximum (0)
    0x16, 0x00, 0x80,             //         Logical Minimum (-32768)
    0x26, 0xFF, 0x7F,             //         Logical Maximum (32767)
    0x75, 0x10,                   //         Report Size (16)
    0x81, 0x06,                   //         Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,                         //       End Collection
    0xA1, 0x02,                   //       Collection (Logical)
    0x85, REPORT_ID_MULTIPLIER,   //         Report ID (REPORT_ID_MULTIPLIER)
    0x09, 0x48,                   //         Usage (Resolution Multiplier)
    0x75, 0x02,                   //         Report Size (2)
    0x15, 0x00,                   //         Logical Minimum (0)
    0x25, 0x01,                   //         Logical Maximum (1)
    0x35, 0x01,                   //         Physical Minimum (1)
    0x45, RESOLUTION_MULTIPLIER,  //         Physical Maximum (RESOLUTION_MULTIPLIER)
    0xB1, 0x02,                   //         Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x35, 0x00,                   //         Physical Minimum (0)
    0x45, 0x00,                   //         Physical Maximum (0)
    0x75, 0x04,                   //         Report Size (4)
    0xB1, 0x03,                   //         Feature (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, REPORT_ID_MOUSE,        //         Report ID (REPORT_ID_MOUSE)
    0x05, 0x0C,                   //         Usage Page (Consumer)
    0x16, 0x00, 0x80,             //         Logical Minimum (-32768)
    0x26, 0xFF, 0x7F,             //         Logical Maximum (32767)
    0x75, 0x10,                   //         Report Size (16)
    0x0A, 0x38, 0x02,             //         Usage (AC Pan)
    0x81, 0x06,                   //         Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,                         //       End Collection
    0xC0,                         //     End Collection
    0xC0,                         //   End Collection
    0xC0,                         // End Collection
};

typedef struct __attribute__((packed)) {
  uint8_t report_id : 8;
  uint16_t button : 1;
  int16_t rotation : 15;
} dial_report_t;

typedef struct __attribute__((packed)) {
  uint8_t report_id;
  uint8_t buttons;
  int16_t x_axis;
  int16_t y_axis;
  int16_t wheel;
  int16_t ac_pan;
} wheel_report_t;

typedef struct __attribute__((packed)) {
  uint8_t report_id : 8;
  uint8_t wheel : 2;
  uint8_t ac_pan : 2;
  uint8_t reserved : 4;
} feature_report_t;

int hid_device_init(void);
bool get_hid_ready(void);
int hid_device_send_wheel_report(int value);
int hid_device_send_dial_report(int rotation, bool button_state);
