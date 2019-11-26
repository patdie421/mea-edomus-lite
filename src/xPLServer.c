//
//
//  Created by Patrice DIETSCH on 17/10/12.
//
//
#ifdef __APPLE__
#include <Python/Python.h>
#else
#include <Python.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include "globals.h"
#include "configuration.h"
#include "consts.h"
#include "mea_verbose.h"
#include "mea_queue.h"
#include "mea_timer.h"
#include "tokens.h"
#include "tokens_da.h"
#include "xPLServer.h"
#include "mea_string_utils.h"
#include "processManager.h"
#include "automator.h"
#include "interfacesServer.h"
#include "automatorServer.h"
#include "mea_sockets_utils.h"
#include "cJSON.h"
#include "mea_xpl.h"

#define XPL_VERSION "0.1a2"

//#define XPL_WD 1
#ifdef XPL_WD
char *xplWDMsg = NULL;
int   xplWDMsg_l = -1;
#endif
mea_timer_t xPLnoMsgReceivedTimer;


char *xpl_server_name_str="XPLSERVER";
char *xpl_server_xplin_str="XPLIN";
char *xpl_server_xplout_str="XPLOUT";
char *xpl_server_senderr_str="XPLSENDERR";
char *xpl_server_readerr_str="XPLREADERR";

// gestion du service xPL
char *xpl_vendorID=NULL;
char *xpl_deviceID=NULL;
char *xpl_instanceID=NULL;
char  xpl_my_addr[36]="";

// gestion du thread et des indicateurs
pthread_t *_xPLServer_thread_id;
jmp_buf    xPLServer_JumpBuffer;
int        xPLServer_longjmp_flag = 0;
int       _xplServer_monitoring_id = -1;
volatile sig_atomic_t _xPLServer_thread_is_running=0;

int   xpl_sd=-1;
int   xpl_sdb=-1;
int   xpl_port=-1;
char  xpl_ip[40];
struct sockaddr_in *xpl_broadcastAddr=NULL;
char  xpl_interface[255]="";
char  xpl_interval=1; // valeur minimum

long xplin_indicator = 0;
long xplout_indicator = 0;
long xplsenderr_indicator = 0;
long xplreaderr_indicator = 0;

// gestion de des messages xpl internes
uint32_t requestId = 1;
pthread_mutex_t requestId_lock;
mea_queue_t    *xplRespQueue;
pthread_cond_t  xplRespQueue_sync_cond;
pthread_mutex_t xplRespQueue_sync_lock;
int             _xPLServer_mutex_initialized=0;

void _rawXPLMessageHandler(char *data, int l_data);


void set_xPLServer_isnt_running(void *data)
{
   _xPLServer_thread_is_running=0;
}


void clean_xPLServer(void *data)
{
#ifdef XPL_WD
   if(xplWDMsg) {
      free(xplWDMsg);
      xplWDMsg=NULL;
   }
#endif
}


uint32_t mea_getXplRequestId() // rajouter un verrou ...
{
   uint32_t id=0;

   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(requestId_lock) );
   pthread_mutex_lock(&requestId_lock);

   id=requestId;

   requestId++;
   if(requestId>20000)
      requestId=1;

   pthread_mutex_unlock(&requestId_lock);
   pthread_cleanup_pop(0);
 
   return id;
}


char *mea_setXPLVendorID(char *value)
{
   return mea_string_free_alloc_and_copy(&xpl_vendorID, value);
}


char *mea_setXPLDeviceID(char *value)
{
   return mea_string_free_alloc_and_copy(&xpl_deviceID, value);
}


char *mea_setXPLInstanceID(char *value)
{
   return mea_string_free_alloc_and_copy(&xpl_instanceID, value);
}


char *mea_getMyXPLAddr()
{
   return xpl_my_addr;
}


char *mea_getXPLInstanceID()
{
   return xpl_instanceID;
}


char *mea_getXPLDeviceID()
{
   return xpl_deviceID;
}


char *mea_getXPLVendorID()
{
   return xpl_vendorID;
}


