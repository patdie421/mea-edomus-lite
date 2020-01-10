#ifndef task_output_h
#define task_output_h

#include "rtos_simu.h"
#include "arduino_simu.h"


struct output_queue_elem_s {
   uint8_t type;
   char data[40];
};

#define MAX_OUTPUT_QUEUE_SIZE 5

#define ERROR_BAD_REQUEST 1
#define ERROR_BAD_ID      2
#define ERROR_BAD_ACTION  3

extern TaskHandle_t outputTaskHandle;
extern QueueHandle_t outputTaskQueue;

bool init_outputTask();

bool send_ok();
bool send_error(int err);
bool send_value_string(char *value);

#endif