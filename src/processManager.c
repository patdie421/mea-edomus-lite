#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include "processManager.h"

#include "mea_verbose.h"
#include "mea_string_utils.h"
#include "mea_timer.h"
#include "mea_queue.h"

#ifndef NO_DATASEND
#include "mea_sockets_utils.h"
#endif


struct managed_processes_s managed_processes;

void _indicators_free_queue_elem(void *e)
{
   if(e)
      free(e);
}

#ifdef QUEUE_ENABLE_INDEX
int _indicators_cmp(void **e1, void **e2)
{
   struct process_indicator_s *_e1 = (struct process_indicator_s *)*e1;
   struct process_indicator_s *_e2 = (struct process_indicator_s *)*e2;

   return strcmp(_e1->name, _e2->name);   
}
#endif

#ifndef NO_DATASEND
char *_notif_hostname=NULL;
int _notif_port=0;


void managed_processes_set_notification_hostname(char *hostname)
{
   if(hostname)
   {
     char *h=(char *)malloc(strlen(hostname)+1);
     strcpy(h, hostname);
     char *t=_notif_hostname;
     _notif_hostname=h;
     if(t)
       free(t);
   }
   else
   {
      char *t=_notif_hostname;
      _notif_hostname=NULL;
      if(t)
         free(t);
   }
}


void managed_processes_set_notification_port(int port)
{
   _notif_port=port;
}
#endif

int _indicator_exist(int id, char *name)
{
   int ret=-1;
   process_heartbeat(id);

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_rdlock(&managed_processes.rwlock);

   if(id>=0 &&
      id<managed_processes.max_processes &&
      managed_processes.processes_table[id] &&
      managed_processes.processes_table[id]->indicators_list )
   {
      struct process_indicator_s *e;

      if(mea_queue_first(managed_processes.processes_table[id]->indicators_list)==0)
      {
         while(1)
         {
            if(mea_queue_current(managed_processes.processes_table[id]->indicators_list, (void **)&e)==0)
            {
               if(strcmp(name, e->name) == 0)
               {
                  ret=0;
                  break;
               }
               else
                  mea_queue_next(managed_processes.processes_table[id]->indicators_list);
            }
            else
               break;
         }
      }
   }

   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0);

   return ret;
}


int16_t _match_scheduling_number(char *s, char num)
{
   // inspirer de http://www.cise.ufl.edu/~cop4600/cgi-bin/lxr/http/source.cgi/commands/simple/cron.c

   int n = 0;
   char c;

   if (strcmp(s, "*")==0)
      return num;

   while ((c = *s++) && (c >= '0') && (c <= '9'))
   {
      n = (n * 10) + c - '0';
   }
      
   switch (c)
   {
      case '\0':
         if(num == n)
            return n;
         else
            return -1; /*NOTREACHED*/
         break;

      case ',':
         if (num == n)
            return num;
         do
         {
            n = 0;
            while ((c = *s++) && (c >= '0') && (c <= '9'))
            {
               n = (n * 10) + c - '0';
            }
            if(num == n)
               return num;
         }
         while (c == ',');
         return -1; /*NOTREACHED*/
         break;

      case '-':
         if (num < n)
            return -1;
         n = 0;
         while ((c = *s++) && (c >= '0') && (c <= '9'))
         {
            n = (n * 10) + c - '0';
         }
         if(num<=n)
            return num;
         else
            return -1; /*NOTREACHED*/
         break;

      default:
         break;
   }
   return -2; /* SYNTAX ERROR */
}


// format "*|*|*|*|*" en gros comme crontab sauf que les " " (espace) sont remplacés par des "|" (c'est plus simple à traiter ...)
int process_job_set_scheduling_data(int id, char *str, int16_t condition)
{
   int ret=-1;
   struct managed_processes_scheduling_data_s *mps;

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);
  
   if(id>=0 &&
      id<managed_processes.max_processes &&
      managed_processes.processes_table[id] &&
      managed_processes.processes_table[id]->type == JOB)
   {
      if(managed_processes.processes_table[id]->scheduling_data==NULL)
      {
         managed_processes.processes_table[id]->scheduling_data=(struct managed_processes_scheduling_data_s *)malloc(sizeof(struct managed_processes_scheduling_data_s));
         if(managed_processes.processes_table[id]->scheduling_data==NULL)
         {
            ret=-1;
            goto process_job_set_scheduling_data_clean_exit;
         }
         managed_processes.processes_table[id]->scheduling_data->schedule_command_mem=NULL;
      }
      mps = managed_processes.processes_table[id]->scheduling_data;
   }
   else
   {
      ret=-1;
      goto process_job_set_scheduling_data_clean_exit;
   }
   
   if(mps->schedule_command_mem)
   {
      free(mps->schedule_command_mem);
      mps->schedule_command_mem=NULL;
   }

   mps->schedule_command_mem = (char *)malloc(strlen(str)+1);
   if(mps->schedule_command_mem==NULL)
   { 
      ret=-1;
      goto process_job_set_scheduling_data_clean_exit;
   }
   
   strcpy(mps->schedule_command_mem, str);
   char *s = mps->schedule_command_mem;

   mea_strremovespaces(s);
   char *tmp_ptr=realloc(mps->schedule_command_mem, strlen(s)+1);
   if(tmp_ptr==NULL)
   {
      free(mps->schedule_command_mem);
      mps->schedule_command_mem=NULL;
      ret=-1;
      goto process_job_set_scheduling_data_clean_exit;
   }
   else
      mps->schedule_command_mem=tmp_ptr;
   s = mps->schedule_command_mem;
   
   mps->schedule_command_strings_ptr[0]=s;
   
   int i=0,j=1;
   for(;s[i] && j<5;i++)
   {
      if(s[i]=='|')
      {
         s[i]=0;
         mps->schedule_command_strings_ptr[j++]=&(s[i+1]);
      }
   }

   if( _match_scheduling_number(mps->schedule_command_strings_ptr[0], 0)==-2 || // mm
       _match_scheduling_number(mps->schedule_command_strings_ptr[1], 0)==-2 || // hh
       _match_scheduling_number(mps->schedule_command_strings_ptr[2], 0)==-2 || // jj
       _match_scheduling_number(mps->schedule_command_strings_ptr[3], 0)==-2 || // MM
       _match_scheduling_number(mps->schedule_command_strings_ptr[4], 0)==-2 ) // DayOfWeek
   {
      free(mps->schedule_command_mem);
      mps->schedule_command_mem=NULL;
      ret=-1;
      goto process_job_set_scheduling_data_clean_exit;
   }

   mps->condition = condition;
   ret=0;
   