int mea_sendXplMsgJson(cJSON *xplMsgJson)
{
   if(xplMsgJson==NULL)
      return -1;

   cJSON *e=xplMsgJson->child;
   if(e==NULL)
      return -1;

//   int retour=0;

   char *schema=NULL;
   char *target=NULL;
   char *source=NULL;
   char *type=NULL;

   char xplBodyStr[2048] = "";
   int xplBodyStrPtr = 0;

   source = xpl_my_addr;
   target="*";

   while(e) {
      // type de message
      if(strcmp(e->string, XPLMSGTYPE_STR_C)==0)
         type=e->valuestring;
      else if(strcmp(e->string, XPLSCHEMA_STR_C)==0)
         schema=e->valuestring;
      else if(strcmp(e->string, XPLSOURCE_STR_C)==0)
         source=e->valuestring;
      else if(strcmp(e->string, XPLTARGET_STR_C)==0)
         target=e->valuestring;
      else {
         int n=sprintf(&(xplBodyStr[xplBodyStrPtr]),"%s=%s\n",e->string,e->valuestring);

         xplBodyStrPtr+=n;
      }
      e=e->next;
   }

   if(!type || !schema || !source || !target || !xplBodyStrPtr)
      return -1;

   char *msg = (char *)alloca(2048);

   int n=sprintf(msg,"%s\n{\nhop=1\nsource=%s\ntarget=%s\n}\n%s\n{\n%s}\n",type,source,target,schema,xplBodyStr);

   if(n>0) {
      int ret=mea_xPLSendMessage(xpl_sdb, xpl_broadcastAddr, msg, n);
      if(ret<0)
         process_update_indicator(_xplServer_monitoring_id, xpl_server_senderr_str, ++xplsenderr_indicator); 
      return 0;
   }
   return -1;
}


uint16_t mea_sendXPLMessage2(cJSON *xplMsgJson)
{
   char *str = NULL;
   char deviceID[17]="";
   xplRespQueue_elem_t *e;
   cJSON *j = NULL;

   process_update_indicator(_xplServer_monitoring_id, xpl_server_xplout_str, ++xplout_indicator);

   str=NULL;
   deviceID[0]=0;
   j=cJSON_GetObjectItem(xplMsgJson, XPLSOURCE_STR_C);
   if(j) {
      str=j->valuestring;
      if(str)
         sscanf(str, "%*[^-]-%[^.].%*s", deviceID);
   }

   if(deviceID[0]!=0 && strcmp(deviceID, INTERNAL_STR_C)==0) { // source interne => dispatching sans passer par le réseau
      dispatchXPLMessageToInterfaces2(xplMsgJson);
      return 0;
   }

   str=NULL;
   deviceID[0]=0;
   j=cJSON_GetObjectItem(xplMsgJson, XPLTARGET_STR_C);
   if(j) {
      str=j->valuestring;
      if(str)
         sscanf(str, "%*[^-]-%[^.].%*s", deviceID);
   }

   if(deviceID[0]!=0 && strcmp(str,INTERNAL_STR_C)==0) { // destination interne, retour à mettre dans une file (avec timestamp) ...
      int id;

      sscanf(str, "%*[^.].%d", &id);
      cJSON *xplMsgJson_new = cJSON_Duplicate(xplMsgJson, 1);

      pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(xplRespQueue_sync_lock) );
      pthread_mutex_lock(&xplRespQueue_sync_lock);

      if(xplRespQueue) {
         e = malloc(sizeof(xplRespQueue_elem_t));
         e->msgjson = xplMsgJson_new;
         e->id = id;
         e->tsp = (uint32_t)time(NULL);

         mea_queue_in_elem(xplRespQueue, e);

         if(xplRespQueue->nb_elem>=1)
            pthread_cond_broadcast(&xplRespQueue_sync_cond);
      }

      pthread_mutex_unlock(&xplRespQueue_sync_lock);
      pthread_cleanup_pop(0);

      return 0;
   }
   else {
      mea_sendXplMsgJson(xplMsgJson);

      return 0;
   }
}


