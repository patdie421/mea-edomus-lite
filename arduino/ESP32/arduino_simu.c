#include <stdio.h>

#include "arduino_simu.h"


int serial_available()
{
   return 1;
}

int serial_read()
{
   return getchar();
}
