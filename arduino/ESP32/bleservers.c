#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "bleservers.h"

#include "rtos_simu.h"
#include "arduino_simu.h"
#include "task_device.h"

struct meaBLEServer_s meaBLEServers;


int mea_lower(char *str)
{
   int i=0;
   for(;i<strlen(str);i++) {
      str[i]=tolower(str[i]);
   }
   return i;
}


bool isAddr(char *addr)
{
   if(!(addr[2]=='-' && addr[5]=='-' && addr[8]=='-' && addr[11]=='-' && addr[14]=='-')) {
      return false;
   }
   int i=0;
   for(;addr[i];i++) {
      if(!(isxdigit(addr[i]) || addr[i]=='-')) {
         return false;
      }
   }
   if(i!=17) {
      return false;
   }
   return true;
}


bool isUUID(char *uuid)
{
   if(!(uuid[8]=='-' && uuid[13]=='-' && uuid[18]=='-' && uuid[23]=='-')) {
      return false;
   }
   int i=0;
   for(;uuid[i];i++) {
      if(!(isxdigit(uuid[i]) || uuid[i]=='-')) {
         return false;
      }
   }
   if(i!=36) {
      return false;
   }
   return true;
}


bool isParameters(char *parameters)
{
   if(parameters[0]) {
      return true;
   }
   else {
      return false;
   }
}


#ifdef DEBUG
void printCharacteristics()
{
   for(int i=0;i<MAX_CHARACTERISTICS;i++) {
      if(meaBLEServers.meaCharacteristics[i].state!=-1) {
         printf("%s %s %s %c %d\n",
            meaBLEServers.meaDevices[meaBLEServers.meaCharacteristics[i].device].address,
            meaBLEServers.meaServices[meaBLEServers.meaCharacteristics[i].service].serviceUUID,
            meaBLEServers.meaCharacteristics[i].characteristicUUID,
            meaBLEServers.meaCharacteristics[i].type,
            meaBLEServers.meaCharacteristics[i].pp
         );
      }
   }
}
#endif


int8_t findDeviceByAddress(char *address) {
   if(meaBLEServers.nbDevices==0) {
      return -1;
   }
   for(uint8_t i=0; i<MAX_DEVICES; i++) {
      if(strcmp(address, meaBLEServers.meaDevices[i].address)==0) {
         return i;
      }
   }
   return -1;
}


int8_t findServiceForDeviceByUUID(int8_t device, char *serviceUUID)
{
   if(meaBLEServers.nbServices==0 || device<0) {
      return -1;
   }

   for(uint8_t i=0;i<MAX_SERVICES;i++) {
      if(meaBLEServers.meaServices[i].state!=1 &&
         meaBLEServers.meaServices[i].device == device &&
         strcmp(serviceUUID, meaBLEServers.meaServices[i].serviceUUID)==0
      ) {
         return i;
      }
   }
   return -1;
}


int8_t findCharacteristicForDeviceServiceByUUID(int8_t device, int8_t service, char *characteristicUUID)
{
   if(meaBLEServers.nbCharacteristics==0 || device<0 || service<0) {
      return -1;
   }
   for(uint8_t i=0;i<MAX_CHARACTERISTICS;i++) {
      if( meaBLEServers.meaCharacteristics[i].state!=1 &&
         meaBLEServers.meaCharacteristics[i].device == device &&
         meaBLEServers.meaCharacteristics[i].service == service &&
         strcmp(characteristicUUID, meaBLEServers.meaCharacteristics[i].characteristicUUID)==0
      ) {
            return i;
      }
   }
   return -1;
}


int8_t findCharacteristic(char *addr, char *serviceUUID, char *characteristicUUID)
{
   int8_t device=findDeviceByAddress(addr);
   int8_t service=findServiceForDeviceByUUID(device, serviceUUID);
   return findCharacteristicForDeviceServiceByUUID(device, service, characteristicUUID);
}


int addDevice(char *address) {
   int i=-1;
   if(!(meaBLEServers.nbDevices<MAX_DEVICES)) {
      return -1;
   }
   i=findDeviceByAddress(address);
   if(i!=-1) {
      return i;
   }
   
   for(int i=0;i<MAX_DEVICES;i++) {
      if(meaBLEServers.meaDevices[i].state==-1) {
         strcpy(meaBLEServers.meaDevices[i].address, address);
         meaBLEServers.meaDevices[i].state=0;
         meaBLEServers.nbDevices++;
         return i;
      }
   }
   return -1;
}


