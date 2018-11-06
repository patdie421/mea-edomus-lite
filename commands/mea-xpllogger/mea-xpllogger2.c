//
//  mea-xpllogger.c
//  mea-edomus
//
//  Created by Patrice Dietsch on 13/10/2014.
//
//

#include <stdio.h>
#include <stdlib.h>
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

#include "mea_timer.h"
#include "mea_verbose.h"

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
  if ((ppe = getprotobyname("udp")) == 0)
  {
    VERBOSE(2) mea_log_printf("Unable to lookup UDP protocol info\n");
    return -1;
  }

  // Attempt to create a socket
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, ppe->p_proto)) < 0)
  {
    VERBOSE(2) mea_log_printf("Unable to create broadcast socket %s (%d)\n", strerror(errno), errno);
    return -1;
  }

  // Mark as a broadcasting socket
  if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &flag, sizeof(flag)) < 0)
  {
    VERBOSE(2) mea_log_printf("Unable to set SO_BROADCAST on socket %s (%d)\n", strerror(errno), errno);
    close(sockfd);
    return -1;
  }

  // Init the interface info request
  bzero(&interfaceInfo, sizeof(struct ifreq));
  interfaceInfo.ifr_addr.sa_family = AF_INET;
  strcpy(interfaceInfo.ifr_name, xPLInterfaceName);

  // Get our interface address
  if (ioctl(sockfd, SIOCGIFADDR, &interfaceInfo) != 0)
  {
    VERBOSE(2) mea_log_printf("Unable to get IP addr for interface %s\n", xPLInterfaceName);
    close(sockfd);
    return -1;
  }
  xPLInterfaceAddr.s_addr = ((struct sockaddr_in *) &interfaceInfo.ifr_addr)->sin_addr.s_addr;
  strncpy(ip, inet_ntoa(xPLInterfaceAddr), l_ip);
  ip[l_ip-1]=0;
  VERBOSE(5) mea_log_printf("Auto-assigning IP address of %s\n", ip);

  // Get interface netmask
  bzero(&interfaceInfo, sizeof(struct ifreq));
  interfaceInfo.ifr_addr.sa_family = AF_INET;
  interfaceInfo.ifr_broadaddr.sa_family = AF_INET;
  strcpy(interfaceInfo.ifr_name, xPLInterfaceName);
  if (ioctl(sockfd, SIOCGIFNETMASK, &interfaceInfo) != 0)
  {
    VERBOSE(2) mea_log_printf("Unable to extract the interface net mask\n");
    close(sockfd);
    return -1;
  }
  interfaceNetmask.s_addr = ((struct sockaddr_in *) &interfaceInfo.ifr_netmask)->sin_addr.s_addr;

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
  VERBOSE(2) mea_log_printf("Assigned xPL Broadcast address of %s, port %d\n", inet_ntoa(xPLBroadcastAddr->sin_addr), XPLPORT);

  return sockfd;
}


// Try to increase the receive buffer as big as possible.  if we make it bigger, return TRUE.  Otherwise, if no change, FALSE
int16_t maximizeReceiveBufferSize(int thefd)
{
  int startRcvBuffSize, idealRcvBuffSize, finalRcvBuffSize;
  socklen_t buffLen = sizeof(int);

  // Get current receive buffer size
  if (getsockopt(thefd, SOL_SOCKET, SO_RCVBUF, &startRcvBuffSize, &buffLen) != 0)
    VERBOSE(5) mea_log_printf("Unable to read receive socket buffer size - %s (%d)\n", strerror(errno), errno);
  else
    VERBOSE(5)mea_log_printf("Initial receive socket buffer size is %d bytes\n", startRcvBuffSize);

  // Try to increase the buffer (maybe multiple times)
  for (idealRcvBuffSize = 1024000; idealRcvBuffSize > startRcvBuffSize; )
  {
    // Attempt to set the buffer size
    if (setsockopt(thefd, SOL_SOCKET, SO_RCVBUF, &idealRcvBuffSize, sizeof(int)) != 0)
    {
      VERBOSE(5) mea_log_printf("Not able to set receive buffer to %d bytes - retrying\n", idealRcvBuffSize);
      idealRcvBuffSize -= 64000;
      continue;
    }

    // We did it!  Get the current size and bail out
    buffLen = sizeof(int);
    if (getsockopt(thefd, SOL_SOCKET, SO_RCVBUF, &finalRcvBuffSize, &buffLen) != 0)
      VERBOSE(5) mea_log_printf("Unable to read receive socket buffer size - %s (%d)\n", strerror(errno), errno);
    else
      VERBOSE(5) mea_log_printf("Actual receive socket buffer size is %d bytes\n", finalRcvBuffSize);

    if(finalRcvBuffSize > startRcvBuffSize)
       return 0;
  }

  // We weren't able to increase it

  return -1;
}


