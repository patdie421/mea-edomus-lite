//
//  monitoringServer.h
//  mea-edomus
//
//  Created by Patrice Dietsch on 25/09/2014.
//
//

#ifndef __processManager_h
#define __processManager_h

#include "mea_timer.h"

#include "mea_queue.h"

#define AUTO_INCREASE_MAX_PROCESSES 1
//#define NO_DATASEND 1

struct process_indicator_s
{
   char name[41];
   long value;
};

typedef int (*managed_processes_process_f)(int, void *, char *, int);

typedef enum process_status_e { STOPPED = 0, RUNNING = 1, RECOVERY_ERROR = 2 } process_status_t;
typedef enum process_type_e   { AUTOSTART = 0, NOTMANAGED = 1, TASK = 2, JOB = 3 } process_type_t;

#define DEFAULTGROUP 0


struct managed_processes_scheduling_data_s
{
   char *schedule_command_mem;
   char *schedule_command_strings_ptr[5];
   int16_t condition;
   time_t *last_exec;
};


struct managed_processes_process_s
{
   char name[41];
   char description[256];

   time_t last_heartbeat;
   int heartbeat_interval; // en second
   int heartbeat_status; // (0 = KO, 1 = OK)
   int heartbeat_counter;
   int heartbeat_wdcounter;
   int wdonoff;

   int group_id;
   int type; // 0 = autostart, 1 = notmanaged, 2 = task (start mais pas de stop, ...), 3 = job (traitement planifier à date)
   int status; // running=1, stopped=0, ...
   int async_stop;
   struct managed_processes_scheduling_data_s *scheduling_data;

   managed_processes_process_f stop;
   managed_processes_process_f start;
   void *start_stop_data;

   managed_processes_process_f heartbeat_recovery;
   void *heartbeat_recovery_data;
   int forced_watchdog_recovery_flag;

   mea_queue_t *indicators_list;
};


struct managed_processes_s
{
   int max_processes;
   struct managed_processes_process_s **processes_table;

   mea_timer_t scheduling_timer;
   mea_timer_t timer;

   pthread_rwlock_t rwlock;
};

extern struct managed_processes_s managed_processes;


//pthread_t *start_monitoringServer(char **parms_list);


struct managed_processes_s *get_managed_processes_descriptor(void);

int   process_register(char *name);
int   process_unregister(int id);
#ifdef QUEUE_ENABLE_INDEX
int   process_index_indicators_list(int id);
#endif
int   process_add_indicator(int id, char *name, long initial_value);
int   process_update_indicator(int id, char *name, long value);
int   process_get_indicator(int id, char *name, long *value);
int   process_heartbeat(int id);
int   process_start(int id, char *errmsg, int l_errmsg);
int   process_stop(int id, char *errmsg, int l_errmsg);
int   process_restart(int id, char *errmsg, int l_errmsg);
int   process_is_running(int id);
int   process_run_task(int id, char *errmsg, int l_errmsg);
int   process_async_stop(int id);

int   process_set_start_stop(int id,  managed_processes_process_f start, managed_processes_process_f stop, void *start_stop_data, int type);
int   process_set_watchdog_recovery(int id, managed_processes_process_f recovery_task, void *recovery_data);
int   process_set_status(int id, process_status_t status);
int   process_set_type(int id, process_type_t type);
int   process_set_group(int id, int group_id);
int   process_set_data(int id, void *data);
void *process_get_data_ptr(int id);
int   process_set_description(int id, char *description);
int   process_set_heartbeat_interval(int id, int interval);
int   process_job_set_scheduling_data(int id, char *str, int16_t condition);
int   process_wd_disable(int id, int flag);

int   process_forced_watchdog_recovery(int id);

int   init_processes_manager(int max_nb_processes);
int   clean_managed_processes(void);

int   managed_processes_processes_to_json(char *json, int l_json, int type);
int   managed_processes_processes_to_json_mini(char *json, int l_json, int type);
int   managed_processes_process_to_json(int id, char *message, int l_message);
int   managed_processes_indicators_list(char *json, int l_json);
/*
#ifndef NO_DATASEND
void  managed_processes_set_notification_hostname(char *hostname);
void  managed_processes_set_notification_port(int port);
int   managed_processes_send_stats_now(char *hostname, int port);
#endif
*/
int   managed_processes_check_heartbeat(int doRecovery);
int   managed_processes_loop(void);

#endif
