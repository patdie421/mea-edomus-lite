#ifdef ASPLUGIN

#ifdef __APPLE__
#include <Python/Python.h>
#else
#include <Python.h>
#endif

#include <stdio.h>
#include <dlfcn.h>

#include "interfacesServer.h"
#include "interface_type_005.h"

// dlsym(interface_type_005_lib, "get_type_interface_type_005");

int get_fns_interface(void *lib, struct interfacesServer_interfaceFns_s *interfacesFns)
{
   if(lib)
   {
      interfacesFns->malloc_and_init2 = (malloc_and_init2_f)dlsym(lib, "malloc_and_init2_interface_type_005");
      if(!interfacesFns->malloc_and_init2)
         fprintf(stderr,"malloc_and_init2: %s\n", dlerror());

      interfacesFns->get_monitoring_id = (get_monitoring_id_f)dlsym(lib, "get_monitoring_id_interface_type_005");
      if(!interfacesFns->get_monitoring_id)
         fprintf(stderr,"get_monitoring_id: %s\n", dlerror());

      interfacesFns->get_xPLCallback = (get_xPLCallback_f)dlsym(lib, "get_xPLCallback_interface_type_005");
      if(!interfacesFns->get_xPLCallback)
         fprintf(stderr,"get_xPLCallback: %s\n", dlerror());

      interfacesFns->clean = (clean_f)dlsym(lib, "clean_interface_type_005");
      if(!interfacesFns->clean)
         fprintf(stderr,"clean: %s\n", dlerror());

      interfacesFns->set_monitoring_id = (set_monitoring_id_f)dlsym(lib, "set_monitoring_id_interface_type_005");
      if(!interfacesFns->set_monitoring_id)
         fprintf(stderr,"set_monitoring_id: %s\n", dlerror());

      interfacesFns->set_xPLCallback = (set_xPLCallback_f)dlsym(lib, "set_xPLCallback_interface_type_005");
      if(!interfacesFns->set_xPLCallback)
         fprintf(stderr,"set_xPLCallback: %s\n", dlerror());

      interfacesFns->get_type = (get_type_f)dlsym(lib, "get_type_interface_type_005");
      if(!interfacesFns->get_type)
         fprintf(stderr,"get_type: %s\n", dlerror());
      
      interfacesFns->api=NULL;
      interfacesFns->pairing=NULL;
      interfacesFns->plugin_flag = 1;
      interfacesFns->lib = lib;

         return 0;
   }
   else {
      interfacesFns->plugin_flag = -1;
      interfacesFns->lib = NULL;

      return -1;
   }
}
#endif
