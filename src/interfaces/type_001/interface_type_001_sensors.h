//
//  interface_type_001_sensors.h
//  mea-eDomus
//
//  Created by Patrice DIETSCH on 29/11/12.
//
//

#ifndef __interface_type_001_sensors_h
#define __interface_type_001_sensors_h

#include "mea_timer.h"
#include "interface_type_001.h"

struct sensor_s
{
   uint16_t sensor_id;
   char name[20];
   
   char arduino_pin_type;
   char arduino_pin;
   char arduino_function;
   
   char compute;
   char algo;
   
   unsigned int val;
   unsigned int last;

   compute_f compute_fn;
   
   float computed_val;
   
   mea_timer_t timer;
   
   uint32_t *nbtrap;
   uint32_t *nbxplout;
   
   int16_t todbflag;
};

void interface_type_001_sensors_free_queue_elem(void *d);
int16_t interface_type_001_sensors_process_traps(int16_t numTrap, char *data, int16_t l_data, void *userdata);

struct sensor_s *interface_type_001_sensors_valid_and_malloc_sensor(int16_t id_sensor_actuator, char *name, char *parameters);
mea_error_t interface_type_001_sensors_process_xpl_msg2(interface_type_001_t *i001, cJSON *xplMsgJson, char *device, char *type);
int16_t interface_type_001_sensors_poll_inputs2(interface_type_001_t *i001);
void interface_type_001_sensors_init(interface_type_001_t *i001);

#endif
