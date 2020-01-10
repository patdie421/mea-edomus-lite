#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
//#include <cstdlib.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "rtos_simu.h"
#include "arduino_simu.h"
#include "bleservers.h"

#include "task_device.h"
#include "task_output.h"
#include "task_input.h"

#define DEBUG 1


int setup()
{
   initBLEServers();
   init_outputTask();
   init_inputTask();
}


int loop()
{
   sleep(1);
}


int main(int argc, char *argv[])
{
   setup();

   while(1) {
      loop();
   }
}