#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <netinet/tcp.h>

#include "mea_http_utils.h"
#include "mea_sockets_utils.h"

#include "mea_verbose.h"
#include "cJSON.h"
#include "mongoose.h"
#include "httpServer.h"


cJSON *getData_alloc(struct mg_connection *conn)
{
   cJSON *json=NULL;

   const char *cl=mg_get_header(conn, "Content-Length");
   if(cl) {
      int l_data=atoi(cl);
      if(l_data>0) {
         char *data=(char *)malloc(l_data+1);
         if(data) {
            mg_read(conn, data, l_data);
            data[l_data]=0;
            json = cJSON_Parse(data);
            free(data);
         }
      }
   }
   return json;
}


int returnResponseAndDeleteJsonData(struct mg_connection *conn, int httperr, int errnum, char *msg, cJSON *jsonData)
{
   int ret=returnResponse(conn, httperr, errnum, msg);
   if(jsonData)
      cJSON_Delete(jsonData);
   return ret;
}


int returnResponse(struct mg_connection *conn, int httperr, int errnum, char *msg)
{
   char buff[2048];

   if(msg!=NULL)
      snprintf(buff, sizeof(buff)-1, "{\"errno\":%d, \"msg\":\"%s\"}", errnum, msg); 
   else
      snprintf(buff, sizeof(buff)-1, "{\"errno\":%d}", errnum); 

   httpResponse(conn, httperr, NULL, buff);

   return 1;
}


int16_t httpGetResponseStatusCode(char *response, uint32_t l_response)
{
   char httpVersion[4], httpCodeText[33];
   int httpCodeNum;
   
   int n=sscanf(response,"HTTP/%3s %d %32[^\r]\r\n", httpVersion, &httpCodeNum, httpCodeText);
   if(n!=3)
      return -1;
   else
      return httpCodeNum;
}


