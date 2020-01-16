/*
 *  queue.c
 *
 *  Created by Patrice Dietsch on 28/07/12.
 *  Copyright 2012 -. All rights reserved.
 *
 */
#include "mea_queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>


unsigned long mea_queue_nb_elem(mea_queue_t *queue)
{
   if(!queue) {
      return -1;
   }

   return queue->nb_elem;
}


mea_error_t mea_queue_in_elem(mea_queue_t *queue, void *data)
{
   struct mea_queue_elem *new;
   
   if(!queue) {
      return ERROR;
   }

   new=(struct mea_queue_elem *)malloc(sizeof(struct mea_queue_elem));
   if(!new) {
      return ERROR;
   }
   
   new->d=data;
   
   if(queue->first==NULL) {
      queue->first=new;
      queue->last=new;
      new->next=NULL;
      new->prev=NULL;
   }
   else {
      new->next=queue->first;
      new->prev=NULL;
      queue->first->prev=new;
      queue->first=new;
   }
   
   queue->nb_elem++;
#ifdef QUEUE_ENABLE_INDEX
   queue->index_status=0;
#endif
   
   return NOERROR;
}


mea_error_t mea_queue_out_elem(mea_queue_t *queue, void **data)
{
   struct mea_queue_elem *ptr=NULL;
   
   if(!queue) {
      return ERROR;
   }
   
   if(queue->last) {
      ptr=queue->last;
      
      if(queue->last == queue->first) {
         queue->last=NULL;
         queue->first=NULL;
      }
      else {
         queue->last=queue->last->prev;
         queue->last->next=NULL;
      }
      *data=ptr->d;
      free(ptr);
      ptr=NULL;
   }
   else {
      return ERROR;
   }
   
   queue->nb_elem--;
#ifdef QUEUE_ENABLE_INDEX
   queue->index_status=0;
#endif
   
   return NOERROR;
}


mea_error_t mea_queue_init(mea_queue_t *queue)
{
   if(!queue) {
      return ERROR;
   }

   queue->clone_flag=0;
   queue->first=NULL;
   queue->last=NULL;
   queue->current=NULL;
   queue->nb_elem=0;

#ifdef QUEUE_ENABLE_INDEX
   queue->index=NULL;
   queue->index_status=0;
   queue->index_order_f=NULL;
#endif
   
   return NOERROR;
}


mea_error_t mea_queue_clone(mea_queue_t *n, mea_queue_t *s)
{
   if(!s) {
      return ERROR;
   }

   n->clone_flag=1;
   n->first=s->first;
   n->last=s->last;
   n->nb_elem=s->nb_elem;
   n->current=NULL;

#ifdef QUEUE_ENABLE_INDEX
   n->index=NULL;
   n->index_status=0;
   n->index_order_f=NULL;
#endif

   return NOERROR;
}


mea_error_t mea_queue_first(mea_queue_t *queue)
{
    if(!queue || !queue->first) {
      return ERROR;
    }
   
   queue->current=queue->first;
   return NOERROR;
}


mea_error_t mea_queue_last(mea_queue_t *queue)
{
   if(!queue || !queue->last) {
      return ERROR;
   }
   
   queue->current=queue->last;
   return NOERROR;
}


mea_error_t mea_queue_next(mea_queue_t *queue)
{
   if(!queue || !queue->current) {
      return ERROR;
   }
   
   if(!queue->current->next) {
      queue->current=NULL;
      return ERROR;
   }
   
   queue->current=queue->current->next;
   return NOERROR;
}


mea_error_t mea_queue_prev(mea_queue_t *queue)
{
   if(!queue || !queue->current) {
      return ERROR;
   }

   if(!queue->current->prev) {
      queue->current=NULL;
      return ERROR;
   }
   
   queue->current=queue->current->prev;
   
   return NOERROR;
}


mea_error_t mea_queue_cleanup(mea_queue_t *queue, mea_queue_free_data_f f)
{
   struct mea_queue_elem *ptr=NULL;
   
   if(!queue) {
      return ERROR;
   }

   while(queue->nb_elem>0) {
      if(queue->last) {
         ptr=queue->last;
         
         if(queue->last == queue->first) {
            queue->last=NULL;
            queue->first=NULL;
         }
         else {
            queue->last=queue->last->prev;
            queue->last->next=NULL;
         }
         if(ptr->d) {
            if(f) {
               f(ptr->d);
            }
         }
         free(ptr);
         ptr=NULL;
      }
      else {
         return NOERROR;
      }
      
      queue->nb_elem--;    
   }

#ifdef QUEUE_ENABLE_INDEX
   if(queue->index) {
      free(queue->index);
      queue->index=NULL;
   }
   queue->index_order_f=NULL;
   queue->index_status=0;
#endif
   
   return NOERROR;
}


