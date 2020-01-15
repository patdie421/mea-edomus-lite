#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define __USE_XOPEN
#include <time.h>
#undef __USE_XOPEN
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>

#define DEBUGFLAG 1

#include "mea_verbose.h"
#include "mea_string_utils.h"
#include "uthash.h"
#include "sunriset.h"
//DBSERVER #include "dbServer.h"
#include "datetimeServer.h"

#define ONESECONDNS 1000000000L

pthread_t *_timerServer_thread_id=NULL;

pthread_mutex_t timeServer_startTimer_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t timerServer_startTimer_cond = PTHREAD_COND_INITIALIZER;

time_t mea_time_value = 0;
time_t mea_sunrise_value = 0;
time_t mea_sunset_value = 0;
time_t mea_twilightstart_value = 0;
time_t mea_twilightend_value = 0;
struct tm mea_tm;

// gestion des dates et heures
enum datetime_type_e { DATETIME_TIME, DATETIME_DATE };
struct mea_datetime_value_s
{
   char dateTimeStr[20]; // format "AAAA/MM/JJ hh:mm:ss" ou "hh:mm:ss"
   enum datetime_type_e type;
   struct tm tm;
   time_t time_value;
   time_t last_access;

   UT_hash_handle hh;
};
struct mea_datetime_value_s *mea_datetime_values_cache = NULL;

// gestion des timers
struct mea_datetime_timer_s
{
   char   name[256];
   long   duration;
   enum   datetime_timer_unit_e unit;
   enum   datetime_timer_state_e state;
   struct timespec start;
   struct timespec end;
   datetime_timer_callback_f callback;
   void  *userdata;

   UT_hash_handle hh;
};
struct mea_datetime_timer_s *mea_datetime_timers_list = NULL;

void mea_getTime(struct timespec *t);

inline void mea_getTime(struct timespec *t)
{
#ifdef __linux__
   clock_gettime(CLOCK_REALTIME, t);
//   clock_gettime(CLOCK_MONOTONIC, t);
//   clock_gettime(CLOCK_MONOTONIC_COARSE, t);
#else
   struct timeval _t;
   gettimeofday(&_t, NULL);
   t->tv_sec = _t.tv_sec;
   t->tv_nsec = _t.tv_usec * 1000L;
#endif
}

void timespecAdd(struct timespec *t, struct timespec *t1, struct timespec *t2)
{
   t->tv_sec  = t1->tv_sec  + t2->tv_sec;
   t->tv_nsec = t1->tv_nsec + t2->tv_nsec;

   if(t->tv_nsec >= ONESECONDNS) {
      t->tv_sec  +=1;
      t->tv_nsec -=ONESECONDNS;
   }
   return;
}


void timespecDiff(struct timespec *t, struct timespec *t1, struct timespec *t2)
{
   t->tv_sec  = t1->tv_sec  - t2->tv_sec;
   t->tv_nsec = t1->tv_nsec - t2->tv_nsec;

   if(t->tv_nsec < 0) {
      t->tv_sec  -=1;
      t->tv_nsec +=ONESECONDNS;
   }
   return;
}


int timespecCmp(struct timespec *a, struct timespec *b)
{
   if (a->tv_sec != b->tv_sec) {
      if (a->tv_sec > b->tv_sec) {
         return 1;
      }
      else {
         return -1;
      }
   }
   else {
      if (a->tv_nsec > b->tv_nsec) {
         return 1;
      }
      else if (a->tv_nsec < b->tv_nsec) {
         return -1;
      }
      else {
         return 0;
      }
   }
}


time_t mea_datetime_sunrise()
{
   return mea_sunrise_value;
}


time_t mea_datetime_sunset()
{
   return mea_sunset_value;
}


time_t mea_datetime_twilightstart()
{
   return mea_twilightstart_value;
}


time_t mea_datetime_twilightend()
{
   return mea_twilightend_value;
}


time_t mea_datetime_time(time_t *v)
{
   if(v!=NULL) {
      *v=mea_time_value;
   }
   return mea_time_value;
}


struct tm *mea_localtime_r(const time_t *timep, struct tm *result)
{
   if(result) {
      memcpy(result, &mea_tm, sizeof(struct tm));
   }

   return &mea_tm;
}


