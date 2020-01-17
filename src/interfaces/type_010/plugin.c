#ifdef ASPLUGIN
#include <stdio.h>
#include <dlfcn.h>

#include "interfacesServer.h"
#include "interface_type_010.h"

int get_fns_interface(void *lib, struct interfacesServer_interfaceFns_s *interfacesFns)
{
   if(lib) {
      interfacesFns->malloc_and_init2 = (malloc_and_init2_f)dlsym(lib, "malloc_and_init2_interface_type_010");
      if(!interfacesFns->malloc_and_init2) {
         fprintf(stderr,"malloc_and_init2: %s\n", dlerror());
      }
      interfacesFns->get_monitoring_id = (get_monitoring_id_f)dlsym(lib, "get_monitoring_id_interface_type_010");
      if(!interfacesFns->get_monitoring_id) {
         fprintf(stderr,"get_monitoring_id: %s\n", dlerror());
      }
      interfacesFns->get_xPLCallback = (get_xPLCallback_f)dlsym(lib, "get_xPLCallback_interface_type_010");
      if(!interfacesFns->get_xPLCallback) {
         fprintf(stderr,"get_xPLCallback: %s\n", dlerror());
      }
      interfacesFns->update_devices = (update_devices_f)dlsym(lib, "update_devices_type_010");
      if(!interfacesFns->update_devices) {
         fprintf(stderr,"update_devies: %s\n", dlerror());
      }
      interfacesFns->clean = (clean_f)dlsym(lib, "clean_interface_type_010");
      if(!interfacesFns->get_xPLCallback) {
         fprintf(stderr,"get_xPLCallback: %s\n", dlerror());
      }
      interfacesFns->set_monitoring_id = (set_monitoring_id_f)dlsym(lib, "set_monitoring_id_interface_type_010");
      if(!interfacesFns->set_monitoring_id) {
         fprintf(stderr,"set_monitoring_id: %s\n", dlerror());
      }
      interfacesFns->set_xPLCallback = (set_xPLCallback_f)dlsym(lib, "set_xPLCallback_interface_type_010");
      if(!interfacesFns->set_xPLCallback) {
         fprintf(stderr,"set_xPLCallback: %s\n", dlerror());
      }
      interfacesFns->get_type = (get_type_f)dlsym(lib, "get_type_interface_type_010");
      if(!interfacesFns->get_type) {
         fprintf(stderr,"get_type: %s\n", dlerror());
      }
      interfacesFns->api = (api_f)dlsym(lib, "api_interface_type_010_json");
      if(!interfacesFns->api) {
         fprintf(stderr,"api: %s\n", dlerror());
      }
      interfacesFns->pairing = (pairing_f)dlsym(lib, "pairing_interface_type_010");
      if(!interfacesFns->pairing) {
         fprintf(stderr,"pairing: %s\n", dlerror());
      }
/*
      interfacesFns->get_interface_id = (get_interface_id_f)dlsym(lib, "get_interface_id_interface_type_010");
      if(!interfacesFns->get_interface_id)
         fprintf(stderr,"get_interface_id: %s\n", dlerror());
*/
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