mea_error_t mea_queue_current(mea_queue_t *queue, void **data)
{
   if(!queue) {
      *data=NULL;
      return ERROR;
   }
   if(!queue->current) {
      *data=NULL;
      return ERROR;
   }
   *data=queue->current->d;
   
   return NOERROR;
}


mea_error_t mea_queue_remove_current(mea_queue_t *queue)
{
   if(!queue) {
      return ERROR;
   }

   if(queue->nb_elem==0) {
      return ERROR;
   }
   
   if(queue->nb_elem==1) {
      free(queue->current);
      queue->current=NULL;
      queue->first=NULL;
      queue->last=NULL;
      queue->nb_elem=0;
      return NOERROR;
   }
   
   struct mea_queue_elem *prev=NULL;
   struct mea_queue_elem *next=NULL;
   struct mea_queue_elem *old=NULL;

   prev=queue->current->prev;
   next=queue->current->next;
   old=queue->current;
   
   if(prev) {
      prev->next=queue->current->next;
   }
   
   if(next) {
      next->prev=queue->current->prev;
   }
   
   if(queue->current==queue->first) {
      queue->first=queue->current->next;
   }
   
   if(queue->current==queue->last) {
      queue->last=queue->current->prev;
   }
   
   queue->nb_elem--;
   
   queue->current=next;
   
   free(old);

#ifdef QUEUE_ENABLE_INDEX
   queue->index_status=0;
#endif
   
   return NOERROR;
}


mea_error_t mea_queue_process_all_elem_data(mea_queue_t *queue, void (*f)(void *))
{
   struct mea_queue_elem *ptr=NULL;
   
   if(!queue) {
      return ERROR;
   }

   ptr=queue->first;
   if(!ptr) {
      return ERROR;
   }
   do {
      f(ptr->d);
      ptr=ptr->next;
      
   } while (ptr);
      
   return NOERROR;
}


mea_error_t mea_queue_find_elem(mea_queue_t *queue, mea_queue_compare_data_f cmpf, void *data_to_find, void **data)
{
   struct mea_queue_elem *ptr=NULL;
   
   *data=NULL;
   if(!queue) {
      return ERROR;
   }

   ptr=queue->first;
   if(!ptr) {
      return ERROR;
   }
   do {
      if(cmpf(data_to_find, ptr->d)==0) {
         *data=ptr->d;
         return NOERROR;
      }
      ptr=ptr->next;
      
   } while (ptr);
   
   return NOERROR;
}


#ifdef QUEUE_ENABLE_INDEX
mea_error_t mea_queue_get_elem_by_index(mea_queue_t *queue, int i, void **data)
{
   *data=NULL;
   if(!queue || !queue->index) {
      return ERROR;
   }
   if(queue->index_status!=1) {
      return ERROR;
   }

   if(i>=0 && i<queue->nb_elem) {
      *data=queue->index[i];
      return NOERROR;
   }
   else {
      return ERROR;
   }
}


static int _create_index(mea_queue_t *queue)
{
   if(queue->nb_elem<=0) {
      return ERROR;
   }

   if(!queue) {
      return ERROR;
   }

   int i=0;
   struct mea_queue_elem *ptr=NULL;
   ptr=queue->first;
   do {
      queue->index[i++]=ptr->d;
      ptr=ptr->next;

   } while (ptr);

#ifdef __linux__
#else
   typedef int (*__compar_fn_t)(const void *, const void *);
#endif
   qsort(queue->index, queue->nb_elem, sizeof(char *), (__compar_fn_t)queue->index_order_f);
   
   return NOERROR;
}


mea_error_t mea_queue_create_index(mea_queue_t *queue, mea_queue_compare_data_f f)
{
   if(!queue) {
      return ERROR;
   }

   if(queue->index) {
      free(queue->index);
      queue->index=NULL;
   }

   queue->index_order_f = f;
   queue->index_status = 0;
   
   if(queue->nb_elem>0) {
      queue->index=malloc(queue->nb_elem * sizeof(void *));
      if(!queue->index) {
         return ERROR;
      }

      if(_create_index(queue)!=ERROR) {
         queue->index_status = 1;
         return NOERROR;
      }
      else {
         queue->index_status = 0;
         free(queue->index);
         queue->index=NULL;
         return ERROR;
      }
   }
   
   return NOERROR;
}


mea_error_t mea_queue_recreate_index(mea_queue_t *queue, char force)
{
   void  **old_index=NULL;
   
   if(!queue) {
      return ERROR;
   }

   if(queue->index_status==1 && force==0) {
      return NOERROR;
   }
      
   if(!queue->first) {
      return NOERROR;
   }
      
   if(!queue->index_order_f) {
      return ERROR;
   }
      
   if(queue->index) {
      old_index=queue->index;
   }

   queue->index=malloc(queue->nb_elem * sizeof(void *));
   if(!queue->index) {
      queue->index=old_index;
      return ERROR;
   }

   if(_create_index(queue)!=ERROR) {
      queue->index_status = 1;
      if(old_index) {
         free(old_index);
         old_index=NULL;
      }
      return NOERROR;
   }
   else {
      queue->index=old_index;
      return ERROR;
   }
}


