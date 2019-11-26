//
//  python_utils.c
//  mea-eDomus
//
//  Created by Patrice Dietsch on 04/06/13.
//
//
#ifdef __APPLE__
#include <Python/Python.h>
#else
#include <Python.h>
#endif
#include <stdio.h>

#include "cJSON.h"
#include "interfacesServer.h"

#include "mea_verbose.h"
#include "tokens.h"
#include "tokens_da.h"

#include "python_utils.h"


PyObject *mea_getMemory(PyObject *self, PyObject *args, PyObject *mea_memory)
{
   PyObject *key;
   PyObject *mem;
   
   if(PyTuple_Size(args)!=1) {
      DEBUG_SECTION mea_log_printf("%s (%s) :  arguments error.\n", DEBUG_STR ,__func__);
      PyErr_BadArgument(); // à replacer
      return NULL;
   }
   
   key=PyTuple_GetItem(args, 0);
   if(!key) {
      DEBUG_SECTION mea_log_printf("%s (%s) :  bad mem id.\n", DEBUG_STR ,__func__);
      PyErr_BadArgument(); // à remplacer
      return NULL;
   }
         
   mem=PyDict_GetItem(mea_memory, key);
   if(!mem) {
      mem = PyDict_New();
      if(PyDict_SetItem(mea_memory, key, mem)==-1) {
         Py_DECREF(mem);
         
         return NULL;
      }
   }
   Py_INCREF(mem);
   
   return mem;
}


void mea_addLong_to_pydict(PyObject *data_dict, char *key, long value)
{
   PyObject * val = PyLong_FromLong((long)value);
   PyDict_SetItemString(data_dict, key, val);
   Py_DECREF(val);
}


void mea_addDouble_to_pydict(PyObject *data_dict, char *key, double value)
{
   PyObject * val = PyFloat_FromDouble((double)value);
   PyDict_SetItemString(data_dict, key, val);
   Py_DECREF(val);
}


void mea_addString_to_pydict(PyObject *data_dict, char *key, char *value)
{
   PyObject *val = PyString_FromString(value);
   PyDict_SetItemString(data_dict, key, val);
   Py_DECREF(val);
}


void mea_addpydict_to_pydict(PyObject *data_dict, char *key, PyObject *adict)
{
   PyDict_SetItemString(data_dict, key, adict);
   Py_DECREF(adict);
}


PyObject *mea_device_info_to_pydict_device(struct device_info_s *device_info)
{
   PyObject *data_dict=NULL;

   data_dict=PyDict_New();
   if(!data_dict)
      return NULL;

   mea_addLong_to_pydict(data_dict, get_token_string_by_id(DEVICE_ID_ID), device_info->id);
   mea_addString_to_pydict(data_dict, get_token_string_by_id(DEVICE_NAME_ID), device_info->name);
   mea_addLong_to_pydict(data_dict, get_token_string_by_id(DEVICE_STATE_ID), device_info->state);
   mea_addLong_to_pydict(data_dict, get_token_string_by_id(DEVICE_TYPE_ID_ID), device_info->type_id);
   mea_addLong_to_pydict(data_dict, get_token_string_by_id(DEVICE_LOCATION_ID_ID), device_info->location_id);
   mea_addLong_to_pydict(data_dict, get_token_string_by_id(INTERFACE_ID_ID), device_info->interface_id);
   mea_addString_to_pydict(data_dict, get_token_string_by_id(DEVICE_INTERFACE_NAME_ID), device_info->interface_name);
   mea_addString_to_pydict(data_dict, get_token_string_by_id(DEVICE_INTERFACE_TYPE_NAME_ID), device_info->type_name);
   mea_addString_to_pydict(data_dict, get_token_string_by_id(DEVICE_TYPE_PARAMETERS_ID), device_info->type_parameters);
   mea_addLong_to_pydict(data_dict, get_token_string_by_id(TODBFLAG_ID), device_info->todbflag);
   mea_addLong_to_pydict(data_dict, get_token_string_by_id(TYPEOFTYPE_ID), device_info->typeoftype_id);

   return data_dict;
}