process_job_set_scheduling_data_clean_exit:

   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0);

   return ret;
}


void _managed_processes_process_jobs_scheduling()
{
   if(mea_test_timer(&(managed_processes.scheduling_timer))<0)
   {
      return;
   }

   time_t cur_time;
   struct tm *tm;

   time(&cur_time);
   tm = localtime(&cur_time);
   char errmsg[81];
   int16_t l_errormsg=80;
  
   int i=0; 
   for(;i<managed_processes.max_processes;i++)
   {
      if(managed_processes.processes_table[i] != NULL &&
         managed_processes.processes_table[i]->type==JOB)
      {
         struct managed_processes_scheduling_data_s *mps=managed_processes.processes_table[i]->scheduling_data;

         if( _match_scheduling_number(mps->schedule_command_strings_ptr[0], tm->tm_min) >=0 && // mm
             _match_scheduling_number(mps->schedule_command_strings_ptr[1], tm->tm_hour)>=0 && // hh
             _match_scheduling_number(mps->schedule_command_strings_ptr[2], tm->tm_mday)>=0 && // jj
             _match_scheduling_number(mps->schedule_command_strings_ptr[3], tm->tm_mon) >=0 && // MM
             _match_scheduling_number(mps->schedule_command_strings_ptr[4], tm->tm_wday)>=0 ) // DayOfWeek
         {
            if(managed_processes.processes_table[i]->status != RUNNING)
            {
               l_errormsg=80;
               process_run_task(i, errmsg, l_errormsg);
            }
         }
      }
   }
   time_t now;
   time(&now);

   mea_init_timer(&(managed_processes.scheduling_timer), 60 - now % 60, 0);
   mea_start_timer(&(managed_processes.scheduling_timer));
}


int managed_processes_indicators_list(char *message, int l_message)
{
   struct process_indicator_s *e;
   char buff[512];
   char json[2048];
   json[0]=0;

   if(mea_strncat(json, sizeof(json), "{ ")<0)
     return -1;

   int first_process=1;
   int i=0;
   for(;i<managed_processes.max_processes;i++)
   {
      if(managed_processes.processes_table[i] &&
         managed_processes.processes_table[i]->type!=TASK &&
         managed_processes.processes_table[i]->type!=JOB )
      {
         if(first_process==0)
         {
            if(mea_strncat(json,sizeof(json),",")<0)
               return -1;
         }
         int n=snprintf(buff,sizeof(buff),"\"%s\":",managed_processes.processes_table[i]->name);
         if(n<0 || n==sizeof(buff))
            return -1;
         if(mea_strncat(json,sizeof(json),buff)<0)
            return -1;

         if(mea_strncat(json, sizeof(json), "[")<0)
            return -1;

         if(mea_strncat(json, sizeof(json), "\"WDCOUNTER\"")<0)
            return -1;

         if(managed_processes.processes_table[i]->indicators_list &&
            mea_queue_first(managed_processes.processes_table[i]->indicators_list)==0)
         {
            while(1)
            {
               if(mea_queue_current(managed_processes.processes_table[i]->indicators_list, (void **)&e)==0)
               {
                  int n=snprintf(buff,sizeof(buff),",\"%s\"",e->name);
                  if(n<0 || n==sizeof(buff))
                     return -1;
                  if(mea_strncat(json,sizeof(json),buff)<0)
                     return -1;
                  mea_queue_next(managed_processes.processes_table[i]->indicators_list);
               }
               else
                  break;
            }
         }
         if(mea_strncat(json, sizeof(json), "]")<0)
            return -1;

         first_process=0;
      }
   }
   
   if(mea_strncat(json, sizeof(json), "}")<0)
     return -1;
     
   strcpy(message,"");
   if(mea_strncat(message,l_message,json)<0)
      return -1;

   return 0;
}


