//
//  mea_xpl.c
//
//  Created by Patrice DIETSCH on 12/31/16.
//  Copyright (c) 2012 -. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <inttypes.h>
#include <time.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#include "cJSON.h"
#include "mea_timer.h"
#include "mea_verbose.h"
#include "tokens.h"
#include "tokens_da.h"

#define XPLPORT 3865

#define XPL_VERSION "0.1a2"


/* Create a socket for broadcasting messages */
int mea_xPLConnectBroadcast(char *xPLInterfaceName, char *ip, int l_ip, struct sockaddr_in *xPLBroadcastAddr) {
   int sockfd;
   int flag = 1;
   struct protoent *ppe;
   struct ifreq interfaceInfo;
   struct in_addr interfaceNetmask;
   struct in_addr xPLInterfaceAddr; // Address associated with interface
   
   // Map protocol name
   if ((ppe = getprotobyname("udp")) == 0) {
      VERBOSE(2) mea_log_printf("%s (%s) : unable to lookup UDP protocol info\n", ERROR_STR, __func__);
      return -1;
   }
   
   // Attempt to create a socket
   errno=0;
   if ((sockfd = socket(AF_INET, SOCK_DGRAM, ppe->p_proto)) < 0) {
      VERBOSE(2) mea_log_printf("%s (%s) : unable to create broadcast socket : %s (%d)\n", ERROR_STR, __func__, strerror(errno), errno);
      return -1;
   }
   
   // Mark as a broadcasting socket
   if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &flag, (socklen_t)sizeof(flag)) < 0) {
      VERBOSE(2) mea_log_printf("%s (%s) : unable to set SO_BROADCAST on socket : %s (%d)\n", ERROR_STR, __func__, strerror(errno), errno);
      if(close(sockfd)==-1) {
         VERBOSE(2) mea_log_printf("%s (%s) : can't close socket : %s (%d)\n", ERROR_STR, __func__, strerror(errno), errno);
      }
      return -1;
   }
   
   // Init the interface info request
   bzero(&interfaceInfo, sizeof(struct ifreq));
   interfaceInfo.ifr_addr.sa_family = AF_INET;
   strncpy(interfaceInfo.ifr_name, xPLInterfaceName,sizeof(interfaceInfo.ifr_name));
   
   // Get our interface address
   if (ioctl(sockfd, SIOCGIFADDR, &interfaceInfo) != 0) {
      VERBOSE(2) mea_log_printf("%s (%s) : unable to get IP addr for interface %s\n", ERROR_STR, __func__, xPLInterfaceName);
      if(close(sockfd)==-1) {
         VERBOSE(2) mea_log_printf("%s (%s) : can't close socket : %s (%d)\n", ERROR_STR, __func__, strerror(errno), errno);
      }
      return -1;
   }
   xPLInterfaceAddr.s_addr = ((struct sockaddr_in *) &interfaceInfo.ifr_addr)->sin_addr.s_addr;
   strncpy(ip, inet_ntoa(xPLInterfaceAddr), (size_t)l_ip);
   ip[l_ip-1]='\0';
   VERBOSE(5) mea_log_printf("%s (%s) : auto-assigning IP address of %s\n", INFO_STR, __func__, ip);
   
   // Get interface netmask
   bzero(&interfaceInfo, sizeof(struct ifreq));
   interfaceInfo.ifr_addr.sa_family = AF_INET;
   interfaceInfo.ifr_broadaddr.sa_family = AF_INET;
   strcpy(interfaceInfo.ifr_name, xPLInterfaceName);
   if(ioctl(sockfd, SIOCGIFNETMASK, &interfaceInfo) != 0) {
      VERBOSE(2) mea_log_printf("%s (%s) : unable to extract the interface net mask\n", ERROR_STR, __func__);
      if(close(sockfd)==-1) {
         VERBOSE(2) mea_log_printf("%s (%s) : can't close socket : %s (%d)\n", ERROR_STR, __func__, strerror(errno), errno);
      }
      return -1;
   }

#ifndef __APPLE__
   interfaceNetmask.s_addr = ((struct sockaddr_in *) &interfaceInfo.ifr_netmask)->sin_addr.s_addr;
#else
   interfaceNetmask.s_addr = ((struct sockaddr_in *) &interfaceInfo.ifr_addr)->sin_addr.s_addr;