int mea_call_python_function3(PyObject *pFunc, PyObject *plugin_params_dict, PyObject **res)
{
   PyObject *pArgs, *pValue=NULL;
   int retour=0;

   if (pFunc && PyCallable_Check(pFunc)) {
      pArgs = PyTuple_New(1);
      Py_INCREF(plugin_params_dict); // PyTuple_SetItem va voler la référence, on en rajoute une pour pouvoir ensuite faire un Py_DECREF
      PyTuple_SetItem(pArgs, 0, plugin_params_dict);

      pValue = PyObject_CallObject(pFunc, pArgs); // appel du plugin
      if (pValue != NULL) {
         *res=pValue;
         retour=0;
      }
      else {
         if (PyErr_Occurred()) {
            VERBOSE(5) {
               mea_log_printf("%s (%s) : python error - ", ERROR_STR, __func__ );
               PyErr_Print();
               fprintf(MEA_STDERR, "\n");
            }
            PyErr_Clear();
            retour=-1;
         }
         *res=NULL;
      }
      Py_DECREF(pArgs);

      return retour;
   }
   else {
      return -1;
   }
}


int mea_call_python_function2(PyObject *pFunc, PyObject *plugin_params_dict)
{
   PyObject *pArgs, *pValue=NULL;
   int retour=-1;

   if (pFunc && PyCallable_Check(pFunc)) {
      pArgs = PyTuple_New(1);
      Py_INCREF(plugin_params_dict); // PyTuple_SetItem va voler la référence, on en rajoute une pour pouvoir ensuite faire un Py_DECREF
      PyTuple_SetItem(pArgs, 0, plugin_params_dict);

      pValue = PyObject_CallObject(pFunc, pArgs); // appel du plugin
      if (pValue != NULL) {
         retour=(int)PyInt_AsLong(pValue);
         Py_DECREF(pValue);
         DEBUG_SECTION mea_log_printf("%s (%s) : Result of call : %d\n", DEBUG_STR, __func__, retour);
      }
      else {
         if (PyErr_Occurred()) {
            VERBOSE(5) {
               mea_log_printf("%s (%s) : python error - ", ERROR_STR, __func__ );
               PyErr_Print();
               fprintf(MEA_STDERR, "\n");
            }
         PyErr_Clear();
         }
      }
      Py_DECREF(pArgs);

      return retour;
   }
   else {
      return -1;
   }
}



int mea_call_python_function_from_module(PyObject *module, char *plugin_func, PyObject *plugin_params_dict)
{
   int retour=0;
   PyErr_Clear();

   if(!module) {
      VERBOSE(5) mea_log_printf("%s (%s) : not found\n", ERROR_STR, __func__);
      retour=-1;
   }
   else {
      PyObject *pFunc = PyObject_GetAttrString(module, plugin_func);
      if(pFunc) {
         retour=mea_call_python_function2(pFunc, plugin_params_dict);

         Py_XDECREF(pFunc);
      }
      else {
         VERBOSE(5) mea_log_printf("%s (%s) : %s not fount\n", ERROR_STR, __func__, plugin_func);
         retour=-1;
      }
   }

   PyErr_Clear();

   return retour;
}


int mea_call_python_function(char *plugin_name, char *plugin_func, PyObject *plugin_params_dict)
{
   PyObject *pName = NULL, *pModule = NULL;
   int retour=-1;

   pName = PyString_FromString(plugin_name);
   if(!pName) {
      return -1;
   }

   pModule = PyImport_Import(pName);
/*
   if(!pModule) {
      VERBOSE(5) mea_log_printf("%s (%s) : %s not found\n", ERROR_STR, __func__, plugin_name);
   }
   else {
      pFunc = PyObject_GetAttrString(pModule, plugin_func);
      if(pFunc) {
         retour=mea_call_python_function2(pFunc, plugin_params_dict);
      }
      else {
         VERBOSE(5) mea_log_printf("%s (%s) : %s not fount in %s module\n", ERROR_STR, __func__, plugin_func, plugin_name);
      }
      Py_XDECREF(pFunc);
   }
*/
   if(pModule) {
      retour=mea_call_python_function_from_module(pModule, plugin_func, plugin_params_dict);

      Py_XDECREF(pModule);
   }

   Py_DECREF(pName);

   return retour;
}


