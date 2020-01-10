#ifndef bleservers_h
#define bleservers_h

#include <inttypes.h>
#include <string.h>
#include <ctype.h>

#include "arduino_simu.h"
#include "rtos_simu.h"

#define F_DEVICE         1 << 0
#define F_SERVICE        1 << 1
#define F_CHARACTERISTIC 1 << 2
#define F_PARAMETERS     1 << 3

#define MAX_DEVICES 10
#define MAX_SERVICES 10
#define MAX_CHARACTERISTICS 20
#define MAX_DEVICE_QUEUE_SIZE 10

struct meaCharacteristic_s {
   char characteristicUUID[37];
   int8_t type; // R|W|B
   uint32_t pp; // polling period
   uint32_t t;
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
   TaskHandle_t task;
   QueueHandle_t queue;
   
};

struct meaBLEServer_s {
   int8_t nbDevices;
   int8_t nbServices;
   int8_t nbCharacteristics;
   struct meaDevice_s meaDevices[MAX_DEVICES];
   struct meaService_s meaServices[MAX_SERVICES];
   struct meaCharacteristic_s meaCharacteristics[MAX_CHARACTERISTICS];
};

extern struct meaBLEServer_s meaBLEServers;


void initBLEServers();
int32_t addWatcher(char *addr, char *serviceUUID, char *characteristicUUID, char *parameters);
int32_t getContext(char *buff, char *addr, char *service, char *characteristic, char *parameters);
bool deleteDevice(int8_t device);
int stopDevice(int8_t deviceId);
int startDevice(int8_t deviceId);

bool get_value(int8_t characteristicId);
bool set_value(int8_t characteristicId, char *parameters);

int8_t findDeviceByAddress(char *address);
int8_t findServiceForDeviceByUUID(int8_t device, char *serviceUUID);
int8_t findCharacteristicForDeviceServiceByUUID(int8_t device, int8_t service, char *characteristicUUID);
int8_t findCharacteristic(char *addr, char *serviceUUID, char *characteristicUUID);
int getDeviceStateById(int8_t deviceId);

#endif