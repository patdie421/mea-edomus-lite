//
//  mea-eDomus
//
//  Created by Patrice Dietsch on 07/12/15.
//
//
//#define DEBUGFLAG 1

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#include "xPLServer.h"
#include "mea_verbose.h"
#include "globals.h"
#include "consts.h"
#include "mea_queue.h"
#include "tokens.h"
#include "configuration.h"

#include "processManager.h"

#include "automator.h"
#include "automatorServer.h"
#include "datetimeServer.h"

#if DEBUGFLAG_AUTOMATOR > 0
char *_automatorServer_fn = "";
char *_automatorEvalStrCaller = "";
char *_automatorEvalStrArg = "";
char _automatorEvalStrOperation = '0';
#endif

char *automator_server_name_str="AUTOMATORSERVER";
char *automator_input_exec_time_str="INEXECTIME";
char *automator_output_exec_time_str="OUTEXECTIME";
char *automator_xplin_str="XPLIN";
char *automator_xplout_str="XPLOUT";
char *automator_err_str="NBERR";

long automator_xplin_indicator=0;
long automator_xplout_indicator=0;

char *rules_file=NULL;

int16_t automator_send_all_inputs_flag = 0;

// globales pour le fonctionnement du thread
pthread_t *_automatorServer_thread_id=NULL;
int _automatorServer_thread_is_running=0;
int _automatorServer_monitoring_id=-1;

mea_queue_t *automator_msg_queue;
pthread_cond_t automator_msg_queue_cond;
pthread_mutex_t automator_msg_queue_lock;


void set_automatorServer_isnt_running(void *data)
{
   _automatorServer_thread_is_running=0;
}


char *getAutomatorRulesFile()
{
   return rules_file;
}


void setAutomatorRulesFile(char *file)
{
   if(rules_file) {
      free(rules_file);
      rules_file=NULL;
   }

   int l=(int)strlen(file); 
   rules_file=malloc(l+1);
   rules_file[l]=0;
  
   strncpy(rules_file, file, l);
}


mea_error_t automatorServer_add_msg(cJSON *msg_json)
{
   automator_msg_t *e=NULL;
   int ret=NOERROR;
   e=(automator_msg_t *)malloc(sizeof(automator_msg_t));
   if(!e) {
      return ERROR;
   }

   e->type=1;
   e->msg_json = msg_json;

   pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&automator_msg_queue_lock);
   pthread_mutex_lock(&automator_msg_queue_lock);
   if(automator_msg_queue) {
      mea_queue_in_elem(automator_msg_queue, e);
      if(automator_msg_queue->nb_elem>=1)
         pthread_cond_broadcast(&automator_msg_queue_cond);
   }
   else {
      cJSON_Delete(msg_json);
      ret=ERROR;
   }
   pthread_mutex_unlock(&automator_msg_queue_lock);
   pthread_cleanup_pop(0);

   if(ret!=ERROR) {
      return NOERROR;
   }
   
automatorServer_add_msg_clean_exit:
   if(e) {
      if(e->msg_json) {
         cJSON_Delete(e->msg_json);
         e->msg_json=NULL;
      }

      free(e);
      e=NULL;
   }
   return ERROR;
}


int automatorServer_send_all_inputs()
{
   automator_send_all_inputs_flag = 1;
   
   return 0;
}


int automatorServer_timer_wakeup(char *name, void *userdata)
{
   int retour=0;

   automator_msg_t *e=NULL;
   e=(automator_msg_t *)malloc(sizeof(automator_msg_t));
   if(!e) {
      return -1;
   }

   e->type=2;
   e->msg_json=NULL;

   pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&automator_msg_queue_lock);
   pthread_mutex_lock(&automator_msg_queue_lock);
   if(automator_msg_queue) {
      mea_queue_in_elem(automator_msg_queue, e);
      if(automator_msg_queue->nb_elem>=1)
         pthread_cond_broadcast(&automator_msg_queue_cond);
   }
   else {
      retour=-1;
   }
   pthread_mutex_unlock(&automator_msg_queue_lock);
   pthread_cleanup_pop(0);

   if(retour==0) {
      return 0;
   }

automatorServer_add_msg_clean_exit:
   if(e) {
      free(e);
      e=NULL;
   }
   return -1;
}