int mea_timeFromStr(char *str, time_t *t)
{
   struct tm tm;
   struct mea_datetime_value_s *e = NULL;

   HASH_FIND_STR(mea_datetime_values_cache, str, e);

   if(e) {
      e->last_access = mea_time_value;
      *t = e->time_value;
      return 0;
   }
   else {
      memcpy(&tm, &mea_tm, sizeof(struct tm));
      char *p=strptime(str, "%H:%M:%S", &tm); // va juste remplacer les heures, minutes et secondes dans tm
      if(p!=NULL && *p==0) {
         struct mea_datetime_value_s *newTime=(struct mea_datetime_value_s *)malloc(sizeof(struct mea_datetime_value_s));
         if(newTime==NULL) {
            return -1;
         }

         strncpy(newTime->dateTimeStr, str, sizeof(newTime->dateTimeStr)-1);
         memcpy(&(newTime->tm), &tm, sizeof(struct tm));
         newTime->last_access = mea_time_value;
         newTime->time_value = mktime(&tm);
         newTime->type = DATETIME_TIME;
         *t =  newTime->time_value;
          
         HASH_ADD_STR(mea_datetime_values_cache, dateTimeStr, newTime); 

         return 0;
      }
      return -1;
   }
}


static int mea_clean_datetime_values_cache()
{
   if(mea_datetime_values_cache) {
      struct mea_datetime_value_s  *current, *tmp;

      HASH_ITER(hh, mea_datetime_values_cache, current, tmp) {
         if((mea_time_value - current->last_access) > 3600) {
            HASH_DEL(mea_datetime_values_cache, current);
            free(current);
         }
      }
   }

   return 0;
}


static int update_datetime_values_cache()
{
   struct tm tm;

   if(mea_datetime_values_cache) {
      struct mea_datetime_value_s  *current, *tmp;

      memcpy(&tm, &mea_tm, sizeof(struct tm));

      HASH_ITER(hh, mea_datetime_values_cache, current, tmp) {
         if(current->type == DATETIME_TIME) {
            strptime(current->dateTimeStr, "%H:%M:%S", &tm);
            memcpy(&(current->tm), &tm, sizeof(struct tm));
            current->time_value = mktime(&tm);
         }
      }
   }

   return 0;
}


static int getSunRiseSetOrTwilingStartEnd(double lon, double lat, time_t *_start, time_t *_end, int twilight)
{
   int year,month,day;
   double start, end;
   int  rs;

   struct tm tm_gmt;

   time_t t = mea_time_value;
   localtime_r(&t, &tm_gmt); // pour récupérer la date du jour (les h, m et s ne nous intéressent pas);

   // mise au format pour sunriset
   year  = tm_gmt.tm_year+1900;
   month = tm_gmt.tm_mon+1;
   day   = tm_gmt.tm_mday;

   if(twilight==0) {
      rs = sun_rise_set(year, month, day, lon, lat, &start, &end);
   }
   else {
      rs = civil_twilight(year, month, day, lon, lat, &start, &end);
   }

   int rh=0,rm=0,sh=0,sm=0;

   switch(rs) {
      case 0: {
         // conversion en heure/minute
         rh=(int)start;
         rm=(int)((start - (double)rh)*60);

         // conversion en time_t
         tm_gmt.tm_sec = 0;
         tm_gmt.tm_hour = rh;
         tm_gmt.tm_min = rm;
         t = timegm(&tm_gmt);
         *_start = t;
 
         DEBUG_SECTION {
            struct tm tm_local;
            localtime_r(&t, &tm_local);
            fprintf(stderr,"start(%d) : %02d:%02d\n", twilight, tm_local.tm_hour, tm_local.tm_min);
         }

         // conversion en heure/minute
         sh=(int)end;
         sm=(int)((end - (double)sh)*60);

         // conversion en time_t
         tm_gmt.tm_sec = 0;
         tm_gmt.tm_hour = sh;
         tm_gmt.tm_min = sm;
         t = timegm(&tm_gmt);
         *_end = t;
         
         DEBUG_SECTION {
            struct tm tm_local;
            localtime_r(&t, &tm_local);
            fprintf(stderr,"end(%d) :  %02d:%02d\n", twilight, tm_local.tm_hour, tm_local.tm_min);
         }
      }
      break;

      case +1:
         DEBUG_SECTION2(DEBUGFLAG) fprintf(stderr, "Sun above horizon\n");
         return -1;

      case -1:
         DEBUG_SECTION2(DEBUGFLAG) fprintf(stderr, "Sun below horizon\n");
         return -1;
   }

   return 0;
}


