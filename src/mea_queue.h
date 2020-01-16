/*
 *  mea_queue.h
 *
 *  Created by Patrice Dietsch on 28/07/12.
 *  Copyright 2012 -. All rights reserved.
 *
 */

#ifndef __mea_queue_h
#define __mea_queue_h

#include "mea_error.h"

#define QUEUE_ENABLE_INDEX 1

struct mea_queue_elem
{
   void *d;
   struct mea_queue_elem *next;
   struct mea_queue_elem *prev;
};
#define MEA_QUEUE_ELEM_INIT { .d=NULL, .next=NULL, .prev=NULL }

typedef void (*mea_queue_free_data_f)(void *);
typedef int (*mea_queue_compare_data_f)(void **, void **);

typedef struct
{
   int clone_flag;
   struct mea_queue_elem *first;
   struct mea_queue_elem *last;
   struct mea_queue_elem *current;
   unsigned long nb_elem;
#ifdef QUEUE_ENABLE_INDEX
   void   **index;
   char   index_status; // 1 = utilisable, 0 = non utilisable (doit être reconstruit)
   mea_queue_compare_data_f index_order_f;
#endif
} mea_queue_t;
#ifndef QUEUE_ENABLE_INDEX
#define MEA_QUEUE_T_INIT { .clone_flag=0, .first=NULL, .last=NULL, .current=NULL, .nb_elem=0 }
#else
#define MEA_QUEUE_T_INIT { .clone_flag=0, .first=NULL, .last=NULL, .current=NULL, .nb_elem=0, .index=NULL, .index_status=0, .index_order_f=NULL }
#endif
mea_error_t mea_queue_init(mea_queue_t *queue);
mea_error_t mea_queue_clone(mea_queue_t *n, mea_queue_t *s);
mea_error_t mea_queue_out_elem(mea_queue_t *queue, void **data);
mea_error_t mea_queue_in_elem(mea_queue_t *queue, void *data);
mea_error_t mea_queue_process_all_elem_data(mea_queue_t *queue, void (*f)(void *));
mea_error_t mea_queue_first(mea_queue_t *queue);
mea_error_t mea_queue_last(mea_queue_t *queue);
mea_error_t mea_queue_next(mea_queue_t *queue);
mea_error_t mea_queue_prev(mea_queue_t *queue);
mea_error_t mea_queue_current(mea_queue_t *queue, void **data);
mea_error_t mea_queue_cleanup(mea_queue_t *queue, mea_queue_free_data_f f);
mea_error_t mea_queue_remove_current(mea_queue_t *queue);
unsigned long mea_queue_nb_elem(mea_queue_t *queue);
void *mea_queue_get_data(void *e);

mea_error_t mea_queue_find_elem(mea_queue_t *queue, mea_queue_compare_data_f, void *data_to_find, void **data);

#ifdef QUEUE_ENABLE_INDEX
mea_error_t mea_queue_create_index(mea_queue_t *queue, mea_queue_compare_data_f f);
mea_error_t mea_queue_recreate_index(mea_queue_t *queue, char force);
mea_error_t mea_queue_remove_index(mea_queue_t *queue);
mea_error_t mea_queue_find_elem_using_index(mea_queue_t *queue, void *data_to_find, void **data);
mea_error_t mea_queue_get_elem_by_index(mea_queue_t *queue, int i, void **data);
char mea_queue_get_index_status(mea_queue_t *queue);
#endif

#endif
