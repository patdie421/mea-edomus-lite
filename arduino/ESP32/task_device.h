#ifndef task_device_h
#define task_device_h

#include <inttypes.h>
#include <string.h>
#include <ctype.h>

#include "task_device.h"

struct device_queue_elem_s {
   uint8_t action;
   char data[20];
};

void deviceTask(void *pvParameters);

bool set_value(int8_t characteristicId, char *parameters);

#endif