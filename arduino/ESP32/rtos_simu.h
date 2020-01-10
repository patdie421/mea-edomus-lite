#ifndef rtos_simu_h
#define rtos_simu_h

#include <pthread.h>

#include "rtos_simu.h"

#define BaseType_t int
#define UBaseType_t unsigned int
#define configSTACK_DEPTH_TYPE int
#define TickType_t int
typedef void(* TaskFunction_t) (void *);
#define TaskHandle_t pthread_t


struct simple_queue_s {
   int uxQueueLength;
   int uxItemSize;
   void *data;
   int in_ptr;
   int out_ptr;
   int nb_elem;
   int debug;
   pthread_mutex_t lock;
   
};

typedef struct simple_queue_s *QueueHandle_t;


void vTaskDelay( const TickType_t xTicksToDelay );
BaseType_t xQueueReceive(
   QueueHandle_t xQueue,
   void *pvBuffer,
   TickType_t xTicksToWait
);
BaseType_t xQueueSend(
   QueueHandle_t xQueue,
   const void * pvItemToQueue,
   TickType_t xTicksToWait
);
void vQueueDelete( QueueHandle_t xQueue );
QueueHandle_t xQueueCreate(UBaseType_t uxQueueLength, UBaseType_t uxItemSize);
void vTaskDelete( TaskHandle_t xTask );
BaseType_t xTaskCreate(
   TaskFunction_t pvTaskCode,
   const char * const pcName,
   configSTACK_DEPTH_TYPE usStackDepth,
   void *pvParameters,
   UBaseType_t uxPriority,
   TaskHandle_t *pxCreatedTask
);


#endif