static void updateTimersStates()
{
   if(mea_datetime_timers_list!=NULL) {
      struct timespec now;
      mea_getTime(&now);

      struct mea_datetime_timer_s  *current, *tmp = NULL;

      HASH_ITER(hh, mea_datetime_timers_list, current, tmp) {
         if(current->state == TIMER_RUNNING) {
            if(timespecCmp(&(current->end), &now)==-1) {
               current->state = TIMER_FALLED;
               if(current->callback) {
                  current->callback(current->name, current->userdata); 
               }
            }
         }
      }
   }
}


static struct mea_datetime_timer_s * findNextFallingTimer()
{
   if(mea_datetime_timers_list!=NULL) {
      struct mea_datetime_timer_s  *current, *tmp, *last = NULL;

      HASH_ITER(hh, mea_datetime_timers_list, current, tmp) {
         if(current->state == TIMER_RUNNING) {
            if(last == NULL) {
               last = current;
            }
            else {
               if(timespecCmp(&(current->end), &(last->end))==1) {
                  last = current;
               }
            } 
         }
      }
      return last;
   }
   else {
      return NULL;
   }
}


static int _startAlarm(char *name, time_t date, datetime_timer_callback_f f, void *userdata)
{
   struct timespec now;
   struct mea_datetime_timer_s *e;
   int retour = 0;

   pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&timeServer_startTimer_lock);
   pthread_mutex_lock(&timeServer_startTimer_lock);

   HASH_FIND_STR(mea_datetime_timers_list, name, e);
   if(!e) {
      e = (struct mea_datetime_timer_s *)malloc(sizeof(struct mea_datetime_timer_s));
      if(!e) {
         retour = -1;
         goto startAlarm_clean_exit;   
      }

      mea_strncpylower(e->name, name, sizeof(e->name)-1);
      e->name[sizeof(e->name)-1]=0;
      e->duration = 0;
      e->unit = TIMER_DATE;
      e->state = 0;
      e->callback = f;
      e->userdata = userdata;

      HASH_ADD_STR(mea_datetime_timers_list, name, e);
   }

   mea_getTime(&now);
   e->start.tv_sec = now.tv_sec;
   e->start.tv_nsec = now.tv_nsec;

   e->end.tv_sec = date;
   e->end.tv_nsec = 0;

   e->state = TIMER_RUNNING; 

   pthread_cond_broadcast(&timerServer_startTimer_cond); // réveil du thread

startAlarm_clean_exit:
   pthread_mutex_unlock(&timeServer_startTimer_lock);
   pthread_cleanup_pop(0);

   return retour;
}


static int _startTimer(char *name, long duration, enum datetime_timer_unit_e unit, datetime_timer_callback_f f, void *userdata)
{
   struct mea_datetime_timer_s *e;
   struct timespec now;
   struct timespec _duration;
   int retour = 0;

   if(duration <= 0) {
      return -1;
   }

   switch(unit)
   {
      case TIMER_MIN:
         _duration.tv_sec = duration * 60;
         _duration.tv_nsec = 0;
         break;
      case TIMER_SEC:
         _duration.tv_sec = duration;
         _duration.tv_nsec = 0;
         break;
      case TIMER_CSEC:
         _duration.tv_sec = duration / 100;
         _duration.tv_nsec = (duration % 100) * 10000000L;
         break;
      case TIMER_MSEC:
         _duration.tv_sec = duration / 1000;
         _duration.tv_nsec = (duration % 1000) * 1000000L;
         break;
      default:
         return -1;
   }

   pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&timeServer_startTimer_lock);
   pthread_mutex_lock(&timeServer_startTimer_lock);

   HASH_FIND_STR(mea_datetime_timers_list, name, e);
   if(!e) {
      e = (struct mea_datetime_timer_s *)malloc(sizeof(struct mea_datetime_timer_s));
      if(!e) {
         retour = -1;
         goto startTimer_clean_exit;   
      }

      mea_strncpylower(e->name, name, sizeof(e->name)-1);
      e->name[sizeof(e->name)-1]=0;
      e->duration = duration;
      e->unit = unit;
      e->state = 0;
      e->callback = f;
      e->userdata = userdata;

      HASH_ADD_STR(mea_datetime_timers_list, name, e);
   }

   mea_getTime(&now);

   e->start.tv_sec = now.tv_sec;
   e->start.tv_nsec = now.tv_nsec;

   timespecAdd(&(e->end), &now, &_duration);

   e->state = TIMER_RUNNING; 

   pthread_cond_broadcast(&timerServer_startTimer_cond); // réveil du thread

