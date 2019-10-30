// **************************************
// * xPL Hub
// *
// * Copyright (C) 2003 Tony Tofts
// * http://www.xplhal.com
// *
// * This program is free software; you can redistribute it and/or
// * modify it under the terms of the GNU General Public License
// * as published by the Free Software Foundation; either version 2
// * of the License, or (at your option) any later version.
// *
// * This program is distributed in the hope that it will be useful,
// * but WITHOUT ANY WARRANTY; without even the implied warranty of
// * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// * GNU General Public License for more details.
// *
// * You should have received a copy of the GNU General Public License
// * along with this program; if not, write to the Free Software
// * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
// **************************************

#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

typedef struct xpl_msg_struc {
   char section[36];
   char name[18];
   char value[130];
} xpl_msg_struc_t ;

typedef struct xpl_hub_struc {
   unsigned port;
   int timeout;
} xpl_hub_struc_t ;

int udpSocket = 0;
struct sockaddr_in inSocket = { 0 }, FromName = { 0 };
unsigned int inSocketLen = 0;

#ifdef xPLCygwin
int FromNameLen = 0;
#else
unsigned int FromNameLen = 0;
#endif

unsigned short xplport = 3865;
char xplmyip[16];
char xpl_interface[32];

#define MAX_MESSAGE_SIZE 1500
char mesg[MAX_MESSAGE_SIZE+1] = "";

#define MAX_MESSAGE_ITEMS 99
xpl_msg_struc xpl_msg[MAX_MESSAGE_ITEMS+1], *pxpl_msg=&xpl_msg[0];

#define MAX_PORT_CONNECTS 100
xpl_hub_struc xplhub[MAX_PORT_CONNECTS];


unsigned int xPLParser(const char* xplmsg, struct xpl_msg_struc * pxpl_msg)
{
   int y, z, c;
   int w;
   char xpl_section[35];
   char xpl_name[17];
   char xpl_value[129];
   unsigned int xpl_msg_index = 0;
   
#define XPL_IN_SECTION 0
#define XPL_IN_NAME 1
#define XPL_IN_VALUE 2
   
   w = XPL_IN_SECTION;
   z = 0;
   y = (int)strlen(xplmsg);
   for (c=0;c<y;c++) {
      
      switch(w) {
         case XPL_IN_SECTION:
            if((xplmsg[c] != 10) && (z != 34)) {
               xpl_section[z]=xplmsg[c];
               z++;
            }
            else {
               c++;
               c++;
               w++;
               xpl_section[z]='\0';
               z=0;
            }
            break;
         case XPL_IN_NAME:
            if((xplmsg[c] != '=') && (xplmsg[c] != '!') && (z != 16)) {
               if(z<16) {
                  xpl_name[z]=xplmsg[c];
                  z++;
               }
            }
            else {
               w++;
               xpl_name[z]='\0';
               z=0;
            }
            break;
         case XPL_IN_VALUE:
            if((xplmsg[c] != 10) && (z != 128)) {
               if(z<128) {
                  xpl_value[z]=xplmsg[c];
                  z++;
               }
            }
            else {
               w++;
               xpl_value[z]='\0';
               z=0;
               if(xpl_msg_index > MAX_MESSAGE_ITEMS) {
                  return(0);
               }
               strcpy(pxpl_msg[xpl_msg_index].section, xpl_section);
               strcpy(pxpl_msg[xpl_msg_index].name, xpl_name);
               strcpy(pxpl_msg[xpl_msg_index].value, xpl_value);
               xpl_msg_index++;
               if(xplmsg[c+1] != '}') {
                  w=XPL_IN_NAME;
               }
               else {
                  w=XPL_IN_SECTION;
                  c++;
                  c++;
               }
            }
            break;
      }
   }
   if(xpl_msg_index<3) {
      xpl_msg_index = 0;
   }
   return xpl_msg_index;
}


void XplSend(const unsigned short xplports, const char* xplmessage)
{
   struct sockaddr_in sendxpl = { 0 };
   unsigned int sendxpllen;
   int xplSocket = 0;
   int optval;
   int optlen;
   sendxpl.sin_family = AF_INET;
   sendxpl.sin_port=htons(xplports);
   sendxpl.sin_addr.s_addr=inet_addr(xplmyip);
   xplSocket=socket(PF_INET, SOCK_DGRAM, 0);
   optval=1;
   optlen=sizeof(int);
   if(setsockopt(xplSocket, SOL_SOCKET, SO_BROADCAST, (char*)&optval, optlen)) { return; }
   sendxpllen=sizeof(struct sockaddr_in);
   
   sendto(xplSocket, xplmessage, strlen(xplmessage),0, (struct sockaddr *) &sendxpl, sendxpllen);
   
   close(xplSocket);
}


void xPLValue(const char* xpl_name, char* xpl_value, struct xpl_msg_struc * pxpl_msg, unsigned int xpl_msg_index)
{
   unsigned int x, c;
   
   x=0;
   c=0;
   
   while ((!x) && (c<xpl_msg_index)) {
      if(strcasecmp(pxpl_msg[c].name,xpl_name)==0) {
         x=1;
         strncpy(xpl_value, pxpl_msg[c].value,sizeof(pxpl_msg[c].value)-1);
         xpl_value[strlen(pxpl_msg[c].value)]='\0';
      }
      c++;
   }
   return;
}


void xplupdate(int signum)
{
   int c;
   
   for(c=0;c<MAX_PORT_CONNECTS;c++) {
      if(xplhub[c].port!=0) {
         xplhub[c].timeout=xplhub[c].timeout-1;
         if(xplhub[c].timeout<1) { xplhub[c].port=0; }
      }
   }
}