int _managed_processes_process_to_json(int id, char *s, int s_l, int flag)
{
   struct process_indicator_s *e;
   char buff[256];
   int n;
   
   s[0]=0;
   if(id>=0 &&
      id<managed_processes.max_processes &&
      managed_processes.processes_table[id])
   {
      if(mea_strncat(s, s_l, "{")<0)
         return -1;

      if(flag && managed_processes.processes_table[id]->status==RUNNING)
      {
         if(mea_strncat(s, s_l, "\"heartbeat\":")<0)
            return -1;
   
         if( managed_processes.processes_table[id]->heartbeat_status != 0)
         {
            if(mea_strncat(s, s_l, "\"OK\"")<0)
               return -1;
         }
         else
         {
            if(mea_strncat(s, s_l, "\"KO\"")<0)
               return -1;
         }
         
         if(mea_strncat(s, s_l, ",")<0)
            return -1;
      }
      
      if(managed_processes.processes_table[id]->description[0])
      {
         n=snprintf(buff,sizeof(buff),"\"desc\":\"%s\",",managed_processes.processes_table[id]->description);
         if(n<0 || n==sizeof(buff))
            return -1;
         if(mea_strncat(s, s_l, buff)<0)
            return -1;
      }
      
      n=snprintf(buff,sizeof(buff),"\"pid\":%d",id);
      if(n<0 || n==sizeof(buff))
         return -1;
      if(mea_strncat(s, s_l, buff)<0)
         return -1;

      n=snprintf(buff,sizeof(buff),",\"status\":%d",managed_processes.processes_table[id]->status);
      if(n<0 || n==sizeof(buff))
         return -1;
      if(mea_strncat(s, s_l, buff)<0)
         return -1;
      
      n=snprintf(buff,sizeof(buff),",\"WDCOUNTER\":%d",managed_processes.processes_table[id]->heartbeat_wdcounter);
      if(n<0 || n==sizeof(buff))
         return -1;
      if(mea_strncat(s, s_l, buff)<0)
         return -1;

      n=snprintf(buff,sizeof(buff),",\"type\":%d",managed_processes.processes_table[id]->type);
      if(n<0 || n==sizeof(buff))
         return -1;
      if(mea_strncat(s, s_l, buff)<0)
         return -1;

      n=snprintf(buff,sizeof(buff),",\"group\":%d",managed_processes.processes_table[id]->group_id);
      if(n<0 || n==sizeof(buff))
         return -1;
      if(mea_strncat(s, s_l, buff)<0)
         return -1;

      if( flag &&
          managed_processes.processes_table[id]->indicators_list &&
          mea_queue_first(managed_processes.processes_table[id]->indicators_list)==0 )
      {
         while(1)
         {
            if(mea_queue_current(managed_processes.processes_table[id]->indicators_list, (void **)&e)==0)
            {
               int n=snprintf(buff,sizeof(buff),",\"%s\":%ld",e->name,e->value);
               if(n<0 || n==sizeof(buff))
                  return -1;
               if(mea_strncat(s,s_l,buff)<0)
                  return -1;
               mea_queue_next(managed_processes.processes_table[id]->indicators_list);
            }
            else
               break;
         }
      }

      if(mea_strncat(s,s_l,"}")<0)
         return -1;
   }
   return 0;
}


int managed_processes_processes_to_json(char *message, int l_message)
{
   char buff[512];
   char json[2048];
   
   json[0]=0;
   
   if(mea_strncat(json, sizeof(json), "{ ")<0)
      return -1;
   int flag=0;
   int i=0;
   for(;i<managed_processes.max_processes;i++)
   {
      if(managed_processes.processes_table[i])
      {
         if(flag==1)
         {
            if(mea_strncat(json, sizeof(json), ", ")<0)
               return -1;
         }
         
         int n=snprintf(buff, sizeof(buff), "\"%s\":", managed_processes.processes_table[i]->name);
         if(n<0 || n==sizeof(buff))
            return -1;

         if(mea_strncat(json,sizeof(json),buff)<0)
            return -1;

         flag=1;
         if(_managed_processes_process_to_json(i, buff, sizeof(buff), 1)<0)
            return -1;
         if(mea_strncat(json,sizeof(json),buff)<0)
            return -1;
      }
   }
   if(mea_strncat(json,sizeof(json)," }\n")<0)
      return -1;

   strcpy(message,"");
   if(mea_strncat(message,l_message,json)<0)
      return -1;

   return 0;
}


int managed_processes_processes_to_json_mini(char *json, int l_json)
{
   char buff[512];
   json[0]=0;
   
   if(mea_strncat(json, l_json-1, "{ ")<0)
      return -1;
   int flag=0;
   int i=0;
   for(;i<managed_processes.max_processes;i++)
   {
      if(managed_processes.processes_table[i])
      {
         if(flag==1)
         {
            if(mea_strncat(json, l_json, ", ")<0)
               return -1;
         }
         int n=snprintf(buff,sizeof(buff),"\"%s\":",managed_processes.processes_table[i]->name);
         if(n<0 || n==sizeof(buff))
            return -1;

         if(mea_strncat(json,l_json-1,buff)<0)
            return -1;

         flag=1;
         if(_managed_processes_process_to_json(i, buff, sizeof(buff), 0)<0)
            return -1;
         if(mea_strncat(json,l_json-1,buff)<0)
            return -1;
      }
   }
   if(mea_strncat(json,l_json-1," }\n")<0)
      return -1;
   
   return 0;
}


