#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>

#include "task_output.h"
#include "task_device.h"

#include "bleservers.h"


void deviceTask(void *pvParameters)
{
   int8_t deviceId=(int8_t)((long)pvParameters)&0xFF;
   struct device_queue_elem_s elem;

   meaBLEServers.meaDevices[deviceId].state=2; // running state

   printf("DEVICE: ");
   printf("%s", meaBLEServers.meaDevices[deviceId].address);
   printf(" is running\n");

   for(;;) {
      BaseType_t xRet=xQueueReceive(
         meaBLEServers.meaDevices[deviceId].queue,
         &elem,
         1000
      );
      if(xRet) {
         if(elem.action=='G') {
            send_value_string("[get]");
         }
         else if(elem.action=='S') {
            send_value_string("[set]");
         }
      }
      // do BLE coms
   }
}