void *_automator_thread(void *data)
{
   int ret;
   
   automator_msg_t *e;
   
   pthread_cleanup_push( (void *)set_automatorServer_isnt_running, (void *)NULL );
   _automatorServer_thread_is_running=1;
   process_heartbeat(_automatorServer_monitoring_id);

   automator_xplin_indicator=0;
   automator_xplout_indicator=0;

   mea_timer_t indicator_timer;
   mea_init_timer(&indicator_timer, 5, 1);
   mea_start_timer(&indicator_timer);

   automator_init(rules_file);
   int16_t errcntr = 0;
   int err_indicator = 0;
   int16_t timeout;
  
   while(1) {
      ret=0;
      process_heartbeat(_automatorServer_monitoring_id);
      pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&automator_msg_queue_lock);
      pthread_mutex_lock(&automator_msg_queue_lock);

      if(mea_test_timer(&indicator_timer)==0) {
         process_update_indicator(_automatorServer_monitoring_id, automator_xplin_str, automator_xplin_indicator);
         process_update_indicator(_automatorServer_monitoring_id, automator_xplout_str, automator_xplout_indicator);
         process_update_indicator(_automatorServer_monitoring_id, automator_err_str, err_indicator);
      }
   
      timeout=0;
      if(automator_msg_queue && automator_msg_queue->nb_elem==0) {
         struct timeval tv;
         struct timespec ts;
         gettimeofday(&tv, NULL);

         long ns_timeout=1000 * 1000 * 250; // 250 ms en nanoseconde
//         long ns_timeout=1000000000L; // 1s
         ts.tv_sec = tv.tv_sec;
         ts.tv_nsec = (tv.tv_usec * 1000) + ns_timeout;
         if(ts.tv_nsec>1000000000L) { // 1.000.000.000 ns = 1 seconde
            ts.tv_sec++;
            ts.tv_nsec=ts.tv_nsec - 1000000000L;
         }
//         ts.tv_sec+=5; // for debug only
         ret=pthread_cond_timedwait(&automator_msg_queue_cond, &automator_msg_queue_lock, &ts);
         if(ret!=0) {
            if(ret==ETIMEDOUT) {
               errcntr=0; // c'est pas vraiment une erreur on clear le compteur d'erreur
               timeout=1;
               ret=0;
            }
            else if(ret==EINVAL) {
               ++err_indicator;
               DEBUG_SECTION2(DEBUGFLAG) {
                  mea_log_printf("%s (%s) : pthread_cond_timedwait EINVAL error (EINVAL)\n", DEBUG_STR, __func__);
               }
            }
            else {
               ++err_indicator;
               ++errcntr;
               // autres erreurs à traiter
               VERBOSE(2) {
                  mea_log_printf("%s (%s) : pthread_cond_timedwait error (%d) - ", DEBUG_STR, __func__, ret);
                  perror("");
               }

               if(errcntr > 10) { // 10 erreurs d'affilé, on va pas s'en sortir => on s'arrête proprement
                  process_async_stop(_automatorServer_monitoring_id);
                  for(;;) sleep(1);
               }

               sleep(1); // on attend un peu, on va peut-être pouvoir se reprendre
            }
         }
         else {
            errcntr=0;
         }
      }
     
      e=NULL;
      if (ret==0 && automator_msg_queue && !timeout) {// pas d'erreur, on récupère un élément dans la queue
         ret=mea_queue_out_elem(automator_msg_queue, (void **)&e);
      }
      else {
         ret=-1;
      }
      
      pthread_mutex_unlock(&automator_msg_queue_lock);
      pthread_cleanup_pop(0);

      process_heartbeat(_automatorServer_monitoring_id); 

      if (timeout==1) { // timeout => un tour d'automate (à vide, càd sans message xPL)
         pthread_testcancel();
         
         automator_matchInputsRules(_inputs_rules, NULL);
         int n= automator_playOutputRules(_outputs_rules);
         if(n>0) {
            automator_xplout_indicator+=n;
         }

         if(automator_send_all_inputs_flag!=0) {
            automator_send_all_inputs();
            automator_send_all_inputs_flag=0;
         }

         automator_reset_inputs_change_flags();
         continue;
      }
     
      if(!ret && e) { // on a sortie un élément de la queue
         if(e->type==1) {
//            DEBUG_SECTION2(DEBUGFLAG) displayXPLMsg(e->msg);
         }
/*
         else if(e->type == 2) {
            DEBUG_SECTION2(DEBUGFLAG) mea_log_printf("%s (%s) : timer wakeup signal\n", INFO_STR, __func__);
         }
*/
/* For DEBUG
         if(e->type == 1 && e->msg_json) {
            char *tmp = cJSON_Print(e->msg_json);
            mea_log_printf("xpl:%s\n", tmp);
            free(tmp);
         }
         else
            mea_log_printf("e:%p type:%d\n", e, e->type);
*/ 
         automator_matchInputsRules(_inputs_rules, e->msg_json);

         int n=automator_playOutputRules(_outputs_rules);
         if(n>0) {
            automator_xplout_indicator+=n;
         }

         if(automator_send_all_inputs_flag!=0) {
            automator_send_all_inputs();
            automator_send_all_inputs_flag=0;
         }

         automator_reset_inputs_change_flags();

         if(e) {
            if(e->type == 1) { // l'élément sorti était un message xpl (pas un timer)
               automator_xplin_indicator++;
               if(e->msg_json) {
                  cJSON_Delete(e->msg_json);
                  e->msg_json=NULL;
               }
            }
            free(e);
            e=NULL;
         }
      }
      else {
         // pb d'accés aux données de la file
         VERBOSE(9) mea_log_printf("%s (%s) : mea_queue_out_elem - can't access queue element\n", ERROR_STR, __func__);
      }
      
      pthread_testcancel();
   }
   
   pthread_cleanup_pop(1);

   pthread_exit(NULL);
   
   return NULL;
}