int process_set_status(int id, process_status_t status)
{
   int ret=0;
   
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);
   
   if(id<0 ||
      id>=managed_processes.max_processes ||
      !managed_processes.processes_table[id])
      ret=-1;
   else
   {
      managed_processes.processes_table[id]->status=status;
   }
   
   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 

   return ret;
}


int process_set_heartbeat_interval(int id, int interval)
{
   int ret=0;
   
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);
   
   if(id<0 ||
      id>=managed_processes.max_processes ||
      !managed_processes.processes_table[id])
      ret=-1;
   else
   {
      managed_processes.processes_table[id]->heartbeat_interval=interval;
   }
   
   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0);

   return ret;
}


int process_wd_disable(int id, int flag)
{
   int ret=0;
   
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   if(id<0 ||
      id>=managed_processes.max_processes ||
      !managed_processes.processes_table[id])
      ret=-1;
   else
   {
      managed_processes.processes_table[id]->wdonoff=flag;
   }
   
   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 

   return ret;
}


int process_set_type(int id, process_type_t type)
{
   int ret=0;
   
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   if(id<0 ||
      id>=managed_processes.max_processes ||
      !managed_processes.processes_table[id])
      ret=-1;
   else
   {
      managed_processes.processes_table[id]->type=type;
   }
   
   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 

   return ret;
}


int process_set_group(int id, int group_id)
{
   int ret=0;
   
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   if(id<0 ||
      id>=managed_processes.max_processes ||
      !managed_processes.processes_table[id])
      ret=-1;
   else
   {
      managed_processes.processes_table[id]->group_id=group_id;
   }
   
   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 

   return ret;
}


int process_set_description(int id, char *description)
{
   int ret=0;
   
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   if(id<0 ||
      id>=managed_processes.max_processes ||
      !managed_processes.processes_table[id])
      ret=-1;
   else
   {
      strncpy(managed_processes.processes_table[id]->description, description, sizeof(managed_processes.processes_table[id]->description)-2);
      managed_processes.processes_table[id]->description[sizeof(managed_processes.processes_table[id]->description)-1]=0; // pour être sur d'avoir une chaine terminée
   }
   
   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0);

   return ret;
}


int process_set_watchdog_recovery(int id, managed_processes_process_f recovery_task, void *recovery_task_data)
{
   int ret=-1;
   
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   if(id<0 ||
      id>=managed_processes.max_processes ||
      !managed_processes.processes_table[id])
      ret=-1;
   else
   {
      managed_processes.processes_table[id]->heartbeat_recovery=recovery_task;
      managed_processes.processes_table[id]->heartbeat_recovery_data=recovery_task_data;
      ret=0;
   }

   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0);
   
   return ret;
}


int process_set_start_stop(int id, managed_processes_process_f start, managed_processes_process_f stop, void *start_stop_data, int auto_restart)
{
   int ret=0;
   
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   if(id<0 ||
      id>=managed_processes.max_processes ||
      !managed_processes.processes_table[id])
      ret=-1;
   else
   {
      managed_processes.processes_table[id]->status=STOPPED;
      managed_processes.processes_table[id]->start=start;
      managed_processes.processes_table[id]->stop=stop;
      managed_processes.processes_table[id]->start_stop_data=start_stop_data;
   }
   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 

   return ret;
}


#ifdef AUTO_INCREASE_MAX_PROCESSES
int16_t _processes_manager_increase_max_processes(int nb_more)
{
   struct managed_processes_process_s **old_table= managed_processes.processes_table;

   managed_processes.processes_table=(struct managed_processes_process_s **)realloc(managed_processes.processes_table, (managed_processes.max_processes + nb_more)*sizeof(struct managed_processes_process_s *));
   if(!managed_processes.processes_table)
   {
      managed_processes.processes_table = old_table;
      return -1;
   }
   else
   {
      int i=0;
      for(i=managed_processes.max_processes;i<managed_processes.max_processes+nb_more;i++)
      {
         managed_processes.processes_table[i]=NULL;
      }
      managed_processes.max_processes=managed_processes.max_processes+nb_more;
      return 0;
   }
}
#endif


