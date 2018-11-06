//
//  interface_type_001_actuators.h
//  mea-eDomus
//
//  Created by Patrice DIETSCH on 29/11/12.
//
//
#ifndef __interface_type_001_actuators_h
#define __interface_type_001_actuators_h

#include <string.h>

#include "cJSON.h"
#include "mea_timer.h"
#include "interface_type_001.h"

// modélisation d'un relai
struct actuator_s
{
   char name[20];
   int16_t actuator_id;
   
   char arduino_pin;
   char arduino_pin_type;
   
   int16_t old_val;
   
   int16_t todbflag;
};

void _interface_type_001_free_actuators_queue_elem(void *d);
struct actuator_s *interface_type_001_valid_and_malloc_actuator(int16_t id_sensor_actuator, char *name, char *parameters);
//mea_error_t xpl_actuator(interface_type_001_t *i001, xPL_NameValueListPtr ListNomsValeursPtr, char *device, char *type);
mea_error_t xpl_actuator2(interface_type_001_t *i001, cJSON *xplMsgJson, char *device, char *type);

#endif