mea_error_t mea_queue_remove_index(mea_queue_t *queue)
{
   if(!queue) {
      return ERROR;
   }
   if(!queue->index) {
      return NOERROR;
   }
   free(queue->index);
   queue->index=NULL;
   queue->index_order_f = NULL;
   queue->index_status = 0;
   
   return NOERROR;
}


mea_error_t mea_queue_find_elem_using_index(mea_queue_t *queue, void *data_to_find, void **data)
{
   if(!queue || !queue->index || !queue->index_status || !queue->nb_elem) {
      return ERROR;
   }
   unsigned long start = 0;
   unsigned long end = queue->nb_elem - 1;
   unsigned long middle = 0;
   int _cmpres = 0;

   *data=NULL;

   struct mea_queue_elem _tmp_qe;

   _tmp_qe.d=data_to_find;
   _tmp_qe.next=NULL;
   _tmp_qe.prev=NULL;

   do {
      middle = (end + start) / 2;
      _cmpres=queue->index_order_f(&data_to_find, &(queue->index[middle]));
      if(_cmpres==0) {
         *data=queue->index[middle];
         return NOERROR;
      }
      if(_cmpres>0) {
         start=middle+1;
      }
      else {
         end=middle-1;
      }

   }
   while(start<=end);
   
   return ERROR;
}


void *mea_queue_get_data(void *e)
{
   struct mea_queue_elem *_e = (struct mea_queue_elem *)e;

   return _e->d;
}


char mea_queue_get_index_status(mea_queue_t *queue)
{
   return queue->index_status;
}
#endif

#ifdef MEA_QUEUE_MODULE_R7
#include <stdio.h>
#include <string.h>

void clnf(void *data)
{
   char *s=(char *)data;
   printf("clean %s\n", s);
//   free(s);
}


int cmpf(void **p1, void **p2)
{
   char *_p1 = (char *)*p1;
   char *_p2 = (char *)*p2;

   return strcmp(_p1, _p2);
}


char *d1[]={"D3", "A1", "d1", "d2", "d3", "Z1", NULL};
char *d2[]={"AA1","aa2", NULL};
char *d3="toto";

int main(int argc, char *argv[])
{
   mea_queue_t q=MEA_QUEUE_ELEM_INIT;
   char *str=NULL;
   int ret=-1;

   mea_queue_init(&q);

   printf("\nTEST1 : construction queue\n");
   int i=0;
   for(;d1[i];i++) {
      int _l_d=strlen(d1[i])+1;
      char *_d=(char *)malloc(_l_d);
      strncpy(_d,d1[i],_l_d-1);
      _d[_l_d-1]=0; 
      mea_queue_in_elem(&q, _d);
   }
   printf("queue index status = %d\n", mea_queue_get_index_status(&q));

   printf("\nTEST2 : construction index\n");
   mea_queue_create_index(&q, cmpf);
   printf("queue index status = %d\n", mea_queue_get_index_status(&q));

   printf("\nTEST3 : affichage queue triée\n");
   for(i=0;i<q.nb_elem;i++) {
      mea_queue_get_elem_by_index(&q, i, (void **)&str);
      printf("index %d = %s (%p)\n", i, str, str);
   }
   printf("\n");

 
   printf("\nTEST4 : ajout de nouveaux éléments\n");
   i=0;
   for(;d2[i];i++) {
      int _l_d=strlen(d2[i])+1;
      char *_d=(char *)malloc(_l_d);
      strncpy(_d,d2[i],_l_d-1);
      _d[_l_d-1]=0; 
      mea_queue_in_elem(&q, _d);
   }
   printf("queue index status = %d\n", mea_queue_get_index_status(&q));

   printf("\nTEST5 : reconstruction index\n");
   mea_queue_recreate_index(&q, 1);
   printf("queue index status = %d\n", mea_queue_get_index_status(&q));


   printf("\nTEST6 : affichage queue triée\n");
   for(i=0;i<q.nb_elem;i++) {
      mea_queue_get_elem_by_index(&q, i, (void **)&str);
      printf("index %d = %s\n", i, str);
   }
   printf("\n");

   printf("\nTEST7 : recherche d'un élément\n");
   ret=mea_queue_find_elem_using_index(&q, (void *)"toto", (void *)&str);   
   printf("ret=%d %s\n",ret,str); 
   printf("\n");

   ret=mea_queue_find_elem_using_index(&q, (void *)"d2", (void *)&str);   
   printf("ret=%d %s\n",ret,str); 
   printf("\n");

   mea_queue_in_elem(&q, d3);
   ret=mea_queue_find_elem_using_index(&q, (void *)"toto", (void *)&str);   
   printf("ret=%d %s\n",ret,str); 
   printf("\n");
   mea_queue_cleanup(&q, clnf);
}
#endif
