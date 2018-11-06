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
#include "guiServer.h"
#include "mongoose.h"
#include "mea_string_utils.h"
#include "mea_queue.h"
#include "xPLServer.h"
#include "tokens.h"
#include "init.h"
#include "mea_sockets_utils.h"
#include "cJSON.h"
#include "mea_rest_api.h"

#include "processManager.h"
#include "automator.h"
#include "automatorServer.h"


const char *str_document_root="document_root";
const char *str_listening_ports="listening_ports";
const char *str_index_files="index_files";
const char *str_cgi_interpreter="cgi_interpreter";
const char *str_cgi_environment="cgi_environment";
const char *str_cgi_pattern="cgi_pattern";
const char *str_num_threads="num_threads";

char *val_document_root=NULL;
char *val_listening_ports=NULL;
char *val_index_files=NULL;
char *val_cgi_interpreter=NULL;
char *val_cgi_environment=NULL;
char *val_cgi_pattern=NULL;
char *val_num_threads=NULL;


// Variable globale privée
int  _httpServer_monitoring_id=-1;
char _php_sessions_path[256];


int httpin_indicator=0; // nombre de demandes recues
int httpout_indicator=0; // nombre de réponses émises

const char *options[15];
struct mg_context* g_mongooseContext = 0;


int gethttp(char *server, int port, char *url, char *response, int l_response)
{
   int sockfd; // descripteur de socket
   
   // creation de la requete
   char requete[1000]="GET ";
   strcat(requete, url);
   strcat(requete, " HTTP/1.1\r\n");
   strcat(requete, "Host: ");
   strcat(requete, server);
   strcat(requete, "\r\n");
   strcat(requete, "Connection: close\r\n");
   strcat(requete, "\r\n");
   
   if(mea_socket_connect(&sockfd, server, port)<0)
   {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : mea_socket_connect - ", ERROR_STR,__func__);
         perror("");
      }
      return -1;
   }
   
   if(mea_socket_send(&sockfd, requete, (int)strlen(requete))<0)
   {
      close(sockfd);
      return -1;
   }
   
   // reception de la reponse HTTP
   int n=(int)recv(sockfd, response, l_response, 0);
   if(n == -1)
   {
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


int _findPHPVar(char *phpserial, char *var, int *type, char *value, int l_value)
{
   char *_toFind;
   char _type[2];
   char _v1[17];
   char _v2[256];
   
   _toFind=(char *)alloca(strlen(var)+2);
   if(!_toFind)
      return -1;
   sprintf(_toFind,"%s|",var);
   char *str_ptr=strstr(phpserial,_toFind)+strlen(_toFind);
//   free(_toFind);
   _toFind=NULL;
   
   if(!str_ptr)
      return -1;
   
   int n=sscanf(str_ptr,"%1[^:;]:%16[^:;]:%255[^:;]",_type,_v1,_v2);
   if(n<2)
      return -1;
   
   *type=0;
   if(strcmp(_type,"i")==0)
   {
      *type=1;
      strncpy(value,_v1,l_value-1);
   }
   else if(strcmp(_type,"s")==0)
   {
      *type=2;
      int l=atoi(_v1)+1;
      if(l > l_value)
         l=l_value;
      
      strncpy(value,&_v2[1],l-1);
   }
   else
      return -1;
   
   return 0;
}


int _phpsessid_check_loggedin_and_admin(char *phpsessid,char *sessions_files_path)
{
   int ret=0;
//   char session_file[256];
   char *session_file;
   
   session_file=alloca(strlen(sessions_files_path)+6 /* len of "/sess_" */ +strlen(phpsessid)+1);
   if(!session_file)
      return -1;
   sprintf(session_file, "%s/sess_%s", sessions_files_path, phpsessid);
  
   char *buff=NULL;
   struct stat st;
   
   if (stat(session_file, &st) == 0)
   {
      buff=(char *)alloca(st.st_size+1);
      if(!buff)
         return -1;
   }
   else
      return -1;
   
   FILE *fd;
   fd=fopen(session_file, "r");
   if(fd)
   {
      int n = (int)fread(buff, 1, st.st_size, fd);
      fclose(fd);
      if(!n)
      {
         ret=-1;
         goto _phpsessid_check_loggedin_and_admin_clean_exit;
      }
      buff[n]='\0'; // pour eviter les problèmes ...
      
      int logged_in = -1;
      int admin = -1;
      
      int type = -1;
      char value[81];
      if(_findPHPVar(buff, "logged_in", &type, value, 80)==0)
      {
         if(type == 1)
            logged_in=atoi(value);
         else
         {
            ret=-1;
            goto _phpsessid_check_loggedin_and_admin_clean_exit;
         }
      }
      else
      {
         ret=-1;
         goto _phpsessid_check_loggedin_and_admin_clean_exit;
      }
      if(_findPHPVar(buff, "profil", &type, value, 80)==0)
      {
         if(type==1)
            admin=atoi(value);
         else
         {
            ret=-1;
            goto _phpsessid_check_loggedin_and_admin_clean_exit;
         }
      }
      else
      {
         ret=-1;
         goto _phpsessid_check_loggedin_and_admin_clean_exit;
      }
      
      if(logged_in==1 && admin==1)
         ret=1;
      else
      {
         if(logged_in==1)
            ret=0;
         else
            ret=-1;
      }
   }
   else
   {
      VERBOSE(5) {
         mea_log_printf("%s (%s) : cannot read %s file - ",ERROR_STR,__func__,session_file);
         perror("");
      }
      ret=-1;
   }
_phpsessid_check_loggedin_and_admin_clean_exit:
   return ret;
}


#define MAX_BUFFER_SIZE 80
#define MAX_TOKEN 20

#define REQUESTERRNO_NOTAUTHORIZED 2
#define REQUESTERRNO_METHODEERR    3
#define REQUESTERRNO_CMNDERR       4
#define REQUESTERRNO_INTERNALERR   5
#define REQUESTERRNO_OPERERR       7


int gui_ping(struct mg_connection *conn, char *phpsessid, char *_php_sessions_path)
{
   process_heartbeat(_httpServer_monitoring_id); // le heartbeat est fait de l'extérieur ...
      
   _httpErrno(conn, 0, NULL);
   return 1;
}


static int _begin_request_handler(struct mg_connection *conn)
{
//   uint32_t id = 0;
   const struct mg_request_info *request_info = mg_get_request_info(conn);
   char phpsessid[80];
   
   httpin_indicator++;
   process_update_indicator(_httpServer_monitoring_id, "HTTPIN", httpin_indicator);

   const char *authorization=mg_get_header(conn, "Authorization");

   char *uri=alloca(strlen((char *)request_info->uri)+1);
   strcpy(uri,request_info->uri);

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
            _httpErrno(conn, 0, NULL);
            return 1;
         }
      }
      else if(l_tokens==2 && strcmp(tokens[0],"CMD")==0) {

         if(strcmp(tokens[1],"PING.PHP")==0 && method==HTTP_GET_ID) {
            const char *cookie = mg_get_header(conn, "Cookie");
            if(mg_get_cookie(cookie, "PHPSESSID", phpsessid, sizeof(phpsessid))>0) {
            }
            else
               phpsessid[0]=0;
            return gui_ping(conn, phpsessid, _php_sessions_path);
         }

      }

   }


   return 0;
}