#endif
   DEBUG_SECTION mea_log_printf("%s (%s) : NETMASK=%x\n",DEBUG_STR, __func__, interfaceNetmask.s_addr);

   // Build our broadcast addr
   bzero(xPLBroadcastAddr, sizeof(struct sockaddr_in));
   xPLBroadcastAddr->sin_family = AF_INET;
   xPLBroadcastAddr->sin_addr.s_addr = xPLInterfaceAddr.s_addr | ~interfaceNetmask.s_addr;
   xPLBroadcastAddr->sin_port = htons(XPLPORT);
   
   /*
    // markNonblocking;
    int fcntlret=fcntl(sockfd, F_GETFL, 0);
    if(fcntlret != -1)
    fcntl(sockfd, F_SETFL, fcntlret|O_NONBLOCK);
    */
   
   // And we are done
   VERBOSE(2) mea_log_printf("%s (%s) : assigned xPL Broadcast address of %s, port %d\n", ERROR_STR, __func__, inet_ntoa(xPLBroadcastAddr->sin_addr), XPLPORT);
   
   return sockfd;
}


/* Attempt to make a hub based connection */
int mea_xPLConnectHub(int *xPLPort)
{
   int sockfd;
   int flag = 1;
   int sockSize = (int)sizeof(struct sockaddr_in);
   struct protoent *ppe;
   struct sockaddr_in theSockInfo;
   
   *xPLPort = 0;
   /* Init the socket definition */
   bzero(&theSockInfo, sizeof(theSockInfo));
   theSockInfo.sin_family = AF_INET;
   theSockInfo.sin_addr.s_addr = INADDR_ANY;
   theSockInfo.sin_port = htons(0);
   
   /* Map protocol name */
   if ((ppe = getprotobyname("udp")) == 0) {
      VERBOSE(2) mea_log_printf("%s (%s) : unable to lookup UDP protocol info\n", ERROR_STR, __func__);
      return -1;
   }
   
   /* Attempt to creat the socket */
   if ((sockfd = socket(PF_INET, SOCK_DGRAM, ppe->p_proto)) < 0) {
      VERBOSE(2) mea_log_printf("%s (%s) : unable to create listener socket %s (%d)\n", ERROR_STR, __func__, strerror(errno), errno);
      return -1;
   }
   
   /* Mark as a broadcast socket */
   if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &flag, (socklen_t)sizeof(flag)) < 0) {
      VERBOSE(2) mea_log_printf("%s (%s) : unable to set SO_BROADCAST on socket %s (%d)\n", ERROR_STR, __func__, strerror(errno), errno);
      if(close(sockfd)==-1) {
         VERBOSE(2) mea_log_printf("%s (%s) : Can't close socket : %s (%d)\n", ERROR_STR, __func__, strerror(errno), errno);
      }
      return -1;
   }
   
   /* Attempt to bind */
   if ((bind(sockfd, (struct sockaddr *)&theSockInfo, (socklen_t)sockSize)) < 0) {
      VERBOSE(2) mea_log_printf("%s (%s) : unable to bind listener socket to port %d, %s (%d)\n", ERROR_STR, __func__, XPLPORT, strerror(errno), errno);
      if(close(sockfd)==-1) {
         VERBOSE(2) mea_log_printf("%s (%s) : can't close socket : %s (%d)\n", ERROR_STR, __func__, strerror(errno), errno);
      }
      return -1;
   }
   
   /* Fetch the actual socket port # */
   if (getsockname(sockfd, (struct sockaddr *) &theSockInfo, (socklen_t *) &sockSize)!=0) {
      VERBOSE(2) mea_log_printf("%s (%s) : unable to fetch socket info for bound listener, %s (%d)\n", ERROR_STR, __func__, strerror(errno), errno);
      if(close(sockfd)==-1) {
         VERBOSE(2) mea_log_printf("%s (%s) : can't close socket : %s (%d)\n", ERROR_STR, __func__, strerror(errno), errno);
      }
      return -1;
   }
   
   /* We are ready to go */
   *xPLPort = (int)ntohs(theSockInfo.sin_port);
   
   /*
    // markNonblocking;
    int fcntlret=fcntl(sockfd, F_GETFL, 0);
    if(fcntlret != -1)
    fcntl(sockfd, F_SETFL, fcntlret|O_NONBLOCK);
    */
   //   maximizeReceiveBufferSize(sockfd);
   
   VERBOSE(5) mea_log_printf("%s (%s) : xPL Starting in Hub mode on port %d\n", INFO_STR, __func__, *xPLPort);
   
   return sockfd;
}