cJSON *mea_readXPLResponse2(int id)
{
   int16_t ret;
   uint16_t notfound=0;
   cJSON *xplMsgJson = NULL;

   // on va attendre le retour dans la file des reponses
   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(xplRespQueue_sync_lock) );
   pthread_mutex_lock(&(xplRespQueue_sync_lock));

   if((xplRespQueue && xplRespQueue->nb_elem==0) || notfound==1) {
      // rien a lire => on va attendre que quelque chose soit mis dans la file
      struct timeval tv;
      struct timespec ts;
      gettimeofday(&tv, NULL);
      ts.tv_sec = tv.tv_sec + 2; // timeout de deux secondes
      ts.tv_nsec = 0;
      ret=pthread_cond_timedwait(&xplRespQueue_sync_cond, &xplRespQueue_sync_lock, &ts);
      if(ret) {
         if(ret!=ETIMEDOUT)
            goto readFromQueue_return;
      } 
   }
   else
      goto readFromQueue_return;

   // a ce point il devrait y avoir quelque chose dans la file.
   if(mea_queue_first(xplRespQueue)==0) {
      xplRespQueue_elem_t *e;
      do { // parcours de la liste jusqu'a trouver une reponse pour nous
         if(mea_queue_current(xplRespQueue, (void **)&e)==0) { 
            if(e->id==id) {
               uint32_t tsp=(uint32_t)time(NULL);

               if((tsp - e->tsp)<=10) { // la reponse est pour nous et dans les temps (retour dans les 10 secondes)
                  // recuperation des donnees
                  xplMsgJson=e->msgjson;
                  // et on fait le menage avant de sortir
                  free(e);
                  e=NULL;
                  xplRespQueue->current->d=NULL; // pour evite un bug (mais je sais plus lequel ...)
                  mea_queue_remove_current(xplRespQueue);
                  goto readFromQueue_return;
               }
               // theoriquement pour nous mais donnees trop vieilles, on supprime ?
               DEBUG_SECTION mea_log_printf("%s (%s) : data are too old\n", DEBUG_STR,__func__);
            }
            else {
               DEBUG_SECTION mea_log_printf("%s (%s) : data aren't for me (%d != %d)\n", DEBUG_STR,__func__, e->id, id);
               e=NULL;
            }
         }
      }
      while(mea_queue_next(xplRespQueue)==0);
   }

readFromQueue_return:
   pthread_mutex_unlock(&(xplRespQueue_sync_lock));
   pthread_cleanup_pop(0);

   return xplMsgJson;
}


void _flushExpiredXPLResponses()
{
   xplRespQueue_elem_t *e;
   uint32_t tsp=(uint32_t)time(NULL);
   
   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(xplRespQueue_sync_lock) );
   pthread_mutex_lock(&xplRespQueue_sync_lock);
   
   if(xplRespQueue && mea_queue_first(xplRespQueue)==0) {
      while(1) {
         if(mea_queue_current(xplRespQueue, (void **)&e)==0) {
            if((tsp - e->tsp) > 5) {
               cJSON_Delete(e->msgjson);
               free(e);
               e=NULL;
               mea_queue_remove_current(xplRespQueue); // remove current passe au le suivant
               DEBUG_SECTION mea_log_printf("%s (%s) : a response queue was flushed\n", DEBUG_STR, __func__);
            }
            else
               mea_queue_next(xplRespQueue);
         }
         else
            break;
      }
   }
   
   pthread_mutex_unlock(&xplRespQueue_sync_lock);
   pthread_cleanup_pop(0);
}


int16_t set_xpl_address(cJSON *params_list)
/**
 * \brief     initialise les données pour l'adresse xPL
 * \details   positionne vendorID, deviceID et instanceID pour xPLServer
 * \param     params_list  liste des parametres.
 * \return   -1 en cas d'erreur, 0 sinon
 */
{
   mea_setXPLVendorID(appParameters_get("XPL_VENDORID", params_list));
   mea_setXPLDeviceID(appParameters_get("XPL_DEVICEID", params_list));
   mea_setXPLInstanceID(appParameters_get("XPL_INSTANCEID", params_list));

   return 0;
}


void _xplRespQueue_free_queue_elem(void *d)
{
   xplRespQueue_elem_t *e=(xplRespQueue_elem_t *)d;
   cJSON_Delete(e->msgjson);
   e->msgjson=NULL;
}


