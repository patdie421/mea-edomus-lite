//
//  interface_type_007.h
//  mea-edomus
//
//  Created by Patrice Dietsch on 13/01/2020.
//
//
#ifndef __interface_type_007_h
#define __interface_type_007_h

#include <signal.h>

#include "interfacesServer.h"
#include "xPLServer.h"

#define INTERFACE_TYPE_007 3007

#define INIT_LINE_BUFFER_SIZE 10
#define INC_LINE_BUFFER_SIZE  10

struct interface_type_007_indicators_s
{
   uint32_t xplin;
   uint32_t senttoplugin;
//   uint32_t xplout;
   uint32_t serialin;
   uint32_t serialout;
   
};

extern char *interface_type_007_xplin_str;
extern char *interface_type_007_senttoplugin_str;
extern char *interface_type_007_serialin_str;
extern char *interface_type_007_serialout_str;

typedef struct interface_type_007_s
{
   int              id_interface;
   int              id_driver;
   char            *parameters;
   char             name[256];
   char             dev[256];
   int              monitoring_id;
   
   pthread_t       *thread;
   volatile sig_atomic_t
                    thread_is_running;
   xpl2_f           xPL_callback2;
   void            *xPL_callback_data;
      
   volatile sig_atomic_t
                    pairing_state;
   double           pairing_startms;

   char             real_dev[256];
   int              real_speed;
   int              fd;

   int              fduration;
   char            *fendstr;
   char            *fstartstr;
   int              fsize;

   char            *line_buffer;
   int              line_buffer_l;
   int              line_buffer_ptr;
 
   struct interface_type_007_indicators_s indicators;

   char            *interface_plugin_name;
   char            *interface_plugin_parameters;
} interface_type_007_t;


struct interface_type_007_data_s
{
   interface_type_007_t *i007;
};


xpl2_f  get_xPLCallback_interface_type_007(void *ixxx);
int     get_monitoring_id_interface_type_007(void *ixxx);
int     set_xPLCallback_interface_type_007(void *ixxx, xpl2_f cb);
int     set_monitoring_id_interface_type_007(void *ixxx, int id);
int     get_type_interface_type_007(void);

int     clean_interface_type_007(void *ixxx);

int16_t api_interface_type_007(void *ixxx, char *cmnd, void *args, int nb_args, void **res, int16_t *nerr, char *err, int l_err);

int     get_fns_interface_type_007(struct interfacesServer_interfaceFns_s *interfacesFns);

#endif
