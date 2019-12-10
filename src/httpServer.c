//
//  httpServer.c
//  mea-eDomus
//
//  Created by Patrice Dietsch on 04/06/13.
//
//

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

#include "globals.h"
#include "consts.h"
#include "mea_verbose.h"
#include "configuration.h"
#include "httpServer.h"
#include "mongoose.h"
#include "mea_string_utils.h"
#include "mea_queue.h"
#include "xPLServer.h"
#include "tokens.h"
#include "mea_sockets_utils.h"
#include "cJSON.h"
#include "mea_rest_api.h"

#include "processManager.h"
#include "automator.h"
#include "automatorServer.h"


const char *str_document_root="document_root";
const char *str_listening_ports="listening_ports";
const char *str_index_files="index_files";
const char *str_num_threads="num_threads";

char *val_document_root=NULL;
char *val_listening_ports=NULL;
char *val_index_files=NULL;
char *val_num_threads=NULL;


// Variable globale privée
int  _httpServer_monitoring_id=-1;


int httpin_indicator=0; // nombre de demandes recues
int httpout_indicator=0; // nombre de réponses émises

const char *options[15];
struct mg_context* g_mongooseContext = 0;

#define REQUETE_SIZE 1000

int gethttp(char *server, int port, char *url, char *response, int l_response)
{
   int sockfd; // descripteur de socket
   
   // creation de la requete
   char requete[REQUETE_SIZE]="GET ";
   requete[REQUETE_SIZE-1]=0;

   strncat(requete, url, REQUETE_SIZE-1);
   strncat(requete, " HTTP/1.1\r\n", REQUETE_SIZE-1);
   strncat(requete, "Host: ", REQUETE_SIZE-1);
   strncat(requete, server, REQUETE_SIZE-1);
   strncat(requete, "\r\n", REQUETE_SIZE-1);
   strncat(requete, "Connection: close\r\n", REQUETE_SIZE-1);
   strncat(requete, "\r\n", REQUETE_SIZE-1);
   
   if(mea_socket_connect(&sockfd, server, port)<0) {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : mea_socket_connect - ", ERROR_STR,__func__);
         perror("");
      }
      return -1;
   }
   
   if(mea_socket_send(&sockfd, requete, (int)strlen(requete))<0) {
      close(sockfd);
      return -1;
   }
   
   // reception de la reponse HTTP
   int n=(int)recv(sockfd, response, l_response, 0);
   if(n == -1) {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : recv - ", ERROR_STR,__func__);
         perror("");
      }
      close(sockfd);
      return -1;
   }
   
   // fermeture de la socket
   close(sockfd);
   
   return 0;
}


void httpResponse(struct mg_connection *conn, int httpCode, char *httpMessage, char *response)
{
   char *msg=NULL;

   if(httpMessage==NULL) {
      switch(httpCode) {
         case 200: // Requête traitée avec succès.
            msg="OK"; break;
         case 201: // Requête traitée avec succès et création d’un document.
            msg="Created"; break;
         case 202: // Requête traitée, mais sans garantie de résultat.
            msg="Accepted"; break;
         case 203: // Information retournée, mais générée par une source non certifiée.
            msg="Non-Authoritative Information"; break;
         case 204: // Requête traitée avec succès mais pas d’information à renvoyer.
            msg="No Content"; break;
         case 205: // Requête traitée avec succès, la page courante peut être effacée.
            msg="Reset Content"; break;
         case 206: // Une partie seulement de la ressource a été transmise.
            msg="Partial Content"; break;

         case 400: // La syntaxe de la requête est erronée.
            msg="Bad Request"; break;
         case 401: // Une authentification est nécessaire pour accéder à la ressource.
            msg="Unauthorized"; break; 
         case 402: // Paiement requis pour accéder à la ressource.
            msg="Payment Required"; break;
         case 403: // Le serveur a compris la requête, mais refuse de l'exécuter. (ex : l'authentification a été acceptée mais les droits d'accès ne permettent pas au client d'accéder à la ressource.)
            msg="Forbidden"; break;
         case 404: // Ressource non trouvée.
            msg="Not Found"; break; 
         case 405: // Méthode de requête non autorisée.
            msg="Method Not Allowed"; break;
         case 406: // La ressource demandée n'est pas disponible dans un format qui respecterait les en-têtes "Accept" de la requête.
            msg="Not Acceptable"; break;

         default:
            msg=""; break;   
      }
   }
   else {
      msg=httpMessage;
   }

   mg_printf(conn,
             "HTTP/1.1 %d %s\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %d\r\n" // Always set Content-Length
             "Cache-Control: no-cache, no-store, must-revalidate\r\n"
             "Pragma: no-cache\r\n"
             "Expires: -1\r\n"
             "Vary: *\r\n"
             "\r\n"
             "%s\r\n",
             (int)httpCode, msg, (int)strlen(response)+2, response);

   process_update_indicator(_httpServer_monitoring_id, "HTTPOUT", ++httpout_indicator);
}


void _httpResponse(struct mg_connection *conn, char *response)
{
   mg_printf(conn,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %d\r\n" // Always set Content-Length
             "Cache-Control: no-cache, no-store, must-revalidate\r\n"
             "Pragma: no-cache\r\n"
             "Expires: -1\r\n"
             "Vary: *\r\n"
             "\r\n"
             "%s\r\n",
             (int)strlen(response)+2, response);

   process_update_indicator(_httpServer_monitoring_id, "HTTPOUT", ++httpout_indicator);
}