cJSON *xPLParser(const char* xplmsg, char *xpl_type, char *xpl_schema, char *xpl_source, char *xpl_target)
{
   int y, z, c;
   int w;
   char xpl_section[35];
   char xpl_name[17];
   char xpl_value[129];

#define XPL_IN_SECTION 0
#define XPL_IN_NAME 1
#define XPL_IN_VALUE 2
   int   s1=0;
   cJSON *msg_json = cJSON_CreateObject();

   w = XPL_IN_SECTION;
   z = 0;
   y = (int)(strlen(xplmsg));

   for (c=0;c<y;c++) {

      switch(w) {
         case XPL_IN_SECTION:
            if((xplmsg[c] != 10) && (z != 34)) {
               xpl_section[z]=xplmsg[c];
               ++z;
            }
            else {
               ++c;
               ++c;
               ++w;
               xpl_section[z]='\0';
               if(s1==0) {
                  if(xpl_type)
                     strcpy(xpl_type, xpl_section);
                  cJSON_AddItemToObject(msg_json, XPLMSGTYPE_STR_C, cJSON_CreateString(xpl_section));
               }
               else if(s1==1) {
                  if(xpl_schema)
                  strcpy(xpl_schema, xpl_section);
                  cJSON_AddItemToObject(msg_json, XPLSCHEMA_STR_C, cJSON_CreateString(xpl_section));
               }
               ++s1;
               z=0;
            }
            break;
         case XPL_IN_NAME:
            if((xplmsg[c] != '=') && (xplmsg[c] != '!') && (z != 16)) {
               if(z<16) {
                  xpl_name[z]=xplmsg[c];
                  ++z;
               }
            }
            else {
               ++w;
               xpl_name[z]='\0';
               z=0;
            }
            break;
         case XPL_IN_VALUE:
            if((xplmsg[c] != 10) && (z != 128)) {
               if(z<128) {
                  xpl_value[z]=xplmsg[c];
                  ++z;
               }
            }
            else {
               ++w;
               xpl_value[z]='\0';
               z=0;
               if(s1==1) {
                  if(strcmp(xpl_name, "source") == 0) {
                     if(xpl_source)
                        strcpy(xpl_source, xpl_value);
                     cJSON_AddItemToObject(msg_json, XPLSOURCE_STR_C, cJSON_CreateString(xpl_value));
                  }
                  else if(strcmp(xpl_name, "target") == 0) {
                     strcpy(xpl_target, xpl_value);
                     cJSON_AddItemToObject(msg_json, XPLTARGET_STR_C, cJSON_CreateString(xpl_value));
                  }
               }
               else if(s1==2) {
                  cJSON_AddItemToObject(msg_json,  xpl_name, cJSON_CreateString(xpl_value));
               }

               if(xplmsg[c+1] != '}') {
                  w=XPL_IN_NAME;
               }
               else {
                  w=XPL_IN_SECTION;
                  ++c;
                  ++c;
               }
            }
            break;
      }
   }

   return msg_json; 
}


