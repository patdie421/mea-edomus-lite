#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_DEVICES 10
#define MAX_SERVICES 10
#define MAX_CHARACTERISTICS 20

#define false 0
#define true 1
#define bool int


struct meaCharacteristic_s {
   char characteristicUUID[37];
   int8_t type;
   int8_t state;
   int8_t service;
   int8_t device;
};

struct meaService_s {
   char serviceUUID[37];
   int8_t state;
   int8_t device;
};

struct meaDevice_s {
   int8_t state;
   char address[18];
};

struct meaBLEServer_s {
   int8_t nbDevices;
   int8_t nbServices;
   int8_t nbCharacteristics;
   struct meaDevice_s meaDevices[MAX_DEVICES];
   struct meaService_s meaServices[MAX_SERVICES];
   struct meaCharacteristic_s meaCharacteristics[MAX_CHARACTERISTICS];
};

struct meaBLEServer_s meaBLEServers;


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


void mea_lower(char *str)
{
   for(int i=0;i<strlen(str);i++)
      str[i]=tolower(str[i]);
}


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
   if(meaBLEServers.nbServices==0) {
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
   if(meaBLEServers.nbCharacteristics==0) {
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


bool addOperation(char *addr, char *serviceUUID, char *characteristicUUID, char *cmd)
{
   int deviceId=addDevice(addr);
   if(deviceId==-1) {
      return false;
   }
   int serviceId=addService(deviceId, serviceUUID);
   if(serviceId==-1) {
      return false;
   }
   int characteristicId=addCharacteristic(deviceId, serviceId, characteristicUUID);
   if(characteristicId==-1) {
      return false;
   }
   return true;
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


void printCharacteristics()
{
   for(int i=0;i<MAX_CHARACTERISTICS;i++) {
      if(meaBLEServers.meaCharacteristics[i].state!=-1) {
         printf("%s %s %s\n",
                meaBLEServers.meaDevices[meaBLEServers.meaCharacteristics[i].device].address,
                meaBLEServers.meaServices[meaBLEServers.meaCharacteristics[i].service].serviceUUID,
                meaBLEServers.meaCharacteristics[i].characteristicUUID);
      }
   }
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


bool isCmd(char *cmd)
{
   return true;
}


int serial_available()
{
   return 1;
}


int serial_read()
{
   return getchar();
}


#define SERIAL_AVAILABLE serial_available
#define SERIAL_READ serial_read

// #define SERIAL_AVAILABLE Serial.available
// #define SERIAL_READ Serial.read


// aa-aa-aa-aa-aa-aa;6E400002-B5A3-F393-E0A9-E50E24DCCA9E;33400002-B5A3-F393-E0A9-E50E24DCCA33;cmd
int getMacAddr(char *macAddr, char *buff)
{
   strncpy(macAddr, buff, 17);
   macAddr[17]=0;
}


int getServiceUUID(char *serviceUUID, char *buff)
{
   strncpy(serviceUUID, &(buff[18]), 36);
   serviceUUID[36]=0;
}


int getCharacteristicUUID(char *characteristicUUID, char *buff)
{
   strncpy(characteristicUUID, &(buff[55]), 36);
   characteristicUUID[36]=0;
}


int getCmd(char *cmd, char *buff)
{
   strcpy(cmd, &(buff[92]));
}


bool getOperation(char *buff, char *addr, char *service, char *characteristic, char *cmd)
{
   addr[0]=0;
   service[0]=0;
   characteristic[0]=0;
   cmd[0];

   if(!(buff[17]==';' && buff[54]==';' && buff[91]==';'))
      return false;

   getMacAddr(addr, buff);
   getServiceUUID(service, buff);
   getCharacteristicUUID(characteristic, buff);
   getCmd(cmd,buff);
   
   if(!isAddr(addr)) {
      addr[0]=0;
   }
   if(!isUUID(service)) {
      service[0]=0;
   }
   if(!isUUID(characteristic)) {
      characteristic[0]=0;
   }
   if(!isCmd(cmd)) {
      cmd[0]=0;
   }
   
   if(addr[0] & service[0] & characteristic[0] & cmd[0]) {
      return true;
   }

   return false;
}


int readLine(char *buff, int l_buff)
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


int main(int argc, char *argv[])
{
   initBLEServers();
/*
   printf("DEV: %d\n", addDevice("TATA"));
   printf("DEV: %d\n", addDevice("TUTU"));
   printf("DEV: %d\n", addDevice("TUTU"));
   printf("SERVICE: %d\n", addService("TATA", "TITI"));
   printf("SERVICE: %d\n", addService("TUTU", "TITI"));
   printf("CHAR: %d\n", addCharacteristic("TATA", "TITI", "TOTO"));
   printf("CHAR: %d\n", addCharacteristic("TATA", "TITI", "TUTU"));
   printf("CHAR: %d\n", addCharacteristic("TATA", "TITI", "TUTU"));
   printf("CHAR: %d\n", addCharacteristic("TUTU", "TITI", "TUTU"));
   printCharacteristics();
   printf("===\n");
//   deleteDevice(1);
   printCharacteristics();
   printf("===\n");
   printf("%d\n",isMacAddr("xx-xx-xx-xx-xx-xx"));
   printf("%d\n",isMacAddr("aa-aa-aa-aa-aa-aa"));
   printf("%d\n",isMacAddr("aa-aa-aa-aa-aa-aaa"));
   printf("%d\n",isMacAddr("xx-xx-xx_xx-xx-xx"));
   printf("%d\n",isUUID("6E400002-B5A3-F393-E0A9-E50E24DCCA9E"));
   printf("%d\n",isUUID("6E400002-B5A3-F393-E0A9-E50E24DCCA9EA"));
   printf("%d\n",isUUID("6E400002-B5A3-F393xE0A9-E50E24DCCA9E"));
*/   
   while(1) {
      char buff[256];
      readLine(buff, sizeof(buff)-1);
/*
      printf("buff: %s\n",buff);
*/ 
      char macAddr[18];
      char serviceUUID[36];
      char characteristicUUID[36];
      char cmd[80];
      int ret=getOperation(buff, macAddr, serviceUUID, characteristicUUID, cmd);
/*      
      printf("ret=%d\n", ret);
      printf("macaddr: %s\n", macAddr);
      printf("serviceUUID: %s\n", serviceUUID);
      printf("characterisitcUUID: %s\n", characteristicUUID);
      printf("cmd: %s\n", cmd);
*/      
      if(ret) {
/*
         int d=addDevice(macAddr);
         int s=addService(d, serviceUUID);
         int c=addCharacteristic(d, s, characteristicUUID);
         printf("DEV: %d\n",d);
         printf("SERVICE: %d\n", s);
         printf("CHAR: %d\n", c);
*/
         
         ret=addOperation(macAddr, serviceUUID, characteristicUUID, cmd);
         if(ret) {
            printCharacteristics();
         }
      }
   }
}