int addService(int deviceId, char *service)
{
   int i=-1;
   if(!(meaBLEServers.nbServices<MAX_SERVICES)) {
      return -1;
   }

   if(meaBLEServers.meaDevices[deviceId].state==-1) {
      return -1;
   }

   i=findServiceForDeviceByUUID(deviceId, service);
   if(i!=-1) {
      return i;
   }

   for(i=0;i<MAX_SERVICES;i++) {
      if(meaBLEServers.meaServices[i].state==-1) {
         strcpy(meaBLEServers.meaServices[i].serviceUUID, service);
         meaBLEServers.meaServices[i].device=deviceId;
         meaBLEServers.meaServices[i].state=0;
         meaBLEServers.nbServices++;
         return i;
      }
   }

   return -1;
}


int addCharacteristic(int deviceId, int serviceId, char *characteristic)
{
   int i=-1;
   if(!(meaBLEServers.nbCharacteristics<MAX_CHARACTERISTICS)) {
      return -1;
   }

   if(meaBLEServers.meaDevices[deviceId].state==-1) {
      return -1;
   }

   if(meaBLEServers.meaServices[serviceId].state==-1 ||
      meaBLEServers.meaServices[serviceId].device!=deviceId) {
      return -1;
   }

   i=findCharacteristicForDeviceServiceByUUID(deviceId, serviceId, characteristic);
   if(i!=-1) {
      return i;
   }

   for(int8_t i=0;i<MAX_CHARACTERISTICS;i++) {
      if(meaBLEServers.meaCharacteristics[i].state==-1) {
         strcpy(meaBLEServers.meaCharacteristics[i].characteristicUUID, characteristic);
         meaBLEServers.meaCharacteristics[i].service=serviceId;
         meaBLEServers.meaCharacteristics[i].device=deviceId;
         meaBLEServers.meaCharacteristics[i].state=0;
         meaBLEServers.nbCharacteristics++;
         return i;
      }
   }
   
   return -1;
}


// aa-aa-aa-aa-aa-aa;6E400002-B5A3-F393-E0A9-E50E24DCCA9E;33400002-B5A3-F393-E0A9-E50E24DCCA33;cmd
int getMacAddr(char *macAddr, char *buff)
{
   strncpy(macAddr, buff, 17);
   macAddr[17]=0;
}


int getServiceUUID(char *serviceUUID, char *buff)
{
   if(buff[17]!=';') {
      return false;
   }

   strncpy(serviceUUID, &(buff[18]), 36);
   serviceUUID[36]=0;

   return true;
}


int getCharacteristicUUID(char *characteristicUUID, char *buff)
{
   if(buff[54]!=';') {
      return false;
   }

   strncpy(characteristicUUID, &(buff[55]), 36);
   characteristicUUID[36]=0;
   
   return true;
}


int getParameters(char *parameters, char *buff)
{
   if(buff[91]!=';') {
      return false;
   }

   strcpy(parameters, &(buff[92]));
   
   return true;
}


int getDeviceStateById(int8_t deviceId)
{
   return meaBLEServers.meaDevices[deviceId].state;
}


int32_t addWatcher(char *addr, char *serviceUUID, char *characteristicUUID, char *parameters)
{
   int8_t deviceId=addDevice(addr);
   if(deviceId==-1) {
      return -1;
   }
   int8_t serviceId=addService(deviceId, serviceUUID);
   if(serviceId==-1) {
      return -1;
   }
   int8_t characteristicId=addCharacteristic(deviceId, serviceId, characteristicUUID);
   if(characteristicId==-1) {
      return -1;
   }

   // parameters :
   // [R|W|B]:PP
   // R: Read
   // W: Write
   // B: both (Read/Write)
   // PP: polling period in s (0: no polling, but allow notification)
   uint8_t ret=0xFF;
   uint32_t pp=0;
   char type=toupper(parameters[0]);

   if(parameters[1]==':' && (type=='R' || type=='W' || type=='B') ) {
      char *_pp=&(parameters[2]);
      for(int i=0;_pp[i];i++) {
         if(!isdigit(_pp[i])) {
            ret=0;
            break;
         }
      }
      if(ret!=0) {
         pp=(uint32_t)atoi(_pp);
      }
   }
   else {
      ret=0;
   }
   if(ret!=0) {
      meaBLEServers.meaCharacteristics[characteristicId].type=type;
      meaBLEServers.meaCharacteristics[characteristicId].pp=pp;
   }
   int32_t res=deviceId & 0xFF | (serviceId & 0xFF) << 8 | (characteristicId & 0xFF) << 16 | (ret & 0xFF) << 24;
   
   return res;
}