pthread_t *automatorServer()
{
   pthread_t *automator_thread=NULL;

   automator_msg_queue=(mea_queue_t *)malloc(sizeof(mea_queue_t));
   if(!automator_msg_queue) {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      return NULL;
   }
   mea_queue_init(automator_msg_queue);
   pthread_mutex_init(&automator_msg_queue_lock, NULL);
   pthread_cond_init(&automator_msg_queue_cond, NULL);
 
   automator_thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!automator_thread) {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ",FATAL_ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      goto automatorServer_clean_exit;
   }
   
   if(pthread_create (automator_thread, NULL, _automator_thread, NULL)) {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : pthread_create - can't start thread - ", FATAL_ERROR_STR, __func__);
         perror("");
      }
      goto automatorServer_clean_exit;
   }
   fprintf(stderr,"AUTOMATORSERVER : %x\n", (unsigned int)*automator_thread);
   pthread_detach(*automator_thread);

   if(automator_thread)   
      return automator_thread;

automatorServer_clean_exit:
   if(automator_thread) {
      free(automator_thread);
      automator_thread=NULL;
   }

   if(automator_msg_queue) {
      free(automator_msg_queue);
      automator_msg_queue=NULL;
   }

   return NULL;
}


void automator_msg_queue_free_queue_elem(void *d)
{
   automator_msg_t *e=(automator_msg_t *)d;
   if(e) {
      if(e->msg_json) {
         cJSON_Delete(e->msg_json);
         e->msg_json=NULL;
      }

      free(e);
   }
} 


int stop_automatorServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   if(_automatorServer_thread_id) {
      pthread_cancel(*_automatorServer_thread_id);
      int counter=100;
      while(counter--) {
         if(_automatorServer_thread_is_running) {
            usleep(100);
         }
         else {
            break;
         }
      }
      DEBUG_SECTION2(DEBUGFLAG) mea_log_printf("%s (%s) : %s, fin après %d itération(s)\n",DEBUG_STR, __func__, automator_server_name_str, 100-counter);
      
      free(_automatorServer_thread_id);
      _automatorServer_thread_id=NULL;
   }

   pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&automator_msg_queue_lock);
   pthread_mutex_lock(&automator_msg_queue_lock);
   if(automator_msg_queue) {
      mea_queue_cleanup(automator_msg_queue, automator_msg_queue_free_queue_elem);
      free(automator_msg_queue);
      automator_msg_queue=NULL;
   }
   pthread_mutex_unlock(&automator_msg_queue_lock);
   pthread_cleanup_pop(0);

   _automatorServer_monitoring_id=-1;

   VERBOSE(1) mea_log_printf("%s (%s) : %s %s.\n", INFO_STR, __func__, automator_server_name_str, stopped_successfully_str);

   return 0;
}


int start_automatorServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   struct automatorServer_start_stop_params_s *automatorServer_start_stop_params = (struct automatorServer_start_stop_params_s *)data;

   char err_str[80], notify_str[256];

   if(appParameters_get("RULESFILE", automatorServer_start_stop_params->params_list)) {
      setAutomatorRulesFile(appParameters_get("RULESFILE", automatorServer_start_stop_params->params_list));
      
      _automatorServer_thread_id=automatorServer();
      if(_automatorServer_thread_id==NULL) {
         strerror_r(errno, err_str, sizeof(err_str));
         VERBOSE(1) {
            mea_log_printf("%s (%s) : can't start %s (thread error) - %s\n", ERROR_STR, __func__, automator_server_name_str, notify_str);
         }

         return -1;
      }
      _automatorServer_monitoring_id=my_id;
   }
   else {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : can't start %s (incorrect rules path).\n", ERROR_STR, __func__, automator_server_name_str);
      }
      return -1;
   }

   VERBOSE(2) mea_log_printf("%s (%s) : %s %s.\n", INFO_STR, __func__, automator_server_name_str, launched_successfully_str);

   return 0;
}


int restart_automatorServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   int ret=0;
   ret=stop_automatorServer(my_id, data, errmsg, l_errmsg);
   if(ret==0) {
      return start_automatorServer(my_id, data, errmsg, l_errmsg);
   }
   return ret;
}