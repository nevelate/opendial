#include "usb_device.h"
#include <zephyr/usb/usbd.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_device);

USBD_DEVICE_DEFINE(usb_device,
                   DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
                   USB_VID, USB_PID);

// These fields are optional
USBD_DESC_LANG_DEFINE(language); // Will be set to English (United States)
USBD_DESC_MANUFACTURER_DEFINE(manufacturer, MANUFACTURER);
USBD_DESC_PRODUCT_DEFINE(product, PRODUCT);

// We need to define a configuration for each supported speed. For simplicity we will not do high speed. Only full speed.
static const uint8_t attributes = 0;

#define EXAMPLE_MAX_POWER 125 // In 2mA units so its 250mA
USBD_DESC_CONFIG_DEFINE(fs_cfg_desc, "Full Speed Configuration");
USBD_CONFIGURATION_DEFINE(example_fs_config, attributes, EXAMPLE_MAX_POWER, &fs_cfg_desc);

int usb_device_init(void) {
    int err;

    err = usbd_add_descriptor(&usb_device, &language);
    if (err) {
        LOG_ERR("Failed to add language descriptor %d", err);
        return err;
    }

    err = usbd_add_descriptor(&usb_device, &manufacturer);
    if (err) {
        LOG_ERR("Failed to add manufacturer descriptor %d", err);
        return err;
    }

    err = usbd_add_descriptor(&usb_device, &product);
    if (err) {
        LOG_ERR("Failed to add product descriptor %d", err);
        return err;
    }

    //
    // Adding configuration for full speed
    //
    err = usbd_add_configuration(&usb_device, USBD_SPEED_FS, &example_fs_config);
    if (err) {
        LOG_ERR("Failed to add Full-Speed configuration");
        return err;
    }

    err = usbd_register_all_classes(&usb_device, USBD_SPEED_FS, 1, NULL);
    if (err) {
        LOG_ERR("Failed to add register class");
        return err;
    }

    /*
     * The device descriptor has three fields for identifying device class
     * bDeviceClass, bDeviceSubClass, bDeviceProtocol
     *
     * If the device provides multiple USB functions (a composite device)
     * it should use the Miscellaneous Device Class to indicate that the host
     * should look at the interface descriptors to determine the device class.
     *
     * If it's a single-function device like HID, these fields can be set to 0
     */
    if (IS_ENABLED(CONFIG_USBD_CDC_ACM_CLASS) ||
        IS_ENABLED(CONFIG_USBD_CDC_ECM_CLASS) ||
        IS_ENABLED(CONFIG_USBD_CDC_NCM_CLASS) ||
        IS_ENABLED(CONFIG_USBD_MIDI2_CLASS) ||
        IS_ENABLED(CONFIG_USBD_AUDIO2_CLASS) ||
        IS_ENABLED(CONFIG_USBD_VIDEO_CLASS)) {

        usbd_device_set_code_triple(&usb_device, USBD_SPEED_FS, USB_BCC_MISCELLANEOUS, 0x02, 0x01);
    } else {
        usbd_device_set_code_triple(&usb_device, USBD_SPEED_FS, 0, 0, 0);
    }

    // Here we could add a callback for handling device state changes
    // This is used for things like sleep and wake up
    // usbd_msg_register_cb(&example_usbd, example_usbd_msg_cb);

    //
    // Initializing the USB device
    //
    err = usbd_init(&usb_device);
    if (err) {
        LOG_ERR("Failed to initialize device %d", err);
        return err;
    }

    // For self powered devices we would only want to enable it if it's plugged in
    if (!usbd_can_detect_vbus(&usb_device)) {
        err = usbd_enable(&usb_device);
    }

    return 0;
}