/* Attempt to make a hub based connection */
int mea_xPLConnectHub(int *xPLPort) {
  int sockfd;
  int flag = 1;
  int sockSize = sizeof(struct sockaddr_in);
  struct protoent *ppe;
  struct sockaddr_in theSockInfo;

  *xPLPort = 0;
  /* Init the socket definition */
  bzero(&theSockInfo, sizeof(theSockInfo));
  theSockInfo.sin_family = AF_INET;
  theSockInfo.sin_addr.s_addr = INADDR_ANY;
  theSockInfo.sin_port = htons(0);

  /* Map protocol name */
  if ((ppe = getprotobyname("udp")) == 0)
  {
    VERBOSE(2) mea_log_printf("Unable to lookup UDP protocol info\n");
    return -1;
  }

  /* Attempt to creat the socket */
  if ((sockfd = socket(PF_INET, SOCK_DGRAM, ppe->p_proto)) < 0)
  {
    VERBOSE(2) mea_log_printf("Unable to create listener socket %s (%d)\n", strerror(errno), errno);
    return -1;
  }

  /* Mark as a broadcast socket */
  if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &flag, sizeof(flag)) < 0)
  {
    VERBOSE(2) mea_log_printf("Unable to set SO_BROADCAST on socket %s (%d)\n", strerror(errno), errno);
    close(sockfd);
    return -1;
  }

  /* Attempt to bind */
  if ((bind(sockfd, (struct sockaddr *)&theSockInfo, sockSize)) < 0)
  {
    VERBOSE(2) mea_log_printf("Unable to bind listener socket to port %d, %s (%d)\n", XPLPORT, strerror(errno), errno);
    close(sockfd);
    return -1;
  }

  /* Fetch the actual socket port # */
  if (getsockname(sockfd, (struct sockaddr *) &theSockInfo, (socklen_t *) &sockSize))
  {
    VERBOSE(2) mea_log_printf("Unable to fetch socket info for bound listener, %s (%d)\n", strerror(errno), errno);
    close(sockfd);
    return -1;
  }

  /* We are ready to go */
  *xPLPort = ntohs(theSockInfo.sin_port);

/*
   // markNonblocking;
   int fcntlret=fcntl(sockfd, F_GETFL, 0);
   if(fcntlret != -1)
      fcntl(sockfd, F_SETFL, fcntlret|O_NONBLOCK);
*/
//   maximizeReceiveBufferSize(sockfd);

   VERBOSE(5) mea_log_printf("xPL Starting in Hub mode on port %d\n", *xPLPort);

   return sockfd;
}


int16_t xpl_sendMessage(int fd, struct sockaddr_in *xPLBroadcastAddr, char *theData, int dataLen) {
  int bytesSent;

  /* Try to send the message */
  if ((bytesSent = sendto(fd, theData, dataLen, 0,
                          (struct sockaddr *) xPLBroadcastAddr, sizeof(struct sockaddr_in))) != dataLen)
  {
    VERBOSE(2) mea_log_printf("Unable to broadcast message, %s (%d)\n", strerror(errno), errno);
    return -1;
  }
  VERBOSE(2) mea_log_printf("Broadcasted %d bytes (of %d attempted)\n", bytesSent, dataLen);

  /* Okey dokey then */
  return 0;
}


int16_t _xpl_read(int16_t fd, int32_t timeoutms, char *data, int *l_data, int16_t *nerr)
{
   fd_set input_set;
   struct timeval timeout;

   FD_ZERO(&input_set);
   FD_SET(fd, &input_set);

   timeout.tv_sec = 0;
   timeout.tv_usec = timeoutms * 1000;
   int ret;

   ret = select(fd+1, &input_set, NULL, NULL, &timeout);
   if(ret<=0)
   {
      if(ret == 0)
      {
         *nerr = 1; // timeout
      }
      else
      {
         perror("_xpl_read: ");
         *nerr = 2;
      }
      goto on_error_exit_xpl_read;
   }

   ret = (int)read(fd,data,*l_data);
   if(ret<1)
   {
      *nerr = 3;
      return -1;
   }
   else
   {
      *l_data = ret;
      *nerr = 0;
      return 0;
   }

on_error_exit_xpl_read:
   return -1;
}

// char *_xplWDMsg = "xpl-trig\n{\nhop=1\nsource=%s\ntarget=%s\n}\nwatchdog.basic\n{\ninterval=10\n}\n";
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

   xpl_sendMessage(fd, xPLBroadcastAddr, msg, strlen(msg));
}


int exit_loop=0;
static void _logger2_stop(int signal_number)
{
   exit_loop=1;
}


void logger2(char *interface)
{
   int16_t nerr;
   int port=0;
   char ip[255];
   char *source="mea-edomus.logger";
   int interval = 1;

   int sd=mea_xPLConnectHub(&port);
   if(sd<0)
      return;

   struct sockaddr_in xPLBroadcastAddr;
   int sdb=mea_xPLConnectBroadcast(interface, ip, sizeof(ip)-1, &xPLBroadcastAddr);
   if(sdb<0)
   {
      close(sd);
      return;
   }

   mea_xPLSendHbeat(sdb, &xPLBroadcastAddr, ip, port, "0.1a2", source, "app", interval);

   char data[0xFFFF];
   int l_data;
   mea_timer_t hbTimer;

   mea_init_timer(&hbTimer, interval*60, 1);

   for(;!exit_loop;)
   {
      if(mea_test_timer(&hbTimer)==0)
      {
         mea_xPLSendHbeat(sdb, &xPLBroadcastAddr, ip, port, "0.1a2", source, "app", interval);
      }

      l_data = sizeof(data);
      int ret=_xpl_read(sd, 1000, data, &l_data, &nerr);
      data[l_data]=0;
      if(ret>=0)
         mea_log_printf("data=%s\n", data); 
   }

   mea_xPLSendHbeat(sdb, &xPLBroadcastAddr, ip, port, "0.1a2", source, "end", interval);

   close(sdb);
   close(sd);
}


int main(int argc, char *argv[])
{
   if(argc == 2)
   {
      signal(SIGINT,  _logger2_stop);
      signal(SIGQUIT, _logger2_stop);
      signal(SIGTERM, _logger2_stop);

      logger2(argv[1]);
   }
   else
   {
      fprintf(stderr,"usage : %s [interface]\n",argv[0]);
      fprintf(stderr,"example : %s eth0\n",argv[0]);
      exit(1);
   }
}
