#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>

#include "task_input.h"
#include "task_output.h"
#include "task_device.h"

#include "rtos_simu.h"
#include "bleservers.h"


TaskHandle_t inputTaskHandle;


int receiveData(char *buff, int l_buff, int timeout)
{
   int i=0;
   while(i<l_buff) {
      if(SERIAL_AVAILABLE()) {
         char c=SERIAL_READ();
         if(c=='\n') {
            buff[i++]=0;
            return i;
         }
         else {
            buff[i++]=c;
         }
      }
   }
   return i;
}


bool serial_in()
{
   char buff[256]="";
   char *_buff=buff;

   uint8_t action='\0';
   char deviceAddr[18]="";
   char serviceUUID[36]="";
   char characteristicUUID[36]="";
   char parameters[80]="";

   if(receiveData(buff, sizeof(buff)-1, 1000)<=0) {
      return false;
   }

   if(buff[1]==';') {
      switch(buff[0]) {
         case '+': // add
         case '-': // delete
         case '?': // query
         case '#': // set
         case '@': // control
            action=buff[0];
            _buff=buff+2;
            break;
         default:
            return send_error(ERROR_BAD_REQUEST);
      }
   }
   else {
      return send_error(ERROR_BAD_REQUEST);
   }
   
   if(action=='@') {
      send_value_string("[control]");
   }
   else {
      bool ret=getContext(_buff, deviceAddr, serviceUUID, characteristicUUID, parameters);
      if(ret) {
         switch(action) {
            case '+':
               if(ret == (F_DEVICE | F_SERVICE | F_CHARACTERISTIC | F_PARAMETERS)) {
                  int32_t ret=addWatcher(deviceAddr, serviceUUID, characteristicUUID, parameters);
                  if((ret & 0xFF000000)!=0) {
                     int8_t deviceId=ret & 0xFF;
                     if(ret!=-1 && getDeviceStateById(deviceId)==0) {
                        startDevice(deviceId);
                     }
                  }
                  else {
                     return send_error(ERROR_BAD_REQUEST);
                  }
                  return send_ok();
               }
               break;
   
            case '-':
               if(ret == F_DEVICE) {
                  int8_t deviceId=findDeviceByAddress(deviceAddr);
                  stopDevice(deviceId);
                  deleteDevice(deviceId);
                  return send_ok();
               }
               break;
   
            case '?':
               if(ret == (F_DEVICE | F_SERVICE | F_CHARACTERISTIC)) {
                  int8_t characteristicId=findCharacteristic(deviceAddr, serviceUUID, characteristicUUID);
                  if(characteristicId!=-1) {
                     if(meaBLEServers.meaCharacteristics[characteristicId].type=='R' ||
                        meaBLEServers.meaCharacteristics[characteristicId].type=='B') {
                        return get_value(characteristicId);
                     }
                     else {
                        return send_error(ERROR_BAD_ACTION);
                     }
                  }
                  else {
                     return send_error(ERROR_BAD_ID);
                  }
               }
               else {
                  return send_error(ERROR_BAD_REQUEST);
               }
               break;
   
            case '#':
               if(ret == (F_DEVICE | F_SERVICE | F_CHARACTERISTIC | F_PARAMETERS)) {
                  int8_t characteristicId=findCharacteristic(deviceAddr, serviceUUID, characteristicUUID);
                  if(characteristicId!=-1) {
                     if(meaBLEServers.meaCharacteristics[characteristicId].type=='W' ||
                        meaBLEServers.meaCharacteristics[characteristicId].type=='B') {
                        return set_value(characteristicId, parameters);
                     }
                     else {
                        return send_error(ERROR_BAD_ACTION);
                     }
                  }
                  else {
                     return send_error(ERROR_BAD_ID);
                  }
               }
               else {
                  return send_error(ERROR_BAD_REQUEST);
               }
               break;
         }
      }
      else {
         return send_error(ERROR_BAD_REQUEST);
      }
      return send_ok();
   }
}


void inputTask(void *pvParameters)
{
   for(;;) {
      serial_in();
   }
}


bool init_inputTask()
{
   BaseType_t xRet=xTaskCreate(inputTask, "INPUT", 1000, (void *)0, 1, &inputTaskHandle);
   if(xRet) {
      DISPLAY("input task OK\n");
      return true;
   }
   else {
      DISPLAY("input task KO\n");
      return false;
   }
}
