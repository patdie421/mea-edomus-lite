#ifndef task_input_h
#define task_input_h

#include "arduino_simu.h"
#include "rtos_simu.h"

extern TaskHandle_t inputTaskHandle;

bool init_inputTask();
bool get_value(int8_t characteristicId);

#endif