#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>

#include "task_output.h"

TaskHandle_t  outputTaskHandle;
QueueHandle_t outputTaskQueue;


bool send_ok()
{
   struct output_queue_elem_s elem;

   elem.type='S';
   strncpy(elem.data, "[ok]", sizeof(elem.data)-1);
   elem.data[sizeof(elem.data)-1]=0;
   BaseType_t xRet=xQueueSend(outputTaskQueue,&elem,0);
   
   return xRet;
}


bool send_error(int err)
{
   char *str;
   switch(err) {
      case ERROR_BAD_REQUEST: str="[ERROR][bad request]"; break;
      case ERROR_BAD_ID: str="[ERROR][bad IDs]"; break;
      case ERROR_BAD_ACTION: str="[ERROR][bad action]"; break;
      default: str="[ERROR][unknown error]";
   }
   
   send_value_string(str);

   return false;
}


bool send_value_string(char *value)
{
   struct output_queue_elem_s elem;

   elem.type='S';
   strncpy(elem.data, value, sizeof(elem.data)-1);
   elem.data[sizeof(elem.data)-1]=0;
   BaseType_t xRet=xQueueSend(outputTaskQueue,&elem,0);
   
   return xRet;
}


bool serial_out(QueueHandle_t outputTaskQueue)
{
   struct output_queue_elem_s elem;
   BaseType_t xRet=xQueueReceive(
      outputTaskQueue,
      &elem,
      1000
   );
   if(xRet) {
      if(elem.type=='S') {
         printf("%s\n", elem.data);
      }
   }
}


void outputTask(void *pvParameters)
{
   QueueHandle_t _outputTaskQueue=(QueueHandle_t)pvParameters;
   for(;;) {
      serial_out(_outputTaskQueue);
   }
}


bool init_outputTask()
{
   outputTaskQueue = xQueueCreate(MAX_OUTPUT_QUEUE_SIZE, sizeof(struct output_queue_elem_s));
   if(outputTaskQueue) {
      BaseType_t xRet=xTaskCreate(outputTask, "OUTPUT", 1000, (void *)outputTaskQueue, 2, &outputTaskHandle);
      if(xRet) {
         DISPLAY("output task OK\n");
         return true;
      }
      else {
         DISPLAY("output task KO\n");
         return false;
      }
   }
   else {
      DISPLAY("can't create output queue\n");
      return false;
   }
}