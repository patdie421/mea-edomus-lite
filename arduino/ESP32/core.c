#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
//#include <cstdlib.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define DEBUG 1

/****************************************************/
#define false 0
#define true 1
#define bool int

#define BaseType_t int
#define UBaseType_t unsigned int
#define configSTACK_DEPTH_TYPE int
#define TickType_t int

typedef void(* TaskFunction_t) (void *);
typedef void * TaskHandle_t;
typedef void * QueueHandle_t;


void vTaskDelay( const TickType_t xTicksToDelay )
{
   sleep(xTicksToDelay/1000);   
}


BaseType_t xQueueReceive(
   QueueHandle_t xQueue,
   void *pvBuffer,
   TickType_t xTicksToWait
)
{
   return 1;
}


BaseType_t xQueueSend(
   QueueHandle_t xQueue,
   const void * pvItemToQueue,
   TickType_t xTicksToWait
)
{
   return 1;
}
 

void vQueueDelete( QueueHandle_t xQueue )
{
}

                           
QueueHandle_t xQueueCreate(UBaseType_t uxQueueLength, UBaseType_t uxItemSize)
{
   return (void *)1;
}


void vTaskDelete( TaskHandle_t xTask )
{
}

BaseType_t xTaskCreate(
   TaskFunction_t pvTaskCode,
   const char * const pcName,
   configSTACK_DEPTH_TYPE usStackDepth,
   void *pvParameters,
   UBaseType_t uxPriority,
   TaskHandle_t *pxCreatedTask
)
{
   return 1;
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
#define DISPLAY(x) printf("%s", (x))

/****************************************************/
// #define SERIAL_AVAILABLE Serial.available
// #define SERIAL_READ Serial.read
// #define DISPLAY(x) Serial.print((x));

#define F_DEVICE         1 << 0
#define F_SERVICE        1 << 1
#define F_CHARACTERISTIC 1 << 2
#define F_PARAMETERS     1 << 3

#define MAX_DEVICES 10
#define MAX_SERVICES 10
#define MAX_CHARACTERISTICS 20
#define MAX_DEVICE_QUEUE_SIZE 10

struct device_queue_elem_s {
   uint8_t action;
   char data[20];
};


struct output_queue_elem_s {
};


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

struct meaBLEServer_s meaBLEServers;

TaskHandle_t inputTaskHandle;
TaskHandle_t outputTaskHandle;
QueueHandle_t outputTaskQueue;

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
                meaBLEServers.meaCharacteristics[i].pp);
      }
   }
}
#endif

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


void deviceTask(void *pvParameters)
{
   int8_t deviceId=(int8_t)((long)pvParameters)&0xFF;
   struct device_queue_elem_s elem;

   meaBLEServers.meaDevices[deviceId].state=2; // running state

   for(;;) {
      DISPLAY("DEVICE: ");
      DISPLAY(meaBLEServers.meaDevices[deviceId].address);
      DISPLAY("is running\n");
      
      BaseType_t xRet=xQueueReceive(
         meaBLEServers.meaDevices[deviceId].queue,
         &elem,
         1000
      );
      if(xRet) {
         // do query
      }
      // do BLE coms
   }
}


int stopDevice(int8_t deviceId)
{
   vTaskDelete(meaBLEServers.meaDevices[deviceId].task);
   vQueueDelete(meaBLEServers.meaDevices[deviceId].queue);
   meaBLEServers.meaDevices[deviceId].state=0;
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


#define ERROR_BAD_REQUEST 1
#define ERROR_BAD_ID      2
#define ERROR_BAD_ACTION  3

bool send_error(int err)
{
   switch(err) {
      case ERROR_BAD_REQUEST: DISPLAY("[ERROR][bad request]\n"); break;
      case ERROR_BAD_ID: DISPLAY("[ERROR][bad IDs]\n"); break;
      case ERROR_BAD_ACTION: DISPLAY("[ERROR][bad action]\n"); break;
      default: DISPLAY("[ERROR][unknown error]\n");
   }
   return false;
}


bool set_value(int8_t characteristicId, char *parameters)
{
   struct device_queue_elem_s elem;
   int8_t deviceId=meaBLEServers.meaCharacteristics[characteristicId].device;
   strncpy(elem.data, parameters, sizeof(elem.data)-1);
   elem.data[sizeof(elem.data)-1]=0;
   elem.action='S';
   BaseType_t xRet=xQueueSend(meaBLEServers.meaDevices[deviceId].queue,&elem,1000);

   DISPLAY("[VALUE][");
   printf("%d", characteristicId);
   DISPLAY("][");
   DISPLAY(parameters);
   DISPLAY("]\n");

   return true;
}


bool get_value(int8_t characteristicId)
{
   struct device_queue_elem_s elem;
   int8_t deviceId=meaBLEServers.meaCharacteristics[characteristicId].device;
   elem.data[0]=0;
   elem.action='S';
   BaseType_t xRet=xQueueSend(meaBLEServers.meaDevices[deviceId].queue,&elem,1000);

   DISPLAY("[VALUE][");
   printf("%d", characteristicId);
   DISPLAY("]\n");

   return true;
}


bool send_ok()
{
   printCharacteristics();
   
   DISPLAY("[ok]\n");
   
   return true;
}


bool doOutput()
{
   struct output_queue_elem_s elem;
   
   BaseType_t xRet=xQueueReceive(
      outputTaskQueue,
      &elem,
      1000
   );
   if(xRet) {
      // do query
   }
}


bool doRequest()
{
   char buff[256]="";
   char *_buff=buff;

   uint8_t action='\0';
   char deviceAddr[18]="";
   char serviceUUID[36]="";
   char characteristicUUID[36]="";
   char parameters[80]="";

   readLine(buff, sizeof(buff)-1);

   if(buff[1]==';') {
      switch(buff[0]) {
         case '+': // add
         case '-': // delete
         case '?': // query
         case '#': // set
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
   
   bool ret=getContext(_buff, deviceAddr, serviceUUID, characteristicUUID, parameters);
   if(ret) {
      switch(action) {
         case '+':
            if(ret == (F_DEVICE | F_SERVICE | F_CHARACTERISTIC | F_PARAMETERS)) {
               int32_t ret=addWatcher(deviceAddr, serviceUUID, characteristicUUID, parameters);
               if((ret & 0xFF000000)!=0) {
                  int8_t deviceId=ret & 0xFF;
                  if(ret!=-1 && meaBLEServers.meaDevices[deviceId].state==0) {
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


void outputTask(void *pvParameters)
{
   for(;;) {
      doOutput();
   }
}


void inputTask(void *pvParameters)
{
   for(;;) {
      doRequest();
   }
}


int setup()
{
   initBLEServers();
   
   outputTaskQueue = xQueueCreate(MAX_DEVICE_QUEUE_SIZE, sizeof(struct output_queue_elem_s));
   if(outputTaskQueue) {
      BaseType_t xRet=xTaskCreate(outputTask, "OUTPUT", 1000, (void *)0, 2, &outputTaskHandle);
      if(xRet) {
         DISPLAY("output task OK\n");
      }
      else {
         DISPLAY("output task KO\n");
      }
   }
   else {
      DISPLAY("can't create output queue\n");
   }
   
   BaseType_t xRet=xTaskCreate(outputTask, "INPUT", 1000, (void *)0, 1, &inputTaskHandle);
   if(xRet) {
      DISPLAY("input task OK\n");
   }
   else {
      DISPLAY("input task KO\n");
   }
}


int loop()
{
   doRequest();
}


int main(int argc, char *argv[])
{
   setup();
   
   while(1) {
      loop();
   }
}