int16_t _process_register(char *name)
{
   int16_t i=0;
   for(;i<managed_processes.max_processes;i++)
   {
      if(!managed_processes.processes_table[i])
      {
         managed_processes.processes_table[i]=(struct managed_processes_process_s *)calloc(1, sizeof(struct managed_processes_process_s));
         if(!managed_processes.processes_table[i])
         {
            return -1;
         }
         else
         {
            strncpy(managed_processes.processes_table[i]->name, name, sizeof(managed_processes.processes_table[i]->name)-1);
            managed_processes.processes_table[i]->description[0]=0;
            managed_processes.processes_table[i]->last_heartbeat=time(NULL);
            
            managed_processes.processes_table[i]->indicators_list=(mea_queue_t *)malloc(sizeof(mea_queue_t));
            if(!managed_processes.processes_table[i]->indicators_list)
            {
               free(managed_processes.processes_table[i]);
               managed_processes.processes_table[i]=NULL;
               
               return -1;
            }
            else
            {
               if(mea_queue_init(managed_processes.processes_table[i]->indicators_list)!=ERROR)
               {
#ifdef QUEUE_ENABLE_INDEX
                  mea_queue_create_index(managed_processes.processes_table[i]->indicators_list, _indicators_cmp);
#endif
               }
            }
            managed_processes.processes_table[i]->heartbeat_interval=20; // 20 secondes par defaut
            managed_processes.processes_table[i]->heartbeat_counter=0; // nombre d'abscence de heartbeat entre deux recovery.
            managed_processes.processes_table[i]->type=AUTOSTART;
            managed_processes.processes_table[i]->status=STOPPED; // arrêté par défaut
            managed_processes.processes_table[i]->async_stop=0;
            managed_processes.processes_table[i]->start=NULL;
            managed_processes.processes_table[i]->stop=NULL;
            managed_processes.processes_table[i]->start_stop_data=NULL;
            managed_processes.processes_table[i]->heartbeat_recovery=NULL;
            managed_processes.processes_table[i]->heartbeat_recovery_data=NULL;
            managed_processes.processes_table[i]->heartbeat_wdcounter=0;
            managed_processes.processes_table[i]->group_id=DEFAULTGROUP;
            managed_processes.processes_table[i]->forced_watchdog_recovery_flag=0;
            managed_processes.processes_table[i]->wdonoff=0;
            managed_processes.processes_table[i]->scheduling_data=NULL;
            return i;
         }
      }
   }
   return -2; // pas de place trouvé
}


int process_register(char *name)
{
   int16_t ret;

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   ret=_process_register(name);
   
   if(ret==-2) // pas trouvé de place
   {
#ifdef AUTO_INCREASE_MAX_PROCESSES
      if(_processes_manager_increase_max_processes(5)==0) // on essaye d'en rajouter
      {
         ret=_process_register(name);
         if(ret==-2)
            ret=-1;
      }
#else
      ret=-1
#endif
   }
   
register_process_clean_exit:
   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 

   return (int)ret;
}


int process_forced_watchdog_recovery(int id)
{
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   if(id>=0 &&
      id<managed_processes.max_processes &&
      managed_processes.processes_table[id])
   {
      managed_processes.processes_table[id]->forced_watchdog_recovery_flag=1;
   }
 
   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0);
   
   return 0;
}


int process_unregister(int id)
{
   int ret=-1;
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   if(id>=0 &&
      id<managed_processes.max_processes &&
      managed_processes.processes_table[id])
   {
      mea_queue_cleanup(managed_processes.processes_table[id]->indicators_list, _indicators_free_queue_elem);
      
      free(managed_processes.processes_table[id]->indicators_list);
      free(managed_processes.processes_table[id]);
      managed_processes.processes_table[id]=NULL;
      ret=0;
   }

   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 

   return ret;
}


int clean_managed_processes()
{
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   int i=0;
   for(;i<managed_processes.max_processes;i++)
      process_unregister(i);

   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 

   return 0;
}


int init_processes_manager(int max_nb_processes)
{
   managed_processes.processes_table=(struct managed_processes_process_s **)malloc(max_nb_processes * sizeof(struct managed_processes_process_s *));
   if(!managed_processes.processes_table)
   {
      return -1;
   }

   int i=0;
   for(;i<max_nb_processes;i++)
   {
      managed_processes.processes_table[i]=NULL;
   }

   managed_processes.max_processes = max_nb_processes;
   mea_init_timer(&managed_processes.timer, 5, 1);
   mea_start_timer(&managed_processes.timer);

   time_t now;
   time(&now);

   mea_init_timer(&managed_processes.scheduling_timer, 60 - now % 60, 0);
   mea_start_timer(&managed_processes.scheduling_timer);
   
   pthread_rwlock_init(&(managed_processes.rwlock), NULL);

   return 0;
}


int process_heartbeat(int id)
{
   int ret=-1;
   
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   if(id>=0 &&
      id<managed_processes.max_processes &&
      managed_processes.processes_table[id])
   {
      time_t now = time(NULL);
      ret = (int)(now - managed_processes.processes_table[id]->last_heartbeat);
      managed_processes.processes_table[id]->last_heartbeat = now;
   }

   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0);
   
   return ret;
}


int process_add_indicator(int id, char *name, long initial_value)
{
   struct process_indicator_s *e = NULL;

   if(id<0)
      return 0;

   process_heartbeat(id);

   if(_indicator_exist(id, name)==0)
      return 0;

   e=(struct process_indicator_s *)malloc(sizeof(struct process_indicator_s));
   if(e)
   {
      strncpy(e->name, name, 40);
      e->value=initial_value;
   }
   else
     return -1;

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   mea_queue_in_elem(managed_processes.processes_table[id]->indicators_list, e);
   
   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 

   return 0;
}


