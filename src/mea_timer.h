//
//  timer.h
//  mea-edomus
//
//  Created by Patrice Dietsch on 08/07/13.
//
//

#ifndef __mea_timer_h
#define __mea_timer_h

#include <time.h>
#include <inttypes.h>

typedef struct mea_timer_s
{
   uint16_t stat; // (0 = inactive, 1=active)
   time_t   start_time;
   uint32_t delay; // timer delay
   uint16_t autorestart; // (0 = no, 1=yes)
} mea_timer_t;
#define MEA_TIMER_S_INIT { .stat=0, .start_time=0, .delay=0, .autorestart=0 }

int16_t  mea_init_timer(mea_timer_t *aTimer, uint32_t aDelay, uint16_t restart_flag);
void     mea_start_timer(mea_timer_t *aTimer);
void     mea_stop_timer(mea_timer_t *aTimer);
int16_t  mea_test_timer(mea_timer_t *aTimer);

double   mea_now(void);
void     mea_nanosleep(uint32_t ns);
void     mea_microsleep(uint32_t usecs);

#endif
