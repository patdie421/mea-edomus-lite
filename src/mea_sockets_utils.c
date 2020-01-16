//
//  sockets_utils.c
//  mea-edomus
//
//  Created by Patrice Dietsch on 16/10/2014.
//
//
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
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
   if( (arg = fcntl(soc, F_GETFL, NULL)) < 0) {
      VERBOSE(2) mea_log_printf ("%s (%s) : fcntl(..., F_GETFL) - %s\n", ERROR_STR, __func__, strerror(errno));
      return -1;
   }
   
   arg |= O_NONBLOCK;
   if( fcntl(soc, F_SETFL, arg) < 0) {
      VERBOSE(2) mea_log_printf ("%s (%s) : fcntl(..., F_SETFL) - %s\n", ERROR_STR, __func__, strerror(errno));
      return -1;
   }
   
   // Trying to connect with timeout
   res = connect(soc, addr, addr_l);
   if (res < 0) {
      if(errno == EINPROGRESS) {
//         fprintf(stderr, "EINPROGRESS in connect() - selecting\n");
         do {
            tv.tv_sec = 2;
            tv.tv_usec = 0;
            FD_ZERO(&myset);
            FD_SET(soc, &myset);
            res = select(soc+1, NULL, &myset, NULL, &tv);
            if (res < 0 && errno != EINTR) {
               VERBOSE(2) mea_log_printf ("%s (%s) : select() - %s\n", ERROR_STR, __func__, strerror(errno));
               return -1;
            }
            else if (res > 0) {
               // Socket selected for write
               socklen_t l = sizeof(int);
               if (getsockopt(soc, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &l) < 0) {
                  VERBOSE(2) mea_log_printf ("%s (%s) : getsockopt() - %s\n", ERROR_STR, __func__, strerror(errno));
                  return -1;
               }
               // Check the value returned...
               if (valopt) {
                  VERBOSE(2) mea_log_printf ("%s (%s) : getsockopt(), valopt=%d - %s\n", ERROR_STR, __func__, valopt, strerror(valopt));
                  return -1;
               }
               break;
            }
            else {
               VERBOSE(2) mea_log_printf ("%s (%s) : select() - %s\n", ERROR_STR, __func__, strerror(errno));
               return -1;
            }
         }
         while(1);
      }
      else {
         VERBOSE(2) mea_log_printf ("%s (%s) : connect() - %s\n", ERROR_STR, __func__, strerror(errno));
         return -1;
      }
   }
   
   // Set to blocking mode again...
   if( (arg = fcntl(soc, F_GETFL, NULL)) < 0) {
      VERBOSE(2) mea_log_printf ("%s (%s) : fcntl(..., F_GETFL) - %s\n", ERROR_STR, __func__, strerror(errno));
      return -1;
   }
   
   arg &= (~O_NONBLOCK);
   if( fcntl(soc, F_SETFL, arg) < 0) {
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
   if(sock < 0) {
      DEBUG_SECTION {
         char err_str[256];
         strerror_r(errno, err_str, sizeof(err_str)-1);
         mea_log_printf("%s (%s) :  socket - can't create, %s\n", ERROR_STR, __func__,err_str);
      }
      return -1;
   }
   
   serv_info = gethostbyname(hostname); // on récupère les informations de l'hôte auquel on veut se connecter
   if(serv_info == NULL) {
      DEBUG_SECTION {
         char err_str[256];
         strerror_r(errno, err_str, sizeof(err_str)-1);
         mea_log_printf("%s (%s) :  gethostbyname - can't get information, %s\n", ERROR_STR, __func__, err_str);
      }
      return -1;
   }

   bzero((char *)&serv_addr, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_port   = htons(port);
   bcopy((char *)serv_info->h_addr, (char *)&serv_addr.sin_addr.s_addr, serv_info->h_length);
   
   if(mea_connect(sock, (const struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
      DEBUG_SECTION {
         mea_log_printf("%s (%s) :  connect - can't connect (hostname=%s, port=%d)\n", ERROR_STR, __func__, hostname, port);
      }
      close(sock);
      return -1;
   }

   *s=sock;
   return 0;
}


int mea_socket_send(int *s, char *message, int l_message)
{
   int ret;
   
   ret=(int)send(*s, message, l_message, 0);

   if(ret < 0) {
      DEBUG_SECTION {
         char err_str[256];
         strerror_r(errno, err_str, sizeof(err_str)-1);
         mea_log_printf("%s (%s) :  send - can't send, %s\n",ERROR_STR,__func__,err_str);
      }
      return -1;
   }
   
   return 0;
}


int mea_socket_read(int *s, char *message, int l_message, int t)
{
   fd_set input_set;
   struct timeval timeout;
   char buff[80];
   ssize_t l=0;
   
   int16_t ret=0;
   if(t) {
      timeout.tv_sec  = t;
      timeout.tv_usec = 0;
   
      FD_ZERO(&input_set);
      FD_SET(*s, &input_set);
   
      ret = (int16_t)select(*s+1, &input_set, NULL, NULL, &timeout);
   }

   if(ret>=0) {
      l = recv(*s, buff, sizeof(buff), 0);
      if(l<0) {
         ret=-1;
      }
   }

   if(ret<0) {
      return -1;
   }
      
   if(l>l_message) {
      memcpy(message, buff, l_message);
      return l_message;
   }
   else {
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
