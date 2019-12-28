//
//  main.c
//
//  Created by Patrice DIETSCH on 08/07/12.
//  Copyright (c) 2012 -. All rights reserved.
//
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <inttypes.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <libgen.h>
#include <sys/wait.h>
#include <execinfo.h>
#include <sys/resource.h>

#include "globals.h"
#include "macros.h"
#include "consts.h"

#include "tokens.h"
#include "tokens_da.h"

#include "mea_verbose.h"
#include "mea_queue.h"
#include "mea_string_utils.h"
#include "mea_sockets_utils.h"
#include "mea_timer.h"
#include "mea_json_utils.h"

#include "parameters_utils.h"
#include "configuration.h"

#include "processManager.h"

#include "users.h"
#include "datetimeServer.h"
#include "xPLServer.h"
#include "pythonPluginServer.h"
#include "httpServer.h"
#include "automatorServer.h"
#include "interfacesServer.h"

#define MEA_PATH               1
#define VERBOSELEVEL          12

// id des process
int xplServer_monitoring_id=-1;
int httpServer_monitoring_id=-1;
int pythonPluginServer_monitoring_id=-1;
int automatorServer_monitoring_id=-1;
int log_rotation_id=-1;
int main_monitoring_id = -1;

pthread_t *monitoringServer_thread=NULL;   /*!< Adresse du thread de surveillance interne. Variable globale car doit être accessible par les gestionnaires de signaux.*/

long sigsegv_indicator = 0;

pid_t automator_pid = 0;


void usage(char *cmd)
/**
 * \brief     Affiche les "usages" de mea-edomus
 * \param     cmd    nom d'appel de mea-edomus (argv[0])
 */
{
   char *usage_text[]={
     //12345678901234567890123456789012345678901234567890123456789012345678901234567890
      "",
      "gestion de capteurs/actionneurs (xbee, arduino, ...) commandés par des messages",
      "xPL",
      "",
      "Options de lancement de l'application : ",
      "  --basepath, -p      : chemin de l'installation",
      "  --apiport, -g       : port tcp du serveur HTTP",
      "  --verboselevel, -v  : niveau de détail des messages d'erreur (1 à 9)",
      "  --help, -h          : ce message",
      "",
      NULL
   };
   
   if(!cmd)
      cmd="mea-edomus";
   printf("\nusage : %s [options de lancemement]\n", cmd);
   for(int16_t i=0;usage_text[i];i++)
      printf("%s\n",usage_text[i]);
}


int logfile_rotation_job(int my_id, void *data, char *errmsg, int l_errmsg)
{
   char *log_file_name=(char *)data;

   if(log_file_name) {
      mea_log_printf("%s (%s) : log file rotation job start\n", INFO_STR, __func__);

      if(mea_rotate_open_log_file(log_file_name, 6)<0) {
         VERBOSE(2) fprintf (MEA_STDERR, "%s (%s) : can't rotate %s - ", ERROR_STR, __func__, log_file_name);
         perror("");
         return -1;
      }

      mea_log_printf("%s (%s) : log file rotation job done\n", INFO_STR, __func__);
   }
   return 0;
}



int16_t read_all_application_parameters(char *cfgfile)
{
   cJSON *p=loadJson_alloc(cfgfile);

   if(p) {
      cJSON *c = p->child;
      while(c) {
         if(c->type == cJSON_String) {
            appParameters_set(c->string, c->valuestring, NULL);
	      }
         c = c->next;
      }
      cJSON_Delete(p);
      return 0;
   }

   return -1;
}