int process_del_indicator(int id, char *name)
{
   process_heartbeat(id);// à compléter

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   if(id>=0 &&
      id<managed_processes.max_processes &&
      managed_processes.processes_table[id])
   {
      struct process_indicator_s *e;

      if(mea_queue_first(managed_processes.processes_table[id]->indicators_list)==0)
      {
         while(mea_queue_current(managed_processes.processes_table[id]->indicators_list, (void **)&e)==0)
         {
            if(strcmp(name, e->name) == 0)
            {
               mea_queue_remove_current(managed_processes.processes_table[id]->indicators_list);
               goto process_del_indicator_clean_exit;
            }
            else
               mea_queue_next(managed_processes.processes_table[id]->indicators_list);
         }
      }
   }

process_del_indicator_clean_exit:   
   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 
   
   return 0;
}

#ifdef QUEUE_ENABLE_INDEX
int process_index_indicators_list(int id)
{

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   if(id>=0 &&
      id<managed_processes.max_processes &&
      managed_processes.processes_table[id])
   {
      mea_queue_recreate_index(managed_processes.processes_table[id]->indicators_list, 0);
   }

   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0);
   
   return 0;
}
#endif


int process_get_indicator(int id, char *name, long *value)
{
   int ret=-1;

   process_heartbeat(id);

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   if(id>=0 &&
      id<managed_processes.max_processes &&
      managed_processes.processes_table[id])
   {
      struct process_indicator_s *e;

#ifdef QUEUE_ENABLE_INDEX
      if(mea_queue_get_index_status(managed_processes.processes_table[id]->indicators_list)==1)
      {
         if(mea_queue_find_elem_using_index(managed_processes.processes_table[id]->indicators_list, (void *)name, (void *)&e)==0)
         {
            *value=e->value;
            ret=0;
         }
         goto process_get_indicator_clean_exit;
      }
#endif
      if(mea_queue_first(managed_processes.processes_table[id]->indicators_list)==0)
      {
         while(mea_queue_current(managed_processes.processes_table[id]->indicators_list, (void **)&e)==0)
         {
            if(strcmp(name, e->name) == 0)
            {
               *value=e->value;
               ret=0;
               goto process_get_indicator_clean_exit;
            }
            else
               mea_queue_next(managed_processes.processes_table[id]->indicators_list);
         }
      }
   }

process_get_indicator_clean_exit:
   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0);

   return ret;
}


int process_update_indicator(int id, char *name, long value)
{
   int ret=-1;
   
   process_heartbeat(id);

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);
   
   if(id>=0 &&
      id<managed_processes.max_processes &&
      managed_processes.processes_table[id])
   {
      struct process_indicator_s *e;

#ifdef QUEUE_ENABLE_INDEX
      if(mea_queue_get_index_status(managed_processes.processes_table[id]->indicators_list)==1)
      {
         if(mea_queue_find_elem_using_index(managed_processes.processes_table[id]->indicators_list, (void *)name, (void *)&e)==0)
         {
            e->value=value;
            ret=0;
         }
         goto process_update_indicator_clean_exit;
      }
#endif
      if(mea_queue_first(managed_processes.processes_table[id]->indicators_list)==0)
      {
         while(mea_queue_current(managed_processes.processes_table[id]->indicators_list, (void **)&e)==0)
         {
            if(strcmp(name, e->name) == 0)
            {
               e->value=value;
               ret=0;
               break;
            }
            else
               mea_queue_next(managed_processes.processes_table[id]->indicators_list);
         }
      }
   }

process_update_indicator_clean_exit:
   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0);

   return ret;
}


#ifndef NO_DATASEND
int managed_processes_send_stats_now(char *hostname, int port)
{
   int ret=-1;

   char json[4096];
   int sock;
   
   if(hostname)
   {
      pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
      pthread_rwlock_wrlock(&managed_processes.rwlock);

      if(managed_processes_processes_to_json(json, sizeof(json)-1)<-1)
      {
         ret=-1;
         goto managed_processes_send_stats_now_clean_exit; 
      }
   
      if(mea_socket_connect(&sock, hostname, port)<0)
      {
         ret=-1;
      }
      else
      {
         int l_data=(int)(strlen(json)+4); // 4 pour MON:
         char *message = (char *)alloca(l_data+12);

         sprintf(message,"$$$%c%cMON:%s###", (char)(l_data%128), (char)(l_data/128), json);
   
         ret = mea_socket_send(&sock, message, l_data+12);
         close(sock);
      }
      
      pthread_rwlock_unlock(&managed_processes.rwlock);
      pthread_cleanup_pop(0); 
   }

managed_processes_send_stats_now_clean_exit:
   return ret;
}


int _managed_processes_send_stats(char *hostname, int port)
{
   int ret=-1;
   
   if(hostname)
   {
      pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
      pthread_rwlock_rdlock(&managed_processes.rwlock);

      if(!mea_test_timer(&managed_processes.timer))
      {
         char json[4096];
         int sock;
      
         managed_processes_processes_to_json(json, sizeof(json)-1);
      
         if(mea_socket_connect(&sock, hostname, port)<0)
         {
            ret=-1;
         }
         else
         {
            int l_data=(int)(strlen(json)+4);
            char *message = (char *)alloca(l_data+12);
            sprintf(message,"$$$%c%cMON:%s###", (char)(l_data%128), (char)(l_data/128), json);
            ret = mea_socket_send(&sock, message, l_data+12);
      
            close(sock);
         }
      }
      
      pthread_rwlock_unlock(&managed_processes.rwlock);
      pthread_cleanup_pop(0); 
   }

   return ret;
}
#endif

