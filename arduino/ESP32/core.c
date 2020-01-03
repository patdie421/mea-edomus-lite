#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>


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
   struct meaDevice_s meaDevices[MAX_DEVICES];
   int8_t nbDevices;
   struct meaService_s meaServices[MAX_SERVICES];
   int8_t nbServices;
   struct meaCharacteristic_s meaCharacteristics[MAX_CHARACTERISTICS];
   int8_t nbCharacteristics;
};

struct meaBLEServer_s meaBLEServers;

void initDevices()
{
   for(int i=0;i<MAX_DEVICES;i++) {
      meaBLEServers.meaDevices[i].state=-1;
   }
    meaBLEServers.nbDevices=0;
}

void initServices()
{
   for(int i=0;i<MAX_SERVICES;i++) {
      meaBLEServers.meaServices[i].state=-1;
   }
   meaBLEServers.nbServices=0;
}

void initCharacteristics()
{
   for(int i=0;i<MAX_CHARACTERISTICS;i++) {
      meaBLEServers.meaCharacteristics[i].state=-1;
   }
   meaBLEServers.nbCharacteristics=0;
}

void initBLEServers()
{
   initDevices();
   initServices();
   initCharacteristics();
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


bool addDevice(char *address) {
   if(!(meaBLEServers.nbDevices<MAX_DEVICES)) {
      return false;
   }
   if(findDeviceByAddress(address)!=-1) {
      return false;
   }
   
   for(int i=0;i<MAX_DEVICES;i++) {
      if(meaBLEServers.meaDevices[i].state==-1) {
         strcpy(meaBLEServers.meaDevices[i].address, address);
         meaBLEServers.meaDevices[i].state=0;
         meaBLEServers.nbDevices++;
         return true;
      }
   }
   return false;
}


bool addService(char *address, char *service)
{
   if(!(meaBLEServers.nbServices<MAX_SERVICES)) {
      return false;
   }
   int8_t device=findDeviceByAddress(address);
   if(device!=-1 && findServiceForDeviceByUUID(device, service)!=-1) {
      return false;
   }
   if(device!=-1) {
      for(int i=0;i<MAX_SERVICES;i++) {
         if(meaBLEServers.meaServices[i].state==-1) {
            strcpy(meaBLEServers.meaServices[i].serviceUUID, service);
            meaBLEServers.meaServices[i].device=device;
            meaBLEServers.meaServices[i].state=0;
            meaBLEServers.nbServices++;
            return true;
         }
      }
   }
   return false;
}


bool addCharacteristic(char *address, char *service, char *characteristic)
{
   if(!(meaBLEServers.nbCharacteristics<MAX_CHARACTERISTICS)) {
      return false;
   }

   int8_t _device=findDeviceByAddress(address);
   if(_device==-1) {
      return false;
   }

   int8_t _service=findServiceForDeviceByUUID(_device, service);
   if(_service==-1) {
      return false;
   }
   
   if(findCharacteristicForDeviceServiceByUUID(_device, _service, characteristic)!=-1) {
      return false;
   }

   for(int8_t i=0;i<MAX_CHARACTERISTICS;i++) {
      if(meaBLEServers.meaCharacteristics[i].state==-1) {
         strcpy(meaBLEServers.meaCharacteristics[i].characteristicUUID, characteristic);
         meaBLEServers.meaCharacteristics[i].service=_service;
         meaBLEServers.meaCharacteristics[i].device=_device;
         meaBLEServers.meaCharacteristics[i].state=0;
         meaBLEServers.nbCharacteristics++;
         return true;
      }
   }
   
   return false;
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

int main(int argc, char *argv[])
{
   initBLEServers();
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
   deleteDevice(1);
   printCharacteristics();
   printf("===\n");
}