void clean_all_and_exit(int code)
{
   if(xplServer_monitoring_id!=-1) {
      process_stop(xplServer_monitoring_id, NULL, 0);
      process_unregister(xplServer_monitoring_id);
      xplServer_monitoring_id=-1;
   }

   if(httpServer_monitoring_id!=-1) {
      process_stop(httpServer_monitoring_id, NULL, 0);
      process_unregister(httpServer_monitoring_id);
      httpServer_monitoring_id=-1;
   }
  

   stop_interfaces();


   if(pythonPluginServer_monitoring_id!=-1) {
      process_stop(pythonPluginServer_monitoring_id, NULL, 0);
      process_unregister(pythonPluginServer_monitoring_id);
      pythonPluginServer_monitoring_id=-1;
   }

   if(httpServer_monitoring_id!=-1) {
      process_stop(httpServer_monitoring_id, NULL, 0);
      process_unregister(httpServer_monitoring_id);
      httpServer_monitoring_id=-1;
   }

   if(automatorServer_monitoring_id!=-1) {
      process_stop(automatorServer_monitoring_id, NULL, 0);
      process_unregister(automatorServer_monitoring_id);
      automatorServer_monitoring_id=-1;
   }
   
   process_unregister(main_monitoring_id);

   appParameters_clean(); 

   parsed_parameters_clean_all(1);
   release_strings_da();
   release_tokens();

   exit(code);
}


static void error_handler(int signal_number)
{
/*
   void *array[10];
   char **messages;

   size_t size;

   size = backtrace(array, 50);

   // print out all the frames to stderr
   fprintf(stderr, "Error: signal %d:\n", signal_number);

   backtrace_symbols_fd(array, size, fileno(stderr));

   messages = backtrace_symbols(array, size);

   for (int i=1; i < size && messages != NULL; ++i)
      fprintf(stderr, "[backtrace]: (%d) %s\n", i, messages[i]);
   free(messages);
*/
   ++sigsegv_indicator;

   fprintf(stderr, "Error: signal %d:\n", signal_number);
   if((_xPLServer_thread_id!=NULL) && pthread_equal(*_xPLServer_thread_id, pthread_self())!=0) {
      fprintf(stderr, "Error: in xPLServer, try to recover\n");
   }

   else if((_automatorServer_thread_id!=NULL) && pthread_equal(*_automatorServer_thread_id, pthread_self())!=0) {
      fprintf(stderr, "Error: in automator.c/automatorServer.c\n");
   }

   fprintf(stderr, "Error: aborting\n");
   fprintf(stderr, "Thread id : %x\n", (unsigned int)pthread_self());
//   abort();
   exit(1);
}


static void _signal_STOP(int signal_number)
/**
 * \brief     Traitement des signaux d'arrêt (SIGINT, SIGQUIT, SIGTERM)
 * \details   L'arrêt de l'application se déclanche par l'émission d'un signal d'arrêt. A la réception de l'un de ces signaux
 *            Les différents process de mea_edomus doivent être arrêtés.
 * \param     signal_number  numéro du signal (pas utilisé mais nécessaire pour la déclaration du handler).
 */
{
   VERBOSE(9) mea_log_printf("%s (%s) : Stopping mea-edomus requested (signal = %d).\n", INFO_STR, __func__, signal_number);

   clean_all_and_exit(0);
}


static void _signal_SIGCHLD(int signal_number)
{
   /* Wait for all dead processes.
    * We use a non-blocking call to be sure this signal handler will not
    * block if a child was cleaned up in another part of the program. */
    while (waitpid(-1, NULL, WNOHANG) > 0) {
    }
}


