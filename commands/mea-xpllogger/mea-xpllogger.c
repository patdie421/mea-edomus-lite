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
#ifdef __linux__
#include <signal.h>
#include <errno.h>
#include <inttypes.h>
#include <time.h>

#include "xPL.h"

#define XPL_VERSION "0.1a2"

char *xpl_msg_any = "xpl-any";
char *xpl_msg_command = "xpl-cmnd";
char *xpl_msg_status = "xpl-stat";
char *xpl_msg_trigger = "xpl-trig";

static unsigned long _millis()
{
  struct timeval tv;
  gettimeofday(&tv,NULL);

  return 1000 * tv.tv_sec + tv.tv_usec/1000;
}

int16_t displayXPLMsg(xPL_MessagePtr theMessage)
{
   char xpl_source[48];
   char xpl_destination[48];
   char xpl_schema[48];

   static unsigned long last = 0;

   unsigned long now=_millis();

   snprintf(xpl_source, sizeof(xpl_source), "%s-%s.%s", xPL_getSourceVendor(theMessage), xPL_getSourceDeviceID(theMessage), xPL_getSourceInstanceID(theMessage));
   if(xPL_isBroadcastMessage(theMessage))
   {
      strcpy(xpl_destination, "*");
   }
   else
   {
      snprintf(xpl_destination,sizeof(xpl_destination),"%s-%s.%s", xPL_getTargetVendor(theMessage), xPL_getTargetDeviceID(theMessage), xPL_getTargetInstanceID(theMessage));
   }

   xPL_MessageType msgType = xPL_getMessageType(theMessage);
   char *msgTypeStrPtr = "";
   switch(msgType)
   {
      case xPL_MESSAGE_ANY:
         msgTypeStrPtr = xpl_msg_any;
         break;
      case xPL_MESSAGE_COMMAND:
         msgTypeStrPtr = xpl_msg_command;
         break;
      case xPL_MESSAGE_STATUS:
         msgTypeStrPtr = xpl_msg_status;
         break;
      case xPL_MESSAGE_TRIGGER:
         msgTypeStrPtr = xpl_msg_trigger;
         break;
      default:
         msgTypeStrPtr = "???";
         break;
   }
   
   snprintf(xpl_schema,sizeof(xpl_schema),"%s.%s", xPL_getSchemaClass(theMessage), xPL_getSchemaType(theMessage));

  
   fprintf(stdout, "[%10u][%06u] %s: source = %s, destination = %s, schema = %s, body = [",now, now-last, msgTypeStrPtr, xpl_source, xpl_destination
   , xpl_schema);

   xPL_NameValueListPtr xpl_body = xPL_getMessageBody(theMessage);
   int n = xPL_getNamedValueCount(xpl_body);
   for (int16_t i = 0; i < n; i++)
   {
      xPL_NameValuePairPtr keyValuePtr = xPL_getNamedValuePairAt(xpl_body, i);

      if(i)
         fprintf(stdout,", ");
      fprintf(stdout,"%s = %s",keyValuePtr->itemName, keyValuePtr->itemValue);
   }
   fprintf(stdout,"]\n");
   fflush(stdout);

   last=now;
   return 0;
}


void xplmsglogger(xPL_MessagePtr theMessage, xPL_ObjectPtr userValue)
{
   displayXPLMsg(theMessage);
}


static void _logger_stop(int signal_number)
{
    exit(0);
}


void logger(char *interface)
{
   signal(SIGINT,  _logger_stop);
   signal(SIGQUIT, _logger_stop);
   signal(SIGTERM, _logger_stop);

   if(interface)
   {
      xPL_setBroadcastInterface(interface);
   }
   else
   {
#ifdef __linux__
      xPL_setBroadcastInterface("eth0");
#else
      xPL_setBroadcastInterface("en0");
#endif
   }
   
   if ( !xPL_initialize(xPL_getParsedConnectionType()) )
      exit(1);
   
   xPL_ServicePtr xPLService = NULL;
   
   xPLService = xPL_createService("mea", "edomus", "debuglogger");
   xPL_setServiceVersion(xPLService, XPL_VERSION);
   xPL_setRespondingToBroadcasts(xPLService, TRUE);
   xPL_addMessageListener(xplmsglogger, NULL);

   xPL_setServiceEnabled(xPLService, TRUE);

   do
   {
      xPL_processMessages(500);
   }
   while (1);
}


int main(int argc, char *argv[])
{
   if(argc == 2)
      logger(argv[1]);
   else if(argc == 1)
      logger(NULL);
   else
   {
      fprintf(stderr,"usage : %s [interface]\n",argv[0]);
      fprintf(stderr,"example : %s eth0\n",argv[0]);
      exit(1);
   }
}
#else
int main(int argc, char *argv[])
{
   fprintf(stderr,"doesn't work on MacOS X now\n");
   exit(1);
}
#endif