void _rawXPLMessageHandler(char *s, int l)
{
   char type[35];
   char schema[35];
   char source[35];
   char target[35];

   process_update_indicator(_xplServer_monitoring_id, xpl_server_xplin_str, ++xplin_indicator);

   cJSON *msg_json = xPLParser(s, type, schema, source, target);
   if(msg_json != NULL) {
      mea_start_timer(&xPLnoMsgReceivedTimer);

      DEBUG_SECTION {
         int fromMe=-1;
         if(strcmp(source, xpl_my_addr) == 0)
            fromMe=0;

         if(strcmp(schema,"watchdog.basic")== 0 && fromMe == 0) {
            mea_log_printf("%s (%s) : watchdog xpl\n", DEBUG_STR, __func__);
         }
      }

      cJSON *_msg_json=NULL; // le message que l'on va envoyer a l'automate
      if(strcmp(type, XPL_CMND_STR_C) != 0) { // on aura pas a faire traiter le message par les interfaces
         _msg_json=msg_json; // donc on ne le transmettra qu'a l'automate
         msg_json=NULL;
      }
      else {
         _msg_json=cJSON_Duplicate(msg_json, 1); // duplication car il sera traiter par les interfaces et l'automate
      }
      
      if(_msg_json) { // on envoie tous les messages valides à l'automate (à lui de filtrer ...)
         if(automatorServer_add_msg(_msg_json)==ERROR) {
            DEBUG_SECTION mea_log_printf("%s (%s) : to automator error\n", DEBUG_STR, __func__);
         }
      }
      
      if(msg_json) {
         dispatchXPLMessageToInterfaces2(msg_json);
      }
      
/*
      // on envoie tous les messages à l'automate (à lui de filtrer ...)
      cJSON *xplMsgJson_automator = cJSON_Duplicate(msg_json, 1);
      if(xplMsgJson_automator) {
         if(automatorServer_add_msg(xplMsgJson_automator)==ERROR) {
            DEBUG_SECTION mea_log_printf("%s (%s) : to automator error\n", DEBUG_STR, __func__);
         }
         else {
            cJSON_Delete(xplMsgJson_automator);
         }
      }
      else {
         DEBUG_SECTION mea_log_printf("%s (%s) : can't duplicate json xpl message\n", DEBUG_STR, __func__);
      }

      // pour les autres on filtre un peu avant de transmettre pour traitement
      // on ne traite que les cmnd au niveau des interfaces
      if(strcmp(type, XPL_CMND_STR_C) != 0) {
         cJSON_Delete(msg_json);
         return;
      }

      dispatchXPLMessageToInterfaces2(msg_json);
*/
   }
}


int16_t mea_xPLSendMessage2(char *data, int l_data)
{
   if(xpl_sdb < 0 || xpl_broadcastAddr == NULL) {
      // DEBUG_SECTION mea_log_printf("%s (%s) : xplServer not ready (data=%s)\n", data);
      return -1;
   }

   int ret=mea_xPLSendMessage(xpl_sdb, xpl_broadcastAddr, data, l_data);
   if(ret<0)
      process_update_indicator(_xplServer_monitoring_id, xpl_server_senderr_str, ++xplsenderr_indicator);
   
   return 0;
}