int main(int argc, const char * argv[])
/**
 * \brief     Point d'entrée du mea-edomus
 * \details   Intitialisation des structures de données et lancement des différents "process" (threads) de l'application
 * \param     argc   paramètres de lancement de l'application.
 * \param     argv   nombre de paramètres.
 * \return    1 en cas d'erreur, 0 sinon
 */
{
   int ret=0;

   // activation du core dump
   struct rlimit core_limit = { RLIM_INFINITY, RLIM_INFINITY };
   assert( setrlimit( RLIMIT_CORE, &core_limit ) == 0 );

   // pour changer l'endroit ou sont les cores dump et donner un nom plus explicite
   // echo '/tmp/core_%e.%p' | sudo tee /proc/sys/kernel/core_pattern

   signal(SIGSEGV, error_handler);

   // toutes les options possibles
   static struct option long_options[] = {
      {"basepath",          required_argument, 0,  MEA_PATH             }, // 'p'
      {"verboselevel",      required_argument, 0,  VERBOSELEVEL         }, // 'v'
      {"help",              no_argument,       0,  'h'                  }, // 'h'
      {0,                   0,                 0,  0                    }
   };

#define __MEA_DEBUG_ON__ 1
#ifdef __MEA_DEBUG_ON__
   debug_on();
   set_verbose_level(10);
#else
   debug_off();
   set_verbose_level(2);
#endif

   //
   // initialisation
   //
   //
   init_tokens();
   init_strings_da();
   parsed_parameters_init();


   // chargement des parametres par defaut
   appParameters_init();

   // XCODE
   // pour debugger sans être interrompu par les SIGPIPE
   // rajouter ici un breakpoint, l'éditer et rajouter une action "Command debugger" avec la ligne suivante :
   // process handle SIGPIPE -n true -p true -s false
   // cocher ensuite "Automatically continue after evaluating"
   

   // initialisation de la liste des parametres à NULL sauf MEAPATH
   //
   if(!appParameters_get(MEAPATH_STR_C, NULL))
      appParameters_set(MEAPATH_STR_C,"/usr/local/mea-edomus", NULL);


   //
   // récupération des autres paramètres de la ligne de commande
   //
   int16_t _v=-1;
   int option_index = 0; // getopt_long function need int
   int c = 0; // getopt_long function need int
//   cJSON *cmdlineParameters = appParameters_create();

   while ((c = getopt_long(argc, (char * const *)argv, "hp:v:g:", long_options, &option_index)) != -1)
   {
      switch (c) {
         case 'h':
            usage((char *)argv[0]);
            exit(0);

         case 'p':
	 case MEA_PATH:
            appParameters_set(MEAPATH_STR_C, optarg, NULL);
            break;

         case 'v':
         case VERBOSELEVEL:
	    appParameters_set("VERBOSELEVEL", optarg, NULL);
            _v=atoi(optarg);
            break;

         default:
            VERBOSE(1) mea_log_printf("%s (%s) : Paramètre \"%s\" inconnu.\n",ERROR_STR,__func__,optarg);
            usage((char *)argv[0]);
            clean_all_and_exit(1);
      }
   }

   //
   // Contrôle des parametres
   //
   if(_v > 0 && _v < 10)
         set_verbose_level(_v);

   // lecture de tous les paramètres de l'application
#ifdef __linux__
   appParameters_set("INTERFACE", "eth0", NULL);
#else
   appParameters_set("INTERFACE", "en0", NULL);
#endif

   char cfgfile[1024];
   snprintf(cfgfile,sizeof(cfgfile)-1, "%s/etc/mea-edomus.json", appParameters_get(MEAPATH_STR_C, NULL));
   ret = read_all_application_parameters(cfgfile);

   if(ret) {
      VERBOSE(1) mea_log_printf("%s (%s) : can't load parameters (%s)\n",ERROR_STR,__func__,cfgfile);
      clean_all_and_exit(1);
   }


   //
   // stdout et stderr vers fichier log
   //
   char log_file[256];

   if(!appParameters_get("LOGPATH",NULL) || !strlen(appParameters_get("LOGPATH",NULL))) {
      appParameters_set("LOGPATH","/var/log",NULL);
   }
/*
   int16_t n;
   n=snprintf(log_file,sizeof(log_file),"%s/mea-edomus.log", appParameters_get("LOGPATH",NULL));
   if(n<0 || n==sizeof(log_file)) {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : snprintf - ", ERROR_STR,__func__);
         perror("");
         clean_all_and_exit(1);
      }
   }

   int fd=open(log_file, O_CREAT | O_APPEND | O_RDWR,  S_IWUSR | S_IRUSR);
   if(fd<0) {
      VERBOSE(1) mea_log_printf("%s (%s) : can't open log file (%s) - ",ERROR_STR,__func__,log_file);
      perror("");
      clean_all_and_exit(1);
   }
   
   dup2(fd, 1);
   dup2(fd, 2);
   close(fd);

   logfile_rotation_job(-1, (void *)log_file, NULL, 0);
*/
   DEBUG_SECTION mea_log_printf("INFO  (main) Starting MEA-EDOMUS %s\n",__MEA_EDOMUS_VERSION__);
   
#ifdef __MEA_DEBUG_ON__
   appParameters_display();
#endif

   // chargement des utilisateurs
   initUsers();
   

   //
   // initialisation gestions des signaux (arrêt de l'appli et réinitialisation)
   //
   signal(SIGINT,  _signal_STOP);
   signal(SIGQUIT, _signal_STOP);
   signal(SIGTERM, _signal_STOP);
   signal(SIGCHLD, _signal_SIGCHLD); // pour eviter les zombis
   signal(SIGPIPE, SIG_IGN);


   //
   // démarrage du serveur de temps
   //
   start_timeServer();


   //
   // initialisation du gestionnaire de process
   //
   init_processes_manager(40);


   //
   // déclaration du process principal
   //
   main_monitoring_id=process_register("MAIN");
   process_set_type(main_monitoring_id, NOTMANAGED);
   process_set_status(main_monitoring_id, RUNNING);
   process_add_indicator(main_monitoring_id, "UPTIME", 0);
   process_add_indicator(main_monitoring_id, "SIGSEGV", 0);
   fprintf(stderr,"MAIN : %x\n",  (unsigned int)pthread_self());


   //
   // guiServer
   //
   struct httpServerData_s httpServer_start_stop_params;
   httpServer_start_stop_params.params_list=getAppParameters();
   httpServer_monitoring_id=process_register("HTTPSERVER");
   process_set_group(httpServer_monitoring_id, 5);
   process_set_start_stop(httpServer_monitoring_id , start_httpServer, stop_httpServer, (void *)(&httpServer_start_stop_params), 1);
   process_set_watchdog_recovery(httpServer_monitoring_id, restart_httpServer, (void *)(&httpServer_start_stop_params));
   process_add_indicator(httpServer_monitoring_id, "HTTPIN", 0);
   process_add_indicator(httpServer_monitoring_id, "HTTPOUT", 0);
   if(process_start(httpServer_monitoring_id, NULL, 0)<0) {
      VERBOSE(2) mea_log_printf("%s (%s) : can't start gui server\n",ERROR_STR,__func__);
   }


   //
   // pythonPluginServer
   //
   struct pythonPluginServer_start_stop_params_s pythonPluginServer_start_stop_params;
   pythonPluginServer_start_stop_params.params_list=getAppParameters();
   pythonPluginServer_monitoring_id=process_register("PYTHONPLUGINSERVER");
   process_set_start_stop(pythonPluginServer_monitoring_id , start_pythonPluginServer, stop_pythonPluginServer, (void *)(&pythonPluginServer_start_stop_params), 1);
   process_set_watchdog_recovery(xplServer_monitoring_id, restart_pythonPluginServer, (void *)(&pythonPluginServer_start_stop_params));
   process_add_indicator(pythonPluginServer_monitoring_id, "PYCALL", 0);
   process_add_indicator(pythonPluginServer_monitoring_id, "PYCALLERR", 0);
   if(process_start(pythonPluginServer_monitoring_id, NULL, 0)<0) {
      VERBOSE(1) mea_log_printf("%s (%s) : can't start python plugin server\n",ERROR_STR,__func__);
      clean_all_and_exit(1);
   }
   sleep(1);


   //
   // xPLServer
   //
   struct xplServer_start_stop_params_s xplServer_start_stop_params;
   xplServer_start_stop_params.params_list=getAppParameters();
   xplServer_monitoring_id=process_register(xpl_server_name_str);
   process_set_start_stop(xplServer_monitoring_id, start_xPLServer, stop_xPLServer, (void *)(&xplServer_start_stop_params), 1);
   process_set_watchdog_recovery(xplServer_monitoring_id, restart_xPLServer, (void *)(&xplServer_start_stop_params));
   process_add_indicator(xplServer_monitoring_id, xpl_server_xplin_str, 0);
   process_add_indicator(xplServer_monitoring_id, xpl_server_xplout_str, 0);
   process_add_indicator(xplServer_monitoring_id, xpl_server_senderr_str, 0);
   process_add_indicator(xplServer_monitoring_id, xpl_server_readerr_str, 0);
   if(process_start(xplServer_monitoring_id, NULL, 0)<0) {
      VERBOSE(1) mea_log_printf("%s (%s) : can't start xpl server\n",ERROR_STR,__func__);
      clean_all_and_exit(1);
   }

   //
   // automatorServer
   //
   struct automatorServer_start_stop_params_s automatorServer_start_stop_params;
   automatorServer_start_stop_params.params_list=getAppParameters();
   automatorServer_monitoring_id=process_register(automator_server_name_str);
   process_set_start_stop(automatorServer_monitoring_id, start_automatorServer, stop_automatorServer, (void *)(&automatorServer_start_stop_params), 1);
   process_set_watchdog_recovery(automatorServer_monitoring_id, restart_automatorServer, (void *)(&automatorServer_start_stop_params));
   process_add_indicator(automatorServer_monitoring_id, automator_input_exec_time_str, 0);
   process_add_indicator(automatorServer_monitoring_id, automator_output_exec_time_str, 0);
   process_add_indicator(automatorServer_monitoring_id, automator_xplin_str, 0);
   process_add_indicator(automatorServer_monitoring_id, automator_xplout_str, 0);
   process_add_indicator(automatorServer_monitoring_id, automator_err_str, 0);
   if(process_start(automatorServer_monitoring_id, NULL, 0)<0) {
      VERBOSE(1) mea_log_printf("%s (%s) : can't start automator server\n",ERROR_STR,__func__);
      clean_all_and_exit(1);
   }


   //
   // interfacesServer
   //
   struct interfacesServerData_s interfacesServerData;
   interfacesServerData.params_list=getAppParameters();
//   interfaces=start_interfaces(getAppParameters()); // démarrage des interfaces
   start_interfaces(getAppParameters()); // démarrage des interfaces
   int interfaces_reload_task_id=process_register("RELOAD"); // mise en place de la tâche de rechargement des paramètrages des interfaces
   process_set_group(interfaces_reload_task_id, 2);
   process_set_start_stop(interfaces_reload_task_id, restart_interfaces, NULL, (void *)(&interfacesServerData), 1);
   process_set_type(interfaces_reload_task_id, TASK);


   //
   // batch rotation des log 1x par jour (0|0|*|*|* = à miniuit)
   // 
   int log_rotation_id=process_register("LOGROTATION");
   process_set_start_stop(log_rotation_id, logfile_rotation_job, NULL, (void *)log_file, 1);
   process_set_type(log_rotation_id, JOB);
//   process_job_set_scheduling_data(log_rotation_id, "0,5,10,15,20,25,30,35,40,45,50,55|*|*|*|*", 0);
   process_job_set_scheduling_data(log_rotation_id, "0|0|*|*|*", 0); // rotation des log tous les jours à minuit
   process_set_group(log_rotation_id, 7);
   VERBOSE(1) mea_log_printf("%s (%s) : MEA-EDOMUS %s starded\n", INFO_STR, __func__, __MEA_EDOMUS_VERSION__);


   time_t start_time;
   long uptime = 0;
   start_time = time(NULL);
   int apiport = atoi(appParameters_get("HTTPPORT",NULL));

   while(1) { // boucle principale
      // supervision "externe" des process
      if(process_is_running(httpServer_monitoring_id)==RUNNING) {
         char response[512];
         // interrogation du serveur HTTP Interne pour heartbeat ... (voir le passage d'un parametre pour sécuriser ...)
         gethttp(localhost_const, apiport, "/CMD/ping", response, sizeof(response)); 
      }

      // indicateur de fonctionnement de mea-edomus
      uptime = (long)(time(NULL)-start_time);
      process_update_indicator(main_monitoring_id, "UPTIME", uptime);
      process_update_indicator(main_monitoring_id, "SIGSEGV", sigsegv_indicator);

      managed_processes_loop(); // watchdog et indicateurs
      
      sleep(1);
   }
}

// http://www.tux-planet.fr/utilisation-des-commandes-diff-et-patch-sous-linux/