int stop_httpServer()
{
   if(g_mongooseContext)
      mg_stop(g_mongooseContext);
   
   return 0;
}


mea_error_t start_httpServer(uint16_t port, char *home, char *php_cgi, char *php_ini_path)
{
   if(port==0)
      port=8083;
   
   if(!home || !php_cgi || !php_ini_path)
      return ERROR;
   
   val_listening_ports=(char *)malloc(6);
   sprintf(val_listening_ports,"%d",port);
   
   val_document_root=malloc(strlen(home)+1);
   strcpy(val_document_root,home);
   
   val_cgi_interpreter=malloc(strlen(php_cgi)+1);
   strcpy(val_cgi_interpreter,php_cgi);
   
   val_cgi_environment=malloc(6+strlen(php_ini_path)+1);
   sprintf(val_cgi_environment,"PHPRC=%s", php_ini_path);
   
   options[ 0]=str_document_root;
   options[ 1]=(const char *)val_document_root;
   options[ 2]=str_listening_ports;
   options[ 3]=(const char *)val_listening_ports;
   options[ 4]=str_index_files;
   options[ 5]="index.php";
   options[ 6]=str_cgi_interpreter;
   options[ 7]=(const char *)val_cgi_interpreter;
   options[ 8]=str_cgi_environment;
   options[ 9]=(const char *)val_cgi_environment;
   options[10]=str_cgi_pattern;
   options[11]="**.php$";
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


int stop_guiServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   stop_httpServer();
   VERBOSE(1) mea_log_printf("%s (%s) : HTTPSERVER %s.\n", INFO_STR, __func__, stopped_successfully_str);
   return 0;
}