startTimer_clean_exit:
   pthread_mutex_unlock(&timeServer_startTimer_lock);
   pthread_cleanup_pop(0);

   return retour;
}


int mea_datetime_getTimerState(char *name)
{
   struct mea_datetime_timer_s *e = NULL;

   HASH_FIND_STR(mea_datetime_timers_list, name, e);
   if(e) {
      return e->state;
   }

   return -1;
}


int mea_datetime_startAlarm(char *name, time_t date)
{
   return _startAlarm(name, date, NULL, NULL);
}


int mea_datetime_startAlarm2(char *name, time_t date, datetime_timer_callback_f f, void *userdata)
{
   return _startAlarm(name, date, f, userdata);
}


int mea_datetime_startTimer(char *name, long duration, enum datetime_timer_unit_e unit)
{
   return _startTimer(name, duration, unit, NULL, NULL);
}


int mea_datetime_startTimer2(char *name, long duration, enum datetime_timer_unit_e unit, datetime_timer_callback_f f, void *userdata)
{
   return _startTimer(name, duration, unit, f, userdata);
}


int mea_datetime_removeAllTimers()
{
   int retour = 0;

   struct mea_datetime_timer_s *e = NULL, *tmp = NULL;

   pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&timeServer_startTimer_lock);
   pthread_mutex_lock(&timeServer_startTimer_lock);

   HASH_ITER(hh, mea_datetime_timers_list, e, tmp) {
      HASH_DEL(mea_datetime_timers_list, e);
      free(e);
      e=NULL;
   }

   mea_datetime_timers_list=NULL;

   pthread_cond_broadcast(&timerServer_startTimer_cond);

stopTimer_clean_exit:
   pthread_mutex_unlock(&timeServer_startTimer_lock);
   pthread_cleanup_pop(0);

   return retour;
}


int mea_datetime_removeTimer(char *name)
{
   struct mea_datetime_timer_s *e;
   int ret = 0;
   pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&timeServer_startTimer_lock);
   pthread_mutex_lock(&timeServer_startTimer_lock);

   HASH_FIND_STR(mea_datetime_timers_list, name, e);
   if(e) {
      HASH_DEL(mea_datetime_timers_list, e);
      free(e);
      e=NULL;
   }
   else {
      ret=-1;
   }

   pthread_mutex_unlock(&timeServer_startTimer_lock);
   pthread_cleanup_pop(0);

   return ret;
}


int mea_datetime_stopTimer(char *name)
{
   int retour = 0;
   struct mea_datetime_timer_s *e;

   pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&timeServer_startTimer_lock);
   pthread_mutex_lock(&timeServer_startTimer_lock);

   HASH_FIND_STR(mea_datetime_timers_list, name, e);
   if(!e) {
      retour = -1;
      goto stopTimer_clean_exit;
   }
    e->state = TIMER_STOPPED; 

   pthread_cond_broadcast(&timerServer_startTimer_cond);

stopTimer_clean_exit:
   pthread_mutex_unlock(&timeServer_startTimer_lock);
   pthread_cleanup_pop(0);

   return retour;
}