int main(int argc, char* argv[])
{
   int size = 0;
   char xpl_param[129];
   unsigned x=0;
   int c;
   int binderr;
   
   struct sockaddr_in *sinp;
   struct ifreq ifr;
   int s;
   
   unsigned int xpl_msg_index = 0;
   
   int xpl_hub_interval;
   unsigned xpl_hub_port;
   char xpl_hub_ip[16];
   int xpl_hub_match;
   
   // parse command line for interface
   if (argc<2) {
      strcpy(xpl_interface, "eth0");
   }
   else {
      strncpy(xpl_interface, argv[1], sizeof(xpl_interface)-1);
   }
   
   // initialise hub
   for(x=0;x<MAX_PORT_CONNECTS;x++) {
      xplhub[x].port=0;
   };
   
   // get my ip address
   if((s=socket(AF_INET,SOCK_DGRAM,0))<0) {
      printf("xPL Hub ERROR: Unable to Initialise!\n");
      exit(-1);
   }
   else {
      strcpy(ifr.ifr_name,xpl_interface);
      if(ioctl(s,SIOCGIFADDR,&ifr)<0) {
         printf("xPL Hub ERROR: Unable to find %s!\n",xpl_interface);
         exit(-1);
      }
      else {
         sinp=(struct sockaddr_in *)&ifr.ifr_addr;
         strcpy(xplmyip,inet_ntoa(sinp->sin_addr));
      }
      close(s);
   }
   
   // initialise interface
   udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
   if(udpSocket < 0) {
      printf("xPL Hub ERROR: Unable to open socket on %s (%d)!\n",xpl_interface,udpSocket);
      exit(-1);
   }
   inSocket.sin_family = AF_INET;
   inSocket.sin_addr.s_addr =  htonl(INADDR_ANY);
   inSocket.sin_port = htons(xplport);
   inSocketLen=sizeof(struct sockaddr);
   binderr=bind(udpSocket, (struct sockaddr *) &inSocket, inSocketLen);
   if(binderr < 0) {
      printf("xPL Hub ERROR: Unable to bind socket on port %d (%d)!\n",xplport,binderr);
      exit(-1);
   }
   FromNameLen=sizeof(struct sockaddr);
   
   // install timer
   struct sigaction sa;
   struct itimerval timer;
   memset(&sa,0,sizeof(sa));
   sa.sa_handler=&xplupdate;
   sigaction(SIGALRM,&sa,NULL);
   timer.it_value.tv_sec=1;
   timer.it_value.tv_usec=0;
   timer.it_interval.tv_sec=60;
   timer.it_interval.tv_usec=0;
   setitimer(ITIMER_REAL,&timer,NULL);
   
   // listen and process
   while(1) {
      // receive packet
      size=-1;
      while(size==-1) {
         size = (int)recvfrom(udpSocket, mesg, MAX_MESSAGE_SIZE, 0,(struct sockaddr *)&FromName, &FromNameLen);
         if(size < -1) {
            return(-1);
         }
      }
      mesg[size]=0;
      // parse message
      xpl_msg_index=xPLParser(mesg, pxpl_msg);
      if(xpl_msg_index > 0) {
         // got a valid message, so check if it's a hub connect
         if((strcasecmp(pxpl_msg[0].section,"XPL-STAT")==0) && ((strncasecmp(pxpl_msg[3].section,"CONFIG.",7)==0) || (strncasecmp(pxpl_msg[3].section,"HBEAT.",6)==0))) {
            // it is a heartbeat, get params
            xPLValue("INTERVAL",xpl_param, pxpl_msg, xpl_msg_index);
            xpl_hub_interval=atoi(xpl_param);
            if(xpl_hub_interval<5) { xpl_hub_interval=5; }
            if(xpl_hub_interval>30) { xpl_hub_interval=30; }

//            strcpy(xpl_hub_ip,inet_ntoa(FromName.sin_addr));

            xPLValue("REMOTE-IP",xpl_param, pxpl_msg, xpl_msg_index);
            strcpy(xpl_hub_ip,xpl_param);

            xPLValue("PORT",xpl_param, pxpl_msg, xpl_msg_index);
            xpl_hub_port=atoi(xpl_param);
            if((strcmp(xplmyip, xpl_hub_ip)==0) && (xpl_hub_port!=0)) {
               // it is local, update timer or add new
               xpl_hub_match=0;
               for(c=0;c<MAX_PORT_CONNECTS;c++) {
                  if(xplhub[c].port==xpl_hub_port) {
                     // got a match
                     xplhub[c].timeout=xpl_hub_interval * 2 + 1;
                     xpl_hub_match=xpl_hub_port;
                     if(strcasecmp(pxpl_msg[3].section,"CONFIG.END")==0 || strcasecmp(pxpl_msg[3].section,"HBEAT.END")==0) {
                        xplhub[c].port=0;
                     }
                     c=MAX_PORT_CONNECTS;
                  }
               }
               if(xpl_hub_match==0) {
                  // new, so find a slot
                  for(c=0;c<MAX_PORT_CONNECTS;c++) {
                     if(xplhub[c].port==0) {
                        // got a slot
                        xplhub[c].port=xpl_hub_port;
                        xplhub[c].timeout=xpl_hub_interval * 2 + 1;
                        c=MAX_PORT_CONNECTS;
                     }
                  }
               }
            }
         }
         for(c=0;c<MAX_PORT_CONNECTS;c++) {
            if(xplhub[c].port!=0) {
               // got a match so relay
               XplSend(xplhub[c].port,mesg);
            }
         }
      }
   }
}
