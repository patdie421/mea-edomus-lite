#ifndef __SERIAL_H
#define __SERIAL_H

#include <inttypes.h>

int serial_open(char *dev, int speed);
int32_t get_speed_from_speed_t(speed_t speed);
int16_t get_dev_and_speed(char *device, char *dev, int16_t dev_l, speed_t *speed);

#endif
