#pragma once

#include <stdint.h>
#include <stdbool.h>

#define DIAL_MODE 0
#define WHEEL_MODE 1

int init_settings(void);
int set_wheel_direction(int direction);
int set_dial_direction(int direction);
int set_mode(int mode);

int get_wheel_direction();
int get_dial_direction();
int get_mode();
