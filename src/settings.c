#include "settings.h"
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>

static int active_mode = WHEEL_MODE;
static int dial_direction = 1;
static int wheel_direction = -1;

static int settings_set(const char *name, size_t len, settings_read_cb read_cb,
                        void *cb_arg) {
  const char *next;
  int rc;

  if (settings_name_steq(name, "dial", &next) && !next) {
    if (len != sizeof(dial_direction)) {
      return -EINVAL;
    }

    rc = read_cb(cb_arg, &dial_direction, sizeof(dial_direction));
    if (rc >= 0) {
      return 0;
    }

    return rc;
  } else if (settings_name_steq(name, "wheel", &next) && !next) {
    if (len != sizeof(wheel_direction)) {
      return -EINVAL;
    }

    rc = read_cb(cb_arg, &wheel_direction, sizeof(wheel_direction));
    if (rc >= 0) {
      return 0;
    }

    return rc;
  } else if (settings_name_steq(name, "mode", &next) && !next) {
    if (len != sizeof(active_mode)) {
      return -EINVAL;
    }

    rc = read_cb(cb_arg, &active_mode, sizeof(active_mode));
    if (rc >= 0) {
      return 0;
    }

    return rc;
  }

  return -ENOENT;
}

struct settings_handler settings_conf = {.name = "opendial",
                                         .h_set = settings_set};

int init_settings() {
  settings_subsys_init();
  settings_register(&settings_conf);
  settings_load();

  return 0;
}

int set_dial_direction(int direction) {
  dial_direction = direction;
  return settings_save_one("opendial/dial", &dial_direction,
                           sizeof(dial_direction));
}

int set_wheel_direction(int direction) {
  wheel_direction = direction;
  return settings_save_one("opendial/wheel", &wheel_direction,
                           sizeof(wheel_direction));
}

int set_mode(int mode) {
  return active_mode = mode;
  return settings_save_one("opendial/mode", &active_mode, sizeof(active_mode));
}

int get_dial_direction() { return dial_direction; }

int get_wheel_direction() { return wheel_direction; }

int get_mode() { return active_mode; }