void _httpErrno(struct mg_connection *conn, int n, char *msg)
{
   char errno_str[100];
   char *iserror;

   if(n!=0)
      iserror="true";
   else
      iserror="false";
      
   if(msg)
      snprintf(errno_str, sizeof(errno_str)-1, "{ \"iserror\" : %s, \"errno\" : %d, \"errMsg\" : \"%s\" }",iserror, n, msg);
   else
      snprintf(errno_str, sizeof(errno_str)-1, "{ \"iserror\" : %s, \"errno\" : %d }", iserror, n);
   
   _httpResponse(conn, errno_str);
}


int gui_ping(struct mg_connection *conn)
{
   process_heartbeat(_httpServer_monitoring_id); // le heartbeat est fait de l'extérieur ...
      
   _httpErrno(conn, 0, NULL);
   return 1;
}


static int _begin_request_handler(struct mg_connection *conn)
{
   const struct mg_request_info *request_info = mg_get_request_info(conn);
//   char phpsessid[80];
   
   httpin_indicator++;
   process_update_indicator(_httpServer_monitoring_id, "HTTPIN", httpin_indicator);

   int l_uri=(int)strlen((char *)request_info->uri)+1;
   char *uri=alloca(l_uri);
   uri[l_uri-1]=0;
   strncpy(uri, request_info->uri, l_uri-1);

   char *tokens[20];
   int l_tokens=mea_strsplit(uri, '/', tokens, 20);
   if(l_tokens>0) {
      int j=0;
      for(int i=0;i<l_tokens;i++) {
         if(tokens[i][0]!=0) {
            tokens[j]=tokens[i];
            j++;
         }
      }
      l_tokens=j;
      for(int i=0;i<l_tokens;i++) {
         char *s = tokens[i];
         while (*s) {
            *s = toupper((unsigned char) *s);
            s++;
         }
      }

      int method=get_token_id_by_string((char *)request_info->request_method);

      if(strcmp(tokens[0],"REST")==0) {
         int ret=mea_rest_api(conn,method,&tokens[1],l_tokens-1);
         if(ret==1) {
            _httpErrno(conn, 1, "not found");
            return 1;
         }
      }
      else if(l_tokens==2 && strcmp(tokens[0],"CMD")==0) {

         if(strcmp(tokens[1],"PING.PHP")==0 && method==HTTP_GET_ID) {
            return gui_ping(conn);
         }

      }

   }


   return 0;
}


int _stop_httpServer()
{
   if(g_mongooseContext)
      mg_stop(g_mongooseContext);
   
   return 0;
}


mea_error_t _start_httpServer(uint16_t port, char *home)
{
   if(port==0)
      port=8083;
   
   if(!home)
      return ERROR;
   
   val_listening_ports=(char *)malloc(6);
   val_listening_ports[5]=0;
   snprintf(val_listening_ports,5,"%d",port);
  
   int l_val_document_root=(int)strlen(home)+1; 
   val_document_root=malloc(l_val_document_root);
   val_document_root[l_val_document_root-1]=0;
   strncpy(val_document_root, home, l_val_document_root-1);
   
   options[ 0]=str_document_root;
   options[ 1]=(const char *)val_document_root;

   options[ 2]=str_listening_ports;
   options[ 3]=(const char *)val_listening_ports;

   options[ 4]=str_index_files;
   options[ 5]="index.html";

   options[12]=str_num_threads;
   options[13]="5";

   options[14]=NULL;
   
   struct mg_callbacks callbacks;
   memset(&callbacks,0,sizeof(struct mg_callbacks));
   callbacks.begin_request = _begin_request_handler;
   //   callbacks.open_file = open_file_handler;
   
   g_mongooseContext = mg_start(&callbacks, NULL, options);
   if (g_mongooseContext == NULL)
      return ERROR;
   else
      return NOERROR;
}


int stop_httpServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   _stop_httpServer();
   VERBOSE(1) mea_log_printf("%s (%s) : HTTPSERVER %s.\n", INFO_STR, __func__, stopped_successfully_str);
   return 0;
}


int start_httpServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   mea_error_t err=0;

   struct httpServerData_s *httpServerData = (struct httpServerData_s *)data;
//   char *phpcgibin=NULL;

   if(appParameters_get("HTML_PATH", httpServerData->params_list) &&
      appParameters_get("HTTP_PORT", httpServerData->params_list)) {

      _httpServer_monitoring_id=my_id;
      err=_start_httpServer(atoi(appParameters_get("HTTP_PORT", httpServerData->params_list)), appParameters_get("HTML_PATH", httpServerData->params_list));
      if(err==NOERROR) { 
         VERBOSE(2) mea_log_printf("%s (%s) : GUISERVER %s.\n", INFO_STR, __func__, launched_successfully_str);
         process_heartbeat(_httpServer_monitoring_id); // un premier heartbeat pour le faire au plus vite ...
         return 0;
      }
      else {
         VERBOSE(2) mea_log_printf("%s (%s) : GUISERVER can't be launched.\n", ERROR_STR, __func__);
         return -1;
      }
   }
   else {
      VERBOSE(3) mea_log_printf("%s (%s) : can't start GUI Server (parameters errors).\n",ERROR_STR,__func__);
   }
   
   VERBOSE(2) mea_log_printf("%s (%s) : GUISERVER can't be launched.\n", ERROR_STR, __func__);
   
   return -1;
}


int restart_httpServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   int ret=0;
   ret=stop_httpServer(my_id, data, errmsg, l_errmsg);
   if(ret==0) {
      return start_httpServer(my_id, data, errmsg, l_errmsg);
   }
   return ret;
}