char *httpRequest(uint8_t type, char *server, int port, char *url, char *data, uint32_t l_data, char *response, uint32_t *l_response, int16_t *nerr)
{
   static char *httpMethodes[]={"GET","POST","PUT","DELETE"};
   
   static const char *sprintf_get_delete_template = "%s %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n";
   static const char *sprintf_put_post_template = "%s %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\nContent-Length: %d\r\n\r\n";

   int sockfd = 0; // descripteur de socket
   
   // creation de la requete
   char *requete = NULL;

//   pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

   if(type==HTTP_GET || type==HTTP_DELETE) {

      requete = (char *)malloc( (strlen(sprintf_get_delete_template) - 6) + strlen(httpMethodes[type]) + strlen(url) + strlen(server) + 1 );
      if(!requete) {
         *nerr = HTTPREQUEST_ERR_MALLOC;
         DEBUG_SECTION {
            mea_log_printf("%s (%s) : malloc - n",ERROR_STR,__func__);
            perror("");
         }
         goto httpRequest_clean_exit;
      }
      sprintf(requete, sprintf_get_delete_template, httpMethodes[type], url, server);
   }
   else if(type==HTTP_PUT || type==HTTP_POST) {
      requete = (char *)malloc( (strlen(sprintf_put_post_template) - 8) + strlen(httpMethodes[type]) + strlen(url) + strlen(server) + 10 + 1 );
      if(!requete) {
         *nerr = HTTPREQUEST_ERR_MALLOC;
         DEBUG_SECTION {
            mea_log_printf("%s (%s) : malloc - n",ERROR_STR,__func__);
            perror("");
         }
         goto httpRequest_clean_exit;
      }
      sprintf(requete, sprintf_put_post_template, httpMethodes[type], url, server, l_data);
   }
   else {
      *nerr = HTTPREQUEST_ERR_METHODE;
      DEBUG_SECTION {
         mea_log_printf("%s (%s) : methode error\n",ERROR_STR,__func__);
      }
      goto httpRequest_clean_exit;
   }

//   pthread_cleanup_push( (void *)_free, (void *)requete );

//   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
//   pthread_testcancel();

   if(mea_socket_connect(&sockfd, server, port)<0) {
      *nerr = HTTPREQUEST_ERR_CONNECT;
      DEBUG_SECTION {
         mea_log_printf("%s (%s) : mea_socket_connect\n",ERROR_STR,__func__);
//         perror("");
      }
      goto httpRequest_clean_exit;
   }
   
   int ret=(int)send(sockfd, requete, strlen(requete), 0);
   if(ret < 0) {
      *nerr = HTTPREQUEST_ERR_SEND;
      DEBUG_SECTION {
         mea_log_printf("%s (%s) : send - can't send header : ",ERROR_STR,__func__);
         perror("");
      }
      goto httpRequest_clean_exit;
   }

   if(requete) {
      free(requete);
      requete=NULL;
   }
   
   if(type==HTTP_PUT || type==HTTP_POST) {
      int ret=(int)send(sockfd, data, l_data, 0);
      if(ret < 0) {
         *nerr = HTTPREQUEST_ERR_SEND;
         DEBUG_SECTION {
            mea_log_printf("%s (%s) : send - can't send body : ",ERROR_STR,__func__);
            perror("");
         }
         goto httpRequest_clean_exit;
      }
   }
   
   // reception de la reponse HTTP
   int s=0,n=0;
   char *ptr=response;
   int l=*l_response;
   do {
      fd_set input_set;
      struct timeval timeout;
      timeout.tv_sec = 1;
      timeout.tv_usec = 0;
   
      FD_ZERO(&input_set);
      FD_SET(sockfd, &input_set);
      s = select(sockfd+1, &input_set, NULL, NULL, &timeout); // avec select pour le timeout
      if (s > 0) {
         n=(int)recv(sockfd, ptr, l, 0);
         if(n == -1) {
            DEBUG_SECTION {
               mea_log_printf("%s (%s) : recv - ",ERROR_STR,__func__);
               perror("");
            }
            *nerr = HTTPREQUEST_ERR_SEND;
            goto httpRequest_clean_exit;
         }
      }
      else {
         if(s==0) {
           *nerr = HTTPREQUEST_ERR_TIMEOUT;
            mea_log_printf("%s (%s) : response timeout\n",ERROR_STR,__func__);
           goto httpRequest_clean_exit;
         }
         else {
            *nerr = HTTPREQUEST_ERR_SELECT;
            DEBUG_SECTION {
               mea_log_printf("%s (%s) : select - ",ERROR_STR,__func__);
               perror("");
            }
            goto httpRequest_clean_exit;
         }
      }
      ptr+=n;
      l=l-n;
   }
   while(n && l);
   
   *l_response = *l_response - l;
   
   // est-ce qu'on a vraiment tout lu ?
   if(!l_response) {
      char tmpbuff[1];
      n=(int)recv(sockfd, tmpbuff, sizeof(tmpbuff), MSG_PEEK | MSG_DONTWAIT); // on verifie que le buffer est vide
      if(n) { // on peut encore lire un caractère, le buffer n'est pas vide ... on a pas assez de place pour tout lire !
         *nerr = HTTPREQUEST_ERR_OVERFLOW;
         DEBUG_SECTION mea_log_printf("%s (%s) : response buffer overflow\n",ERROR_STR,__func__);
         goto httpRequest_clean_exit;
      }
   }
   else
      *ptr=0; // s'il reste de la place on termine par 1 zero
   
   // fermeture de la socket
   close(sockfd);
   sockfd=0;
   ptr = strstr(response, "\r\n\r\n");
   if(ptr)
      ptr+=4;
   return ptr; // on retourne 1 pointeur sur la zone de données de la requete http
  
//   pthread_cleanup_pop(1);

httpRequest_clean_exit:
   if(sockfd) {
      close(sockfd);
      sockfd=0;
   }
   if(requete) {
      free(requete);
      requete = NULL;
   }
   return NULL;
}