void *_timeServer_thread(void *data)
{
   struct timespec te;
   struct timespec req;
   mea_time_value = time(NULL);
   int current_day = -1;

   time_t last_te_tv_sec = 0;
   long chrono_ns = 0;
   long sleep_time_ns = 0;
   struct timespec t;

   struct mea_datetime_timer_s *nextTimer = NULL;

   mea_getTime(&te);
   mea_time_value=te.tv_sec;
   localtime_r(&(mea_time_value), &mea_tm); // conversion en tm
   int hour_job_done = 0; 
   while(1) {
      mea_getTime(&te);

      updateTimersStates();

      if(last_te_tv_sec != te.tv_sec) { // toutes les secondes
         mea_time_value=te.tv_sec;
      }

      if((last_te_tv_sec != te.tv_sec) && (te.tv_sec % 60) == 0) { // toutes les minutes
//         DEBUG_SECTION2(DEBUGFLAG) {
//            mea_log_printf("%s (%s) : Traitement minute\n", INFO_STR, __func__);
//         }
         localtime_r(&(mea_time_value), &mea_tm); // conversion en tm
      }

      if((te.tv_sec % 3600) == 0) { // toutes les heures;
         if(hour_job_done == 0) { // pour pas begailler
            DEBUG_SECTION2(DEBUGFLAG) {
               mea_log_printf("%s (%s) : Traitement heure\n", INFO_STR, __func__);
            }
            mea_clean_datetime_values_cache(); // on fait le ménage dans le cache
/* DBSERVER
            start_consolidation_batch();
            start_purge_batch();
            start_resync_batch();
*/
            hour_job_done = 1;
         }
      }
      else
         hour_job_done=0;

      if(mea_tm.tm_wday != current_day) { // 1x par jour
         DEBUG_SECTION2(DEBUGFLAG) {
            mea_log_printf("%s (%s) : Traitement jour\n", INFO_STR, __func__);
         }
         current_day = mea_tm.tm_wday;

         update_datetime_values_cache(); // on remet à jour les heures pour la nouvelle journée

         // calcul des heures de couché et levé du soleil et +/- pénombre
         // Les coordonnées de Paris en degrés décimaux (constante à remplacer ...)
         double lat = 48.8534100;
         double lon = 2.3488000;
         time_t s=0, e=0;
         if(getSunRiseSetOrTwilingStartEnd(lon, lat, &s, &e,0)==0) {
            mea_sunrise_value = s;
            mea_sunset_value = e;
         }
         if(getSunRiseSetOrTwilingStartEnd(lon, lat, &s, &e,1)==0) {
            mea_twilightstart_value = s;
            mea_twilightend_value = e;
         }
      }

      last_te_tv_sec = te.tv_sec;
      chrono_ns = te.tv_nsec;

      // on se prépare à dormir en tenant compte du temps perdu pour les calculs
      mea_getTime(&te);

      #define SLEEPTIME_NS 1000000000L // une seconde
      // calcul du temps de sommeil
      if(chrono_ns <= te.tv_nsec)
         sleep_time_ns=SLEEPTIME_NS - (te.tv_nsec - chrono_ns);
      else
         sleep_time_ns=SLEEPTIME_NS - (ONESECONDNS + te.tv_nsec - chrono_ns);

      // un timer arrive-t-il a échéance avant le temps de sommeil calculé ?
      nextTimer=findNextFallingTimer();
      if(nextTimer) {
         timespecDiff(&t, &(nextTimer->end), &te);
         if(t.tv_sec == 0 && t.tv_nsec < sleep_time_ns) // oui, on ajuste le temps de sommeil en concéquence
            sleep_time_ns = t.tv_nsec;
      }

      req.tv_sec = te.tv_sec;
      req.tv_nsec = te.tv_nsec + sleep_time_ns;
      if(req.tv_nsec > ONESECONDNS) {
         ++(req.tv_sec);
         req.tv_nsec-=ONESECONDNS;
      }

      pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&timeServer_startTimer_lock);
      pthread_mutex_lock(&timeServer_startTimer_lock);

      int ret=pthread_cond_timedwait(&timerServer_startTimer_cond, &timeServer_startTimer_lock, &req);
      if(ret != 0) {
      }

      pthread_mutex_unlock(&timeServer_startTimer_lock);
      pthread_cleanup_pop(0);

      pthread_testcancel();
   }
   
//   pthread_exit(NULL);

   return NULL;
}


pthread_t *timeServer()
{
   pthread_t *timeServer_thread=NULL;

   timeServer_thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!timeServer_thread) {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ",FATAL_ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      goto timeServer_clean_exit;
   }
   
   if(pthread_create (timeServer_thread, NULL, _timeServer_thread, NULL)) {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : pthread_create - can't start thread - ", FATAL_ERROR_STR, __func__);
         perror("");
      }
      goto timeServer_clean_exit;
   }
   pthread_detach(*timeServer_thread);
   DEBUG_SECTION2(DEBUGFLAG) fprintf(stderr,"DATATIMESERVER : %x\n", (unsigned int)*timeServer_thread);

   if(timeServer_thread) {
      return timeServer_thread;
   }

timeServer_clean_exit:
   if(timeServer_thread) {
      free(timeServer_thread);
      timeServer_thread=NULL;
   }

   return NULL;
}


int start_timeServer()
{
   _timerServer_thread_id=timeServer();
   
   if(_timerServer_thread_id==NULL) {
      return -1;
   }

   return 0;
}
