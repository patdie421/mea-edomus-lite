//
//  interface_type_006.h
//  mea-edomus
//
//  Created by Patrice Dietsch on 16/11/2015.
//
//
#ifndef __interface_type_006_h
#define __interface_type_006_h

/*
#ifdef ASPLUGIN
#define NAME(f) f ## _ ## PLGN
#else
#define NAME(f) f
#endif
*/
#include <signal.h>
#include <Python.h>

#include "interfacesServer.h"
#include "xPLServer.h"
#include "pythonPluginServer.h"

#define INTERFACE_TYPE_006 465

struct interface_type_006_indicators_s
{
   uint32_t senttoplugin;
   uint32_t xplin;
   uint32_t xplout;
   uint32_t serialin;
   uint32_t serialout;
};

extern char *interface_type_006_senttoplugin_str;
extern char *interface_type_006_xplin_str;
extern char *interface_type_006_serialin_str;

typedef struct interface_type_006_s
{
   int              id_interface;
   char             name[41];
   char             dev[81];

   char             real_dev[81];
   int              real_speed;
   int              fd;

   char            *parameters;
   int              monitoring_id;
   pthread_t       *thread;
   volatile sig_atomic_t thread_is_running;
   xpl2_f           xPL_callback2;
   void            *xPL_callback_data;

   PyThreadState   *mainThreadState; 
   PyThreadState   *myThreadState; 

   PyObject      *pModule, *pFunc, *pParams;

   struct interface_type_006_indicators_s indicators;
} interface_type_006_t;


struct interface_type_006_data_s
{
   interface_type_006_t *i006;
};


xpl2_f get_xPLCallback_interface_type_006(void *ixxx);
int get_monitoring_id_interface_type_006(void *ixxx);
int set_xPLCallback_interface_type_006(void *ixxx, xpl2_f cb);
int set_monitoring_id_interface_type_006(void *ixxx, int id);
int get_type_interface_type_006(void);

int clean_interface_type_006(void *ixx);

#ifndef ASPLUGIN
int get_fns_interface_type_006(struct interfacesServer_interfaceFns_s *interfacesFns);
#endif

#endif
