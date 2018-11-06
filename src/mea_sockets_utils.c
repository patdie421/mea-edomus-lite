//
//  sockets_utils.c
//  mea-edomus
//
//  Created by Patrice Dietsch on 16/10/2014.
//
//
#include <stdio.h>
#include <stdlib.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // close
#include <netdb.h> // gethostbyname
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>

#include "mea_sockets_utils.h"

#include "mea_verbose.h"


int mea_connect(int soc, const struct sockaddr *addr, socklen_t addr_l)
{
   int res;
   long arg;
   fd_set myset;
   struct timeval tv;
   int valopt;
   
   // Set non-blocking
   if( (arg = fcntl(soc, F_GETFL, NULL)) < 0)
   {
      VERBOSE(2) mea_log_printf ("%s (%s) : fcntl(..., F_GETFL) - %s\n", ERROR_STR, __func__, strerror(errno));
      return -1;
   }
   
   arg |= O_NONBLOCK;
   if( fcntl(soc, F_SETFL, arg) < 0)
   {
      VERBOSE(2) mea_log_printf ("%s (%s) : fcntl(..., F_SETFL) - %s\n", ERROR_STR, __func__, strerror(errno));
      return -1;
   }
   
   // Trying to connect with timeout
   res = connect(soc, addr, addr_l);
   if (res < 0)
   {
      if(errno == EINPROGRESS)
      {
         //        fprintf(stderr, "EINPROGRESS in connect() - selecting\n");
         do
         {
            tv.tv_sec = 2;
            tv.tv_usec = 0;
            FD_ZERO(&myset);
            FD_SET(soc, &myset);
            res = select(soc+1, NULL, &myset, NULL, &tv);
            if (res < 0 && errno != EINTR)
            {
               VERBOSE(2) mea_log_printf ("%s (%s) : select() - %s\n", ERROR_STR, __func__, strerror(errno));
               return -1;
            }
            else if (res > 0)
            {
               // Socket selected for write
               socklen_t l = sizeof(int);
               if (getsockopt(soc, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &l) < 0)
               {
                  VERBOSE(2) mea_log_printf ("%s (%s) : getsockopt() - %s\n", ERROR_STR, __func__, strerror(errno));
                  return -1;
               }
               // Check the value returned...
               if (valopt)
               {
                  VERBOSE(2) mea_log_printf ("%s (%s) : getsockopt(), valopt=%d - %s\n", ERROR_STR, __func__, valopt, strerror(valopt));
                  return -1;
               }
               break;
            }
            else
            {
               VERBOSE(2) mea_log_printf ("%s (%s) : select() - %s\n", ERROR_STR, __func__, strerror(errno));
               return -1;
            }
         }
         while(1);
      }
      else
      {
         VERBOSE(2) mea_log_printf ("%s (%s) : connect() - %s\n", ERROR_STR, __func__, strerror(errno));
         return -1;
      }
   }
   
   // Set to blocking mode again...
   if( (arg = fcntl(soc, F_GETFL, NULL)) < 0)
   {
      VERBOSE(2) mea_log_printf ("%s (%s) : fcntl(..., F_GETFL) - %s\n", ERROR_STR, __func__, strerror(errno));
      return -1;
   }
   
   arg &= (~O_NONBLOCK);
   if( fcntl(soc, F_SETFL, arg) < 0)
   {
      VERBOSE(2) mea_log_printf ("%s (%s) : fcntl(..., F_SETFL) - %s\n", ERROR_STR, __func__, strerror(errno));
      return -1;
   } 
   
   return 0;
}


