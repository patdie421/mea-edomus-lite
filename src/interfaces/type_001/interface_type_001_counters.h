//
//  interface_type_001_counters.h
//  mea-eDomus
//
//  Created by Patrice DIETSCH on 29/11/12.
//
//

#ifndef __interface_type_001_counters_h
#define __interface_type_001_counters_h

#include <pthread.h>
#include <inttypes.h>

#include "interface_type_001.h"

#include "comio2.h"
#include "mea_timer.h"

// modélisation d'un compteur
struct electricity_counter_s
{
   char name[20];
   int16_t sensor_id;
   
   uint32_t wh_counter;
   uint32_t kwh_counter;
   uint32_t counter; // wh total
   
   double power; // estimation de la puissance instantanée
   double t; // time reférence
   
   uint32_t last_counter;
   double last_power;
   
   pthread_mutex_t lock;
   
   int16_t sensor_mem_addr[4]; // sensor data addr
   uint32_t trap; // comio trap number
   
   mea_timer_t timer;
   mea_timer_t trap_timer;
   
   // indicateur à traiter dans le trap
   uint32_t *nbtrap;
   uint32_t *nbxplout;
   
   int16_t todbflag;
};

struct electricity_counter_s
            *interface_type_001_sensors_valid_and_malloc_counter(int id_sensor_actuator, char *name, char *parameters);
void         interface_type_001_free_counters_queue_elem(void *d);
int16_t      interface_type_001_counters_process_traps(int16_t numTrap, char *buff, int16_t l_buff, void * args);
mea_error_t  interface_type_001_counters_process_xpl_msg2(interface_type_001_t *i001, cJSON *xplMsgJson, char *device, char *type);
int16_t      interface_type_001_counters_poll_inputs2(interface_type_001_t *i001);
void         interface_type_001_counters_init(interface_type_001_t *i001);

#endif
