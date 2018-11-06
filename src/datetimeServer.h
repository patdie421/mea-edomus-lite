#ifndef __datetimeServer_h
#define __datetimeServer_h

enum datetime_timer_unit_e { TIMER_MIN, TIMER_SEC, TIMER_CSEC, TIMER_MSEC, TIMER_DATE=-1 };
enum datetime_timer_state_e { TIMER_RUNNING, TIMER_STOPPED, TIMER_FALLED };

typedef int (*datetime_timer_callback_f)(char *name, void *userdata);


time_t mea_datetime_time(time_t *v);
time_t mea_datetime_sunrise(void);
time_t mea_datetime_sunset(void);
time_t mea_datetime_twilightstart(void);
time_t mea_datetime_twilightend(void);

int mea_datetime_getTimerState(char *name);
int mea_datetime_startTimer(char *name, long duration, enum datetime_timer_unit_e unit);
int mea_datetime_startTimer2(char *name, long duration, enum datetime_timer_unit_e unit, datetime_timer_callback_f f, void *userdata);
int mea_datetime_startAlarm(char *name, time_t date);
int mea_datetime_startAlarm2(char *name, time_t date, datetime_timer_callback_f f, void *userdata);
      
int mea_datetime_stopTimer(char *name);
int mea_datetime_removeAllTimers(void);

void mea_getTime(struct timespec *t);

void timespecAdd(struct timespec *t, struct timespec *t1, struct timespec *t2);
void timespecDiff(struct timespec *t, struct timespec *t1, struct timespec *t2);
int  timespecCmp(struct timespec *a, struct timespec *b);

struct tm *mea_localtime_r(const time_t *timep, struct tm *result);

int mea_timeFromStr(char *str, time_t *t);

int start_timeServer(void);

#endif