void *xPLServer_thread(void *data)
{
   int16_t nerr;

   pthread_cleanup_push( (void *)set_xPLServer_isnt_running, (void *)NULL );
   pthread_cleanup_push( (void *)clean_xPLServer, (void *)NULL);
   
   xPLServer_longjmp_flag = 0;
   
   if(strlen(xpl_interface)==0)
      return NULL;

   // ouverture des communications udp
   xpl_sd=mea_xPLConnectHub(&xpl_port);
   if(xpl_sd<0)
      return NULL;
   xpl_broadcastAddr=(struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
   if(!xpl_broadcastAddr) {
      close(xpl_sd);
      return NULL;
   }
   xpl_sdb=mea_xPLConnectBroadcast(xpl_interface, xpl_ip, sizeof(xpl_ip)-1, xpl_broadcastAddr);
   if(xpl_sdb<0) {
      free(xpl_broadcastAddr);
      xpl_broadcastAddr=NULL;
      close(xpl_sd);
      xpl_sd=-1;
      xpl_sdb=-1;
      return NULL;
   }

   _xPLServer_thread_is_running=1; // c'est bon, on a démarré

   process_heartbeat(_xplServer_monitoring_id); // 1er heartbeat après démarrage.

   mea_xPLSendHbeat(xpl_sdb, xpl_broadcastAddr, xpl_ip, xpl_port, XPL_VERSION, xpl_my_addr, "app", xpl_interval);

#ifdef XPL_WD
   char *_xplWDMsg = "xpl-trig\n{\nhop=1\nsource=%s\ntarget=%s\n}\nwatchdog.basic\n{\ninterval=10\n}\n";

   mea_timer_t xPLWDSendMsgTimer;
//   mea_init_timer(&xPLnoMsgReceivedTimer, 30, 1);
   mea_init_timer(&xPLWDSendMsgTimer, 10, 1);

   xplWDMsg=malloc(strlen(_xplWDMsg)-4 + 2*strlen(xpl_my_addr) + 1);
   xplWDMsg_l = sprintf(xplWDMsg, _xplWDMsg, xpl_my_addr, xpl_my_addr); // message pour moi meme ...
#endif

   mea_init_timer(&xPLnoMsgReceivedTimer, 30, 1);
   mea_start_timer(&xPLnoMsgReceivedTimer);

   char data[0xFFFF];
   int l_data;
   mea_timer_t hbTimer;

#ifdef XPL_WD
   mea_init_timer(&hbTimer, xpl_interval*60, 1);
#else
   mea_init_timer(&hbTimer, 10, 1); // pour remplacer watchdog.basic
#endif
   mea_start_timer(&hbTimer);

   do {
      if(setjmp(xPLServer_JumpBuffer) != 0) {
         xPLServer_longjmp_flag++;
         break;
      }

      pthread_testcancel();
      process_heartbeat(_xplServer_monitoring_id); // heartbeat après chaque boucle

      if(mea_test_timer(&hbTimer)==0) {
         mea_xPLSendHbeat(xpl_sdb, xpl_broadcastAddr, xpl_ip, xpl_port, XPL_VERSION, xpl_my_addr, "app", xpl_interval);
      }

      l_data = sizeof(data);
      int ret=mea_xPLReadMessage(xpl_sd, 500, data, &l_data, &nerr);
      data[l_data]=0;
      if(ret>=0) {
         if(l_data>0)
            _rawXPLMessageHandler(data, l_data);
      }
      else {
         process_update_indicator(_xplServer_monitoring_id, xpl_server_readerr_str, ++xplreaderr_indicator);
         perror("");
      }
      
      if(mea_test_timer(&xPLnoMsgReceivedTimer)==0) {
         // pas de message depuis X secondes
         VERBOSE(2) {
            mea_log_printf("%s (%s) : no xPL message since 30 seconds, but we should have one ... I send me three or more !\n", ERROR_STR, __func__);
            mea_log_printf("%s (%s) : watchdog_recovery forced ...\n", ERROR_STR, __func__);
         }
         process_forced_watchdog_recovery(_xplServer_monitoring_id);
      }
#ifdef XPL_WD
      if(mea_test_timer(&xPLWDSendMsgTimer)==0) { // envoie d'un message toutes les X secondes
         mea_xPLSendMessage(xpl_sdb, xpl_broadcastAddr, xplWDMsg, xplWDMsg_l);
      }
#endif

      DEBUG_SECTION {
         static char compteur=0;
         if(compteur>59) {
            compteur=0;
            mea_log_printf("%s (%s) : %s thread is running\n", INFO_STR, __func__, xpl_server_name_str);
         }
         else
            compteur++;
      }

      _flushExpiredXPLResponses();
   }
   while (1);

   if(xPLServer_longjmp_flag > 1)
      abort();

   pthread_cleanup_pop(1);
   pthread_cleanup_pop(1);
   
   return NULL;
}


pthread_t *xPLServer()
{
   if(!xpl_deviceID || !xpl_instanceID || !xpl_vendorID) {
      return NULL;
   }
   
   // préparation synchro consommateur / producteur si nécessaire
   if(_xPLServer_mutex_initialized==0) {
      pthread_cond_init(&xplRespQueue_sync_cond, NULL);
      pthread_mutex_init(&xplRespQueue_sync_lock, NULL);
      pthread_mutex_init(&requestId_lock, NULL);
      _xPLServer_mutex_initialized=1;
   }

   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(xplRespQueue_sync_lock) );
   pthread_mutex_lock(&(xplRespQueue_sync_lock));
   xplRespQueue=(mea_queue_t *)malloc(sizeof(mea_queue_t));
   if(!xplRespQueue) {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      return NULL;
   }
   mea_queue_init(xplRespQueue); // initialisation de la file
   pthread_mutex_unlock(&(xplRespQueue_sync_lock));
   pthread_cleanup_pop(0);

   _xPLServer_thread_id=(pthread_t *)malloc(sizeof(pthread_t));
   if(!_xPLServer_thread_id) {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      free(xplRespQueue);
      xplRespQueue=NULL;
      return NULL;
   }

   if(pthread_create (_xPLServer_thread_id, NULL, xPLServer_thread, NULL)) {
      VERBOSE(1) mea_log_printf("%s (%s) : pthread_create - can't start thread\n",ERROR_STR,__func__);
      return NULL;
   }
      
   return _xPLServer_thread_id;
}