int16_t mea_xPLSendMessage(int fd, struct sockaddr_in *xPLBroadcastAddr, char *theData, int dataLen) {
   int bytesSent;
   
   /* Try to send the message */
   if ((bytesSent = (int)sendto(fd, theData, (size_t)dataLen, 0,
                                (struct sockaddr *) xPLBroadcastAddr, (socklen_t)sizeof(struct sockaddr_in))) != dataLen) {
      VERBOSE(2) mea_log_printf("%s (%s) : unable to broadcast message, %s (%d)\n", ERROR_STR, __func__,strerror(errno), errno);
      return -1;
   }
   
   return 0;
}


int16_t mea_xPLReadMessage(int16_t fd, int32_t timeoutms, char *data, int *l_data, int16_t *nerr)
{
   fd_set input_set;
   struct timeval timeout;
   
   FD_ZERO(&input_set);
   FD_SET(fd, &input_set);
   
   timeout.tv_sec = 0;
   timeout.tv_usec = timeoutms * 1000;
   
   int ret = select(fd+1, &input_set, NULL, NULL, &timeout);
   if(ret<=0) {
      *l_data=0;
      if(ret == 0) {
         *nerr = -1; // timeout
      }
      else {
         ret=-1;
         *nerr = -2;
      }
      goto on_error_exit_xpl_read;
   }
   
   ret = (int)read(fd,data,*l_data);
   if(ret<1) {
      *l_data=0;
      if(ret==0) {
         *nerr=0;
         return 0;
      }
      else {
         *nerr = ret;
         return -1;
      }
   }
   else {
      *l_data = ret;
      *nerr = 0;
      return 0;
   }
   
on_error_exit_xpl_read:
   return ret;
}


/*
 xpl-stat
 {
 hop=1
 source=mea-edomus.homedev1
 target=*
 }
 hbeat.app
 {
 interval=1
 port=54813
 remote-ip=192.168.10.1
 version=0.1a2
 */
void mea_xPLSendHbeat(int fd, struct sockaddr_in *xPLBroadcastAddr, char *remoteip, int port, char *version, char *source, char *type, int interval)
{
   char *hbeatMsg = "xpl-stat\n{\nhop=1\nsource=%s\ntarget=*\n}\nhbeat.%s\n{\ninterval=%d\nport=%d\nremote-ip=%s\nversion=%s\n}\n";
   
   char msg[0xFFFF];
   
   sprintf(msg, hbeatMsg, source, type, interval, port, remoteip, version);
   
   mea_xPLSendMessage(fd, xPLBroadcastAddr, msg, (int)strlen(msg));
}


cJSON *mea_xplMsgToJson_alloc(cJSON *xplMsgJson)
{
   if(xplMsgJson == NULL)
      return NULL;

   cJSON *jsonXplMsg = NULL;
   cJSON *jsonBody= NULL;
   
   jsonXplMsg = cJSON_CreateObject();
   jsonBody=cJSON_CreateObject();

   if(!jsonXplMsg || !jsonBody) {
      if(jsonXplMsg) {
         cJSON_Delete(jsonXplMsg);
      }
      if(jsonBody) {
         cJSON_Delete(jsonBody);
      }
      return NULL;
   }

   cJSON_AddNumberToObject(jsonXplMsg, XPLMSG_STR_C, 1.0);
   
   cJSON *e=xplMsgJson->child;
   while(e) {
      if(e->string) {
         if(strcmp(e->string, XPLSOURCE_STR_C)==0  ||
            strcmp(e->string, XPLTARGET_STR_C)==0  ||
            strcmp(e->string, XPLMSGTYPE_STR_C)==0 ||
            strcmp(e->string, XPLSCHEMA_STR_C)==0) {
               
            cJSON_AddStringToObject(jsonXplMsg, e->string, e->valuestring);
         }
         else {
            if (e->valuestring != NULL) {
               cJSON_AddStringToObject(jsonBody, e->string, e->valuestring);
            }
            else {
               cJSON_AddNullToObject(jsonBody, e->string);
            }
         }
      }
      e=e->next;
   }
   
   cJSON_AddItemToObject(jsonXplMsg, "body", jsonBody);

   return jsonXplMsg;
}