PyObject *mea_jsonObjectToPyObject(cJSON *e)
{
   PyObject *p = NULL;
   
   switch(e->type) {
      case cJSON_False:
         p=Py_False;
         Py_INCREF(p);
         break;

      case cJSON_True:
         p=Py_True;
         Py_INCREF(p);
         break;
            
      case cJSON_NULL:
         p=Py_None;
         Py_INCREF(p);
         break;
               
      case cJSON_Number:
         p = PyFloat_FromDouble((double)e->valuedouble);
         break;

      case cJSON_String:
         p = PyString_FromString(e->valuestring);
         break;
               
      case cJSON_Array:
         break;
               
      case cJSON_Object:
         p=mea_jsonToPyDict(e->child);
         break;
      
      default:
         return NULL;
   }
   
   return p;
}


PyObject *mea_jsonArrayToPyList(cJSON *j)
{
   if(j->type!=cJSON_Array)
      return NULL;
      
   if(!j->child)
      return NULL;
      
   PyObject *p = PyList_New(0);
   
   cJSON *e=j->child;
   while(e) {
      PyObject *s=mea_jsonObjectToPyObject(e);
      PyList_Append(p,s);
      Py_DECREF(s);
      e=e->next;
   }

   return p;
}


PyObject *mea_jsonToPyDict(cJSON *j)
{
   if(!j) {
      PyErr_SetString(PyExc_RuntimeError, "ERROR (mea_jsonToPyDict) : PyDict_New error");
      return NULL;
   }
   
//   cJSON *e=j->child;
   if(!j->child) {
      return mea_jsonObjectToPyObject(j);
   }
   else {
      PyObject *p = PyDict_New();
      cJSON *e=j->child;
      while(e) {
         PyObject *s = NULL;
         if(e->type!=cJSON_Array) {
            s=mea_jsonObjectToPyObject(e);
         }
         else {
            s=mea_jsonArrayToPyList(e);
         }
         PyDict_SetItemString(p, e->string, s);
         Py_DECREF(s);
         e=e->next;
      }
      return p;
   }
}


PyObject *mea_xplMsgToPyDict2(cJSON *xplMsgJson)
{
   if(xplMsgJson == NULL)
      return NULL;

   PyObject *pyXplMsg = NULL;
   PyObject *s = NULL;
   PyObject *l = NULL;
//   char tmpStr[35]=""; // chaine temporaire. Taille max pour vendorID(8) + "-"(1) + deviceID(8) + "."(1) + instanceID(16)
//   cJSON *j = NULL;
   
   pyXplMsg = PyDict_New();
   PyObject *pyBody=PyDict_New();

   if(!pyXplMsg || !pyBody) {
      PyErr_SetString(PyExc_RuntimeError, "ERROR (mea_xplMSgToPyDict) : PyDict_New error");
      return NULL;
   }

   l = PyLong_FromLong(1L);
   PyDict_SetItemString(pyXplMsg, XPLMSG_STR_C, l);
   Py_DECREF(l);
 
   cJSON *e=xplMsgJson->child; 
   while(e) {
      if(e->string) {
         if(strcmp(e->string, XPLSOURCE_STR_C)==0 ||
            strcmp(e->string, XPLTARGET_STR_C)==0 ||
            strcmp(e->string, XPLMSGTYPE_STR_C)==0 ||
            strcmp(e->string, XPLSCHEMA_STR_C)==0) {
            s=PyString_FromString(e->valuestring);
            PyDict_SetItemString(pyXplMsg, e->string, s);
            Py_DECREF(s);
         }
         else {
            if (e->valuestring != NULL)
               s=PyString_FromString(e->valuestring);
            else {
               s=Py_None;
               Py_INCREF(s);
            }
            PyDict_SetItemString(pyBody, e->string, s);
            Py_DECREF(s);
         }
      }
      e=e->next;
   }
   
   PyDict_SetItemString(pyXplMsg, "body", pyBody);
   Py_DECREF(pyBody);
   
   return pyXplMsg;
}