int _managed_processes_processes_check_heartbeats(int doRecovery)
{
   int ret=0;
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   int i=0;
   for(;i<managed_processes.max_processes;i++)
   {
      if(managed_processes.processes_table[i] &&
         managed_processes.processes_table[i]->status==RUNNING )
      {
         time_t now = time(NULL);

         if(managed_processes.processes_table[i]->async_stop==1)
         {
            managed_processes.processes_table[i]->async_stop=0;
            managed_processes_process_f f=NULL;
            void *d=NULL;
            if(managed_processes.processes_table[i]->stop)
            {
               f=managed_processes.processes_table[i]->stop;
               d=managed_processes.processes_table[i]->start_stop_data;
               if(f)
                  ret=f(i, d, NULL, 0);
               managed_processes.processes_table[i]->status=0;
            }
         }
         else if((((now - managed_processes.processes_table[i]->last_heartbeat) < managed_processes.processes_table[i]->heartbeat_interval) && (managed_processes.processes_table[i]->forced_watchdog_recovery_flag == 0))
               || ( managed_processes.processes_table[i]->type == TASK)
               || ( managed_processes.processes_table[i]->type == JOB))
         {
            managed_processes.processes_table[i]->heartbeat_status=1;
            managed_processes.processes_table[i]->heartbeat_counter=0;
         }
         else if(managed_processes.processes_table[i]->wdonoff==0)
         {
            VERBOSE(5) mea_log_printf("%s  (%s) : watchdog process for %s\n",INFO_STR,__func__,managed_processes.processes_table[i]->name);
            managed_processes.processes_table[i]->heartbeat_counter++;
            managed_processes.processes_table[i]->heartbeat_status=0;
            if(doRecovery)
            {
               if(managed_processes.processes_table[i]->heartbeat_counter<=5)
               {
                  char errmsg[80];
                  if(managed_processes.processes_table[i]->heartbeat_recovery)
                  {
                     int ret=0;
                     
                     managed_processes.processes_table[i]->heartbeat_wdcounter++;
                     
                     pthread_cleanup_push( (void *)pthread_rwlock_wrlock, (void *)&managed_processes.rwlock ); // /!\ inversion par rapport à l'habitude ... lock en cas de fin de thread d'abord.
                     pthread_rwlock_unlock(&managed_processes.rwlock); // on delock
                     
                     VERBOSE(5) mea_log_printf("%s  (%s) : watchdog recovery started for %s\n", INFO_STR, __func__, managed_processes.processes_table[i]->name);

                     ret=managed_processes.processes_table[i]->heartbeat_recovery(i, managed_processes.processes_table[i]->heartbeat_recovery_data, errmsg, sizeof(errmsg));

                     VERBOSE(5) mea_log_printf("%s  (%s) : watchdog recovery done for %s (ret=%d)\n", INFO_STR, __func__, managed_processes.processes_table[i]->name, ret);

                     pthread_rwlock_wrlock(&managed_processes.rwlock); // on relock
                     pthread_cleanup_pop(0);

                     managed_processes.processes_table[i]->last_heartbeat = time(NULL);
                     managed_processes.processes_table[i]->forced_watchdog_recovery_flag=0;
                  }
                  else
                  {
                     VERBOSE(5) mea_log_printf("%s  (%s) : no watchdog recovery procedure for %s\n", INFO_STR, __func__, managed_processes.processes_table[i]->name);
                  }
               }
               else
               {
                  // traiter l'erreur
                  VERBOSE(5) mea_log_printf("%s  (%s) : watchdog recovery not started for %s, too mush restart\n", INFO_STR, __func__, managed_processes.processes_table[i]->name);
                  managed_processes.processes_table[i]->heartbeat_status=RECOVERY_ERROR;
               }
            }
         }
      }
   }
 
   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0);
   
   return ret;
}


int managed_processes_loop()
{
   int ret=0;
   _managed_processes_process_jobs_scheduling();
   ret=_managed_processes_processes_check_heartbeats(1);

#ifndef NO_DATASEND
   _managed_processes_send_stats(_notif_hostname, _notif_port);
#endif
 
  return ret; // -1 si un process en erreur definitif ... à traiter par le programme
}


int process_is_running(int id)
{
   int ret;
   
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_rdlock(&managed_processes.rwlock);

   if(id<0 || id>=managed_processes.max_processes)
   {
     ret=-1;
   }
   else
   {
      if(managed_processes.processes_table[id])
      {
         ret=managed_processes.processes_table[id]->status;
      }
      else
        ret=-1;
   }
   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 

   return ret;
}