int32_t getContext(char *buff, char *addr, char *service, char *characteristic, char *parameters)
{
   addr[0]=0;
   service[0]=0;
   characteristic[0]=0;
   parameters[0];

   getMacAddr(addr, buff);
   getServiceUUID(service, buff);
   getCharacteristicUUID(characteristic, buff);
   getParameters(parameters,buff);
   
   int8_t ret=0;
   
   if(!isAddr(addr)) {
      addr[0]=0;
   }
   else {
      ret=F_DEVICE;
   }

   if(!isUUID(service)) {
      service[0]=0;
   }
   else {
      ret=ret | F_SERVICE;
   }

   if(!isUUID(characteristic)) {
      characteristic[0]=0;
   }
   else {
      ret=ret | F_CHARACTERISTIC;
   }

   if(!isParameters(parameters)) {
      parameters[0]=0;
   }
   else {
      ret=ret | F_PARAMETERS;
   }

   return ret;   
}


bool get_value(int8_t characteristicId)
{
   struct device_queue_elem_s elem;
   int8_t deviceId=meaBLEServers.meaCharacteristics[characteristicId].device;
   elem.data[0]=0;
   elem.action='G';
   BaseType_t xRet=xQueueSend(meaBLEServers.meaDevices[deviceId].queue,&elem,1000);

   return xRet;
}


bool set_value(int8_t characteristicId, char *parameters)
{
   struct device_queue_elem_s elem;
   int8_t deviceId=meaBLEServers.meaCharacteristics[characteristicId].device;
   strncpy(elem.data, parameters, sizeof(elem.data)-1);
   elem.data[sizeof(elem.data)-1]=0;
   elem.action='S';
   BaseType_t xRet=xQueueSend(meaBLEServers.meaDevices[deviceId].queue,&elem,1000);

   return xRet;
}


bool deleteDevice(int8_t device)
{
   for(int i=0;i<MAX_CHARACTERISTICS;i++) {
      if(meaBLEServers.meaCharacteristics[i].device==device) {
         meaBLEServers.meaCharacteristics[i].state=-1;
         if(meaBLEServers.meaServices[meaBLEServers.meaCharacteristics[i].service].state!=-1) {
            meaBLEServers.meaServices[meaBLEServers.meaCharacteristics[i].service].state=-1;
            meaBLEServers.nbServices--;
         }
         if(meaBLEServers.meaDevices[device].state!=-1) {
            meaBLEServers.meaDevices[device].state=-1;
            meaBLEServers.nbDevices--;
         }
         meaBLEServers.nbCharacteristics--;
      }
   }
   return false;
}


int stopDevice(int8_t deviceId)
{
   vTaskDelete(meaBLEServers.meaDevices[deviceId].task);
   vQueueDelete(meaBLEServers.meaDevices[deviceId].queue);
   meaBLEServers.meaDevices[deviceId].state=0;

   DISPLAY("DEVICE: ");
   DISPLAY(meaBLEServers.meaDevices[deviceId].address);
   DISPLAY(" is down\n");
}


int startDevice(int8_t deviceId)
{
   if(meaBLEServers.meaDevices[deviceId].state>0) {
      printf("already running\n");
   }
   else if(meaBLEServers.meaDevices[deviceId].state<0) {
      printf("not defined\n");
   }
   else {
      char name[9]="DEVICE_x";
      void *pvParameters = (void *)((long)deviceId);
      name[7]='0'+deviceId;

      DISPLAY("starting device ... ");
      meaBLEServers.meaDevices[deviceId].state=1; // starting state
      meaBLEServers.meaDevices[deviceId].queue = xQueueCreate(MAX_DEVICE_QUEUE_SIZE, sizeof(struct device_queue_elem_s));
      meaBLEServers.meaDevices[deviceId].queue->debug=1;
      if(meaBLEServers.meaDevices[deviceId].queue) {
         BaseType_t xRet=xTaskCreate(
            deviceTask,
            name,
            1000, // stack size
            pvParameters,
            1, // priorité
            &(meaBLEServers.meaDevices[deviceId].task));
         if(xRet) {
            DISPLAY("done\n");
         }
         else {
            DISPLAY("failed\n");
         }
      }
      else {
         DISPLAY("can't create queue");
      }
   }
}


void initBLEServers()
{
   for(int i=0;i<MAX_DEVICES;i++) {
      meaBLEServers.meaDevices[i].state=-1;
   }
    meaBLEServers.nbDevices=0;

   for(int i=0;i<MAX_SERVICES;i++) {
      meaBLEServers.meaServices[i].state=-1;
   }
   meaBLEServers.nbServices=0;

   for(int i=0;i<MAX_CHARACTERISTICS;i++) {
      meaBLEServers.meaCharacteristics[i].state=-1;
   }
}