int mea_socket_connect(int *s, char *hostname, int port)
{
   int sock;
   struct sockaddr_in serv_addr;
   struct hostent *serv_info = NULL;

   *s=0;
   
   sock = socket(AF_INET, SOCK_STREAM, 0);
   if(sock < 0)
   {
      DEBUG_SECTION {
         mea_log_printf("%s (%s) :  socket - can't create : ", ERROR_STR, __func__);
         perror("");
      }
      return -1;
   }
   
   serv_info = gethostbyname(hostname); // on récupère les informations de l'hôte auquel on veut se connecter
   if(serv_info == NULL)
   {
      DEBUG_SECTION {
         mea_log_printf("%s (%s) :  gethostbyname - can't get information : ", ERROR_STR, __func__);
         perror("");
      }
      return -1;
   }

   bzero((char *)&serv_addr, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_port   = htons(port);
   bcopy((char *)serv_info->h_addr, (char *)&serv_addr.sin_addr.s_addr, serv_info->h_length);
   
//   if(connect(sock, (const struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
   if(mea_connect(sock, (const struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
   {
      DEBUG_SECTION {
         mea_log_printf("%s (%s) :  connect - can't connect (hostname=%s, port=%d)\n", ERROR_STR, __func__, hostname, port);
//         perror("");
      }
      close(sock);
      return -1;
   }

   *s=sock;
   return 0;
}


pthread_mutex_t mea_socket_send_lock;
int mea_socket_send_lock_is_init=0;
int mea_socket_send(int *s, char *message, int l_message)
{
   int ret;
   
   if(mea_socket_send_lock_is_init==0)
   {
      pthread_mutex_init(&mea_socket_send_lock, NULL);
      mea_socket_send_lock_is_init=1;
   }
   
   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&mea_socket_send_lock );
   pthread_mutex_lock(&mea_socket_send_lock);
   
   ret=(int)send(*s, message, l_message, 0);

   pthread_mutex_unlock(&mea_socket_send_lock);
   pthread_cleanup_pop(0);

   if(ret < 0)
   {
      DEBUG_SECTION {
         mea_log_printf("%s (%s) :  send - can't send : ",ERROR_STR,__func__);
         perror("");
      }
      return -1;
   }
   
   return 0;
}


pthread_mutex_t mea_socket_read_lock;
int mea_socket_read_lock_is_init=0;
int mea_socket_read(int *s, char *message, int l_message, int t)
{
   fd_set input_set;
   struct timeval timeout;
   char buff[80];
   ssize_t l=0;
   
   if(mea_socket_read_lock_is_init==0)
   {
      pthread_mutex_init(&mea_socket_read_lock, NULL);
      mea_socket_read_lock_is_init=1;
   }

   int16_t ret=0;
   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&mea_socket_read_lock );
   pthread_mutex_lock(&mea_socket_read_lock);
   if(t)
   {
      timeout.tv_sec  = t;
      timeout.tv_usec = 0;
   
      FD_ZERO(&input_set);
      FD_SET(*s, &input_set);
   
      ret = (int16_t)select(*s+1, &input_set, NULL, NULL, &timeout);
   }

   if(ret>=0)
   {
      l = recv(*s, buff, sizeof(buff), 0);
      if(l<0)
         ret=-1;
   }
   pthread_mutex_unlock(&mea_socket_read_lock);
   pthread_cleanup_pop(0);

   if(ret<0)
      return -1;
      
   if(l>l_message)
   {
      memcpy(message, buff, l_message);
      return l_message;
   }
   else
   {
      memcpy(message, buff, (int)l);
      return (int)l;
   }
}


void *_free(void *r)
{
   if(r)
      free(r);
   return NULL;
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

   if(type==HTTP_GET || type==HTTP_DELETE)
   {

      requete = (char *)malloc( (strlen(sprintf_get_delete_template) - 6) + strlen(httpMethodes[type]) + strlen(url) + strlen(server) + 1 );
      if(!requete)
      {
         *nerr = HTTPREQUEST_ERR_MALLOC;
         DEBUG_SECTION {
            mea_log_printf("%s (%s) : malloc - n",ERROR_STR,__func__);
            perror("");
         }
         goto httpRequest_clean_exit;
      }
      sprintf(requete, sprintf_get_delete_template, httpMethodes[type], url, server);
   }
   else if(type==HTTP_PUT || type==HTTP_POST)
   {
      requete = (char *)malloc( (strlen(sprintf_put_post_template) - 8) + strlen(httpMethodes[type]) + strlen(url) + strlen(server) + 10 + 1 );
      if(!requete)
      {
         *nerr = HTTPREQUEST_ERR_MALLOC;
         DEBUG_SECTION {
            mea_log_printf("%s (%s) : malloc - n",ERROR_STR,__func__);
            perror("");
         }
         goto httpRequest_clean_exit;
      }
      sprintf(requete, sprintf_put_post_template, httpMethodes[type], url, server, l_data);
   }
   else
   {
      *nerr = HTTPREQUEST_ERR_METHODE;
      DEBUG_SECTION {
         mea_log_printf("%s (%s) : methode error\n",ERROR_STR,__func__);
      }
      goto httpRequest_clean_exit;
   }

   pthread_cleanup_push( (void *)_free, (void *)requete );

   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
   pthread_testcancel();

   if(mea_socket_connect(&sockfd, server, port)<0)
   {
      *nerr = HTTPREQUEST_ERR_CONNECT;
      DEBUG_SECTION {
         mea_log_printf("%s (%s) : mea_socket_connect\n",ERROR_STR,__func__);
//         perror("");
      }
      goto httpRequest_clean_exit;
   }
   
   int ret=(int)send(sockfd, requete, strlen(requete), 0);
   if(ret < 0)
   {
      *nerr = HTTPREQUEST_ERR_SEND;
      DEBUG_SECTION {
         mea_log_printf("%s (%s) : send - can't send header : ",ERROR_STR,__func__);
         perror("");
      }
      goto httpRequest_clean_exit;
   }

   if(requete)
   {
      free(requete);
      requete=NULL;
   }
   
   if(type==HTTP_PUT || type==HTTP_POST)
   {
      int ret=(int)send(sockfd, data, l_data, 0);
      if(ret < 0)
      {
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
   do
   {
      fd_set input_set;
      struct timeval timeout;
      timeout.tv_sec = 1;
      timeout.tv_usec = 0;
   
      FD_ZERO(&input_set);
      FD_SET(sockfd, &input_set);
      s = select(sockfd+1, &input_set, NULL, NULL, &timeout); // avec select pour le timeout
      if (s > 0)
      {
         n=(int)recv(sockfd, ptr, l, 0);
         if(n == -1)
         {
            DEBUG_SECTION {
               mea_log_printf("%s (%s) : recv - ",ERROR_STR,__func__);
               perror("");
            }
            *nerr = HTTPREQUEST_ERR_SEND;
            goto httpRequest_clean_exit;
         }
      }
      else
      {
         if(s==0)
         {
           *nerr = HTTPREQUEST_ERR_TIMEOUT;
            mea_log_printf("%s (%s) : response timeout\n",ERROR_STR,__func__);
           goto httpRequest_clean_exit;
         }
         else
         {
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
   if(!l_response)
   {
      char tmpbuff[1];
      n=(int)recv(sockfd, tmpbuff, sizeof(tmpbuff), MSG_PEEK | MSG_DONTWAIT); // on verifie que le buffer est vide
      if(n) // on peut encore lire un caractère, le buffer n'est pas vide ... on a pas assez de place pour tout lire !
      {
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
  
   pthread_cleanup_pop(1);

httpRequest_clean_exit:
   if(sockfd)
   {
      close(sockfd);
      sockfd=0;
   }
   if(requete)
   {
      free(requete);
      requete = NULL;
   }
   return NULL;
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