int process_start(int id, char *errmsg, int l_errmsg)
{
   int ret=1;
   managed_processes_process_f f=NULL;
   void *d=NULL;
   
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   if(id>=0 &&
      id<managed_processes.max_processes &&
      managed_processes.processes_table[id] &&
      managed_processes.processes_table[id]->status==STOPPED &&
      managed_processes.processes_table[id]->type!=NOTMANAGED)
   {
      if(managed_processes.processes_table[id]->start)
      {
         f=managed_processes.processes_table[id]->start;
         d=managed_processes.processes_table[id]->start_stop_data;
      }
      else
         ret=-1;
   }
   else
      ret=-1;

   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 

   if(ret!=-1)
   {
      managed_processes.processes_table[id]->last_heartbeat = time(NULL);

      ret=f(id, d, errmsg, l_errmsg); // lancement du process

      pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
      pthread_rwlock_wrlock(&managed_processes.rwlock);
      
      if(managed_processes.processes_table[id])
      {
         if(ret<0)
            managed_processes.processes_table[id]->status=STOPPED;
         else
         {
            managed_processes.processes_table[id]->status=RUNNING;
         }
      }
      pthread_rwlock_unlock(&managed_processes.rwlock);
      pthread_cleanup_pop(0);
   }

   return ret;
}


int process_async_stop(int id)
{
   int ret=1;

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   if(id>=0 &&
      id<managed_processes.max_processes &&
      managed_processes.processes_table[id] &&
      managed_processes.processes_table[id]->status==RUNNING &&
      managed_processes.processes_table[id]->type!=NOTMANAGED)
   {
      managed_processes.processes_table[id]->async_stop=1;
   }
   else
      ret=-1;

   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0);
   
   return ret;
}


int process_restart(int id, char *errmsg, int l_errmsg)
{
   int ret=-1;
   ret=process_stop(id,errmsg, l_errmsg);
   if(ret==0)
      return process_start(id,errmsg, l_errmsg);
   else
      return ret;
}


struct task_thread_data_s
{
   int id;
   managed_processes_process_f task;
   void *task_data;
};


void *_task_thread(void *data)
{
   struct task_thread_data_s *task_thread_data=(struct task_thread_data_s *)data;
   
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);
   
   managed_processes.processes_table[task_thread_data->id]->status=RUNNING;
   
   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0);

#ifndef NO_DATASEND
   managed_processes_send_stats_now(_notif_hostname, _notif_port);
#endif

   task_thread_data->task(task_thread_data->id, task_thread_data->task_data, NULL, 0);
   
#ifndef NO_DATASEND
   managed_processes_send_stats_now(_notif_hostname, _notif_port);
#endif

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);
   
   managed_processes.processes_table[task_thread_data->id]->status=STOPPED;
   
   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0);
   
   if(data)
   {
      free(data);
      data=NULL;
   }
   
   return NULL;
}


int process_run_task(int id, char *errmsg, int l_errmsg)
{
   int ret=1;
 
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   if(id>=0 &&
      id<managed_processes.max_processes    &&
      managed_processes.processes_table[id] &&
      (managed_processes.processes_table[id]->type==TASK || managed_processes.processes_table[id]->type==JOB) )
   {
      if(managed_processes.processes_table[id]->start)
      {
         struct task_thread_data_s *task_thread_data=(struct task_thread_data_s *)malloc(sizeof(struct task_thread_data_s));
         task_thread_data->task=managed_processes.processes_table[id]->start;
         task_thread_data->task_data=managed_processes.processes_table[id]->start_stop_data;
         task_thread_data->id=id;
         pthread_t process_task_thread;
         if(pthread_create (&process_task_thread, NULL, _task_thread, (void *)task_thread_data))
         {
            VERBOSE(2) mea_log_printf("%s (%s) : pthread_create - can't start thread\n",ERROR_STR,__func__);
            if(errmsg)
            {
               snprintf(errmsg, l_errmsg, "internal error (pthread_create)");
               ret=-1;
               goto process_task_clean_exit;
            }
         }
         pthread_detach(process_task_thread);
         ret=0;
      }
   }

process_task_clean_exit:
   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 

   return ret;
}


int process_stop(int id, char *errmsg, int l_errmsg)
{
   int ret=1;
   managed_processes_process_f f=NULL;
   void *d=NULL;

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   if(id>=0 &&
      id<managed_processes.max_processes &&
      managed_processes.processes_table[id] &&
      managed_processes.processes_table[id]->status==RUNNING &&
      managed_processes.processes_table[id]->type!=NOTMANAGED)
   {
      if(managed_processes.processes_table[id]->stop)
      {
         f=managed_processes.processes_table[id]->stop;
         d=managed_processes.processes_table[id]->start_stop_data;
      }
      else
         ret=-1;
   }
   else
      ret=-1;

   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0);
   
   if(ret != -1)
      ret=f(id, d, errmsg, l_errmsg);
   
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   if(managed_processes.processes_table[id])
   {
      managed_processes.processes_table[id]->status=0;
   }
   
   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0);

   return ret;
}


int process_set_data(int id, void *data)
{
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   if(id>=0 &&
      id<managed_processes.max_processes &&
      managed_processes.processes_table[id])
   {
      managed_processes.processes_table[id]->start_stop_data=data;
   }

   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0);

   return 0;
}


void* process_get_data_ptr(int id)
{
   void *ptr=NULL;

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&managed_processes.rwlock );
   pthread_rwlock_wrlock(&managed_processes.rwlock);

   if(id>=0 &&
      id<managed_processes.max_processes &&
      managed_processes.processes_table[id])
   {
      ptr=managed_processes.processes_table[id]->start_stop_data;
   }

   pthread_rwlock_unlock(&managed_processes.rwlock);
   pthread_cleanup_pop(0); 

   return ptr;
}