int stop_xPLServer(int my_id, void *data,  char *errmsg, int l_errmsg)
{
   int ret=0;
   
   if(_xPLServer_thread_id) {
      pthread_cancel(*_xPLServer_thread_id);
      int counter=1000; // une seconde max pour s'arrêter proprement
      int stopped=-1;
      while(counter--) {
         if(_xPLServer_thread_is_running) {  // pour éviter une attente "trop" active
            usleep(100); // will sleep for 1 ms
         }
         else {
            stopped=0;
            break;
         }
      }

      DEBUG_SECTION mea_log_printf("%s (%s) : %s, end after %d loop(s)\n",DEBUG_STR, __func__,xpl_server_name_str,1000-counter);
      ret=stopped;
   }

   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(xplRespQueue_sync_lock) );
   pthread_mutex_lock(&(xplRespQueue_sync_lock));
   if(xplRespQueue) {
      mea_queue_cleanup(xplRespQueue,_xplRespQueue_free_queue_elem);
      free(xplRespQueue);
      xplRespQueue=NULL;
   }
   pthread_mutex_unlock(&(xplRespQueue_sync_lock));
   pthread_cleanup_pop(0);


   if(xpl_sd>=0) {
      close(xpl_sd);
      xpl_sd=-1;
   }
   if(xpl_sdb>=0) {
      mea_xPLSendHbeat(xpl_sdb, xpl_broadcastAddr, xpl_ip, xpl_port, XPL_VERSION, xpl_my_addr, "end", xpl_interval);
      close(xpl_sdb);
      xpl_sd=-1;
   }
   if(xpl_broadcastAddr) {
      free(xpl_broadcastAddr);
      xpl_broadcastAddr=NULL;
   }

   _xplServer_monitoring_id=-1;

   if(_xPLServer_thread_id) {
      free(_xPLServer_thread_id);
      _xPLServer_thread_id=NULL;
   }   

   if(ret==0) {
      VERBOSE(2) mea_log_printf("%s (%s) : %s %s.\n", INFO_STR, __func__, xpl_server_name_str, stopped_successfully_str);
   }
   else {
      VERBOSE(2) mea_log_printf("%s (%s) : %s can't cancel thread.\n", INFO_STR, __func__, xpl_server_name_str);
   }
   return ret;
}


int start_xPLServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   struct xplServer_start_stop_params_s *xplServer_params = (struct xplServer_start_stop_params_s *)data;
   char err_str[256];
   
   if(!set_xpl_address(xplServer_params->params_list)) {
      strcpy(xpl_interface, appParameters_get("INTERFACE", xplServer_params->params_list));
      _xplServer_monitoring_id=my_id;
      _xPLServer_thread_id=xPLServer();

      if(_xPLServer_thread_id==NULL) {
#ifdef _POSIX_SOURCE
         char *ret;
#else
         int ret;
#endif
         ret=strerror_r(errno, err_str, sizeof(err_str));
         VERBOSE(1) {
            mea_log_printf("%s (%s) : can't start xpl server - %s\n",ERROR_STR,__func__,err_str);
         }

         return -1;
      }
      else {
         pthread_detach(*_xPLServer_thread_id);
         VERBOSE(2) mea_log_printf("%s (%s) : %s launched successfully.\n", INFO_STR, __func__, xpl_server_name_str);

         return 0;
      }
   }
   else {
      VERBOSE(1) mea_log_printf("%s (%s) : no valid xPL address.\n", ERROR_STR, __func__);
      return -1;
   }
}


int restart_xPLServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   int ret=0;
   ret=stop_xPLServer(my_id, data, errmsg, l_errmsg);
   if(ret==0) {
      return start_xPLServer(my_id, data, errmsg, l_errmsg);
   }
   return ret;
}
