#include <stdio.h>
#include <stdint.h>
#define __USE_BSD
#include <unistd.h>
#undef __USE_BSD
#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>


#include "rtos_simu.h"

#define TICK 1000 // 1 milliseconde

void vTaskDelay( const TickType_t xTicksToDelay )
{
   usleep(TICK*xTicksToDelay);
}


BaseType_t xQueueReceive(
   QueueHandle_t xQueue,
   void *pvBuffer,
   TickType_t xTicksToWait
)
{
   int t=(int)xTicksToWait;
   
   while(1) {
      pthread_mutex_lock(&(xQueue->lock));
      if(xQueue->nb_elem>0) {
         memcpy(pvBuffer, (void *)(xQueue->data+xQueue->uxItemSize*xQueue->out_ptr), xQueue->uxItemSize);
         if(xQueue->out_ptr!=xQueue->uxItemSize) {
            xQueue->out_ptr++;
         }
         else {
            xQueue->out_ptr=0;
         }
         xQueue->nb_elem--;
         pthread_mutex_unlock(&(xQueue->lock));
         return 1;
      }
      else {
         pthread_mutex_unlock(&(xQueue->lock));
         if(t==0) {
            return 0;
         }
         t--;
         usleep(TICK);
      }
   }
   return 1;
}


BaseType_t xQueueSend(
   QueueHandle_t xQueue,
   const void * pvItemToQueue,
   TickType_t xTicksToWait
)
{
   int t=(int)xTicksToWait;
   
   while(1) {
      pthread_mutex_lock(&(xQueue->lock));
      if(xQueue->nb_elem < xQueue->uxQueueLength) {
         memcpy((void *)(xQueue->data+xQueue->uxItemSize*xQueue->in_ptr), pvItemToQueue,xQueue->uxItemSize);
         if(xQueue->in_ptr!=xQueue->uxItemSize) {
            xQueue->in_ptr++;
         }
         else {
            xQueue->in_ptr=0;
         }
         xQueue->nb_elem++;
         pthread_mutex_unlock(&(xQueue->lock));
         return 1;
      }
      else {
         pthread_mutex_unlock(&(xQueue->lock));
         if(t==0) {
            return 0;
         }
         t--;
         usleep(TICK);
      }
   }
}
 

void vQueueDelete(QueueHandle_t xQueue)
{
   pthread_mutex_destroy(&(xQueue->lock));
   free(xQueue->data);
   free(xQueue);
}

                           
QueueHandle_t xQueueCreate(UBaseType_t uxQueueLength, UBaseType_t uxItemSize)
{
   QueueHandle_t queue=malloc(sizeof(struct simple_queue_s));
   
   queue->uxQueueLength=uxQueueLength;
   queue->uxItemSize=uxItemSize;
   queue->data=malloc(uxQueueLength*uxItemSize);
   queue->nb_elem=0;
   queue->in_ptr=0;
   queue->out_ptr=0;
   queue->debug=0;
   pthread_mutex_init(&(queue->lock), NULL);

   return queue;
}


void vTaskDelete(TaskHandle_t xTask)
{
   pthread_cancel(xTask);
   pthread_join(xTask, NULL);
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
   pthread_t _thread=NULL;

   if(pthread_create(pxCreatedTask, NULL, (void *)pvTaskCode, pvParameters)) {
      perror(__func__);
      return 0;
   }

   return 1;
}