int start_guiServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   struct httpServerData_s *httpServerData = (struct httpServerData_s *)data;
   char *phpcgibin=NULL;
   if(appParameters_get("PHPCGIPATH", httpServerData->params_list) &&
      appParameters_get("PHPINIPATH", httpServerData->params_list) &&
      appParameters_get("GUIPATH", httpServerData->params_list) &&
      appParameters_get("SQLITE3DBPARAMPATH", httpServerData->params_list))
   {
      phpcgibin=(char *)malloc(strlen(appParameters_get("PHPCGIPATH", httpServerData->params_list))+10); // 9 = strlen("/cgi-bin") + 1
      
      sprintf(phpcgibin, "%s/php-cgi",appParameters_get("PHPCGIPATH", httpServerData->params_list));
      long guiport;
      if(appParameters_get("GUIPORT", httpServerData->params_list)[0])
      {
         char *end;
         guiport=strtol(appParameters_get("GUIPORT", httpServerData->params_list),&end,10);
         if(*end!=0 || errno==ERANGE)
         {
            VERBOSE(9) mea_log_printf("%s (%s) : GUI port (%s), not a number, 8083 will be used.\n", INFO_STR, __func__, appParameters_get("GUIPORT", httpServerData->params_list));
            guiport=8083;
         }
      }
      else
      {
         VERBOSE(9) mea_log_printf("%s (%s) : can't get GUI port, 8083 will be used.\n", INFO_STR, __func__);
         guiport=8083;
      }
      
      strncpy(_php_sessions_path, appParameters_get("PHPSESSIONSPATH", httpServerData->params_list), sizeof(_php_sessions_path)-1);

      if(create_configs_php(appParameters_get("MEAPATH", httpServerData->params_list),
                            appParameters_get("GUIPATH", httpServerData->params_list),
			    appParameters_get("SQLITE3DBPARAMPATH", httpServerData->params_list),
                            appParameters_get("LOGPATH", httpServerData->params_list),
                            appParameters_get("PHPSESSIONSPATH", httpServerData->params_list))==0)
      {
         _httpServer_monitoring_id=my_id;
         start_httpServer(guiport, appParameters_get("GUIPATH", httpServerData->params_list) , phpcgibin, appParameters_get("PHPINIPATH", httpServerData->params_list));
         
         if(phpcgibin)
         {
            free(phpcgibin);
            phpcgibin=NULL;
         }
         
         VERBOSE(2) mea_log_printf("%s (%s) : GUISERVER %s.\n", INFO_STR, __func__, launched_successfully_str);
         process_heartbeat(_httpServer_monitoring_id); // un premier heartbeat pour le faire au plus vite ...
         return 0;
      }
      else
      {
         VERBOSE(3) mea_log_printf("%s (%s) : can't start GUI Server (can't create configs.php).\n",ERROR_STR,__func__);
      }
      free(phpcgibin);
      phpcgibin=NULL;
   }
   else
   {
      VERBOSE(3) mea_log_printf("%s (%s) : can't start GUI Server (parameters errors).\n",ERROR_STR,__func__);
   }
   
   VERBOSE(2) mea_log_printf("%s (%s) : GUISERVER can't be launched.\n", ERROR_STR, __func__);
   
   return -1;
}


int restart_guiServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   int ret=0;
   ret=stop_guiServer(my_id, data, errmsg, l_errmsg);
   if(ret==0)
   {
      return start_guiServer(my_id, data, errmsg, l_errmsg);
   }
   return ret;
}
