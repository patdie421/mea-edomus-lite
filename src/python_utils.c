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

#include "pythonPluginServer.h"
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
   PyObject *val = PYSTRING_FROMSTRING(value);
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
         retour=(int)PYINT_ASLONG(pValue);
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

//   pName = PYSTRING_FROMSTRING(plugin_name);
#if PY_MAJOR_VERSION >= 3
   pName = PyUnicode_DecodeFSDefault(plugin_name);
#else
   pName = PYSTRING_FROMSTRING(plugin_name);
#endif
   if(!pName) {
      return -1;
   }

   pModule = PyImport_Import(pName);

   if(pModule) {
      retour=mea_call_python_function_from_module(pModule, plugin_func, plugin_params_dict);

      Py_XDECREF(pModule);
   }

   Py_DECREF(pName);

   return retour;
}


cJSON *mea_call_python_function_json_alloc(char *module_name, char *function_name, cJSON *j) /* to test */
{
   cJSON *r=NULL;
   PyObject *p=NULL;
   PyObject *pModuleName=NULL;

   if(j==NULL) {
      p=Py_None;
      Py_INCREF(p);
   }
   else {
      p=mea_jsonToPyObject(j);
      if(!p) {
         return NULL;
      }
   }
   
//   PyObject *pModuleName = PYSTRING_FROMSTRING(module_name);
#if PY_MAJOR_VERSION >= 3
   pModuleName = PyUnicode_DecodeFSDefault(module_name);
#else
   pModuleName = PYSTRING_FROMSTRING(module_name);
#endif

   if(!pModuleName) {
      if(p) {
         Py_DECREF(p);
         p=NULL;
      }
      return NULL;
   }
   PyObject *pModule = PyImport_Import(pModuleName);
   Py_DECREF(pModuleName);
   pModuleName=NULL;
   
   if(pModule) {
      PyObject *pFunction = PyObject_GetAttrString(pModule, function_name);

      if (pFunction && PyCallable_Check(pFunction)) {
         PyObject *pFunctionArg = PyTuple_New(1); 
         PyTuple_SetItem(pFunctionArg, 0, p); // PyTuple_SetItem va voler la référence sur p pas besoin de DEREF
         p=NULL;
         PyObject *pResult = PyObject_CallObject(pFunction, pFunctionArg);
         if (pResult != NULL) {
            r=mea_PyObjectToJson(pResult);
            Py_DECREF(pResult);
            pResult=NULL;
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

         if(pFunctionArg) {
            Py_DECREF(pFunctionArg);
            pFunctionArg=NULL;
         }
      }
      else {
         VERBOSE(5) {
            mea_log_printf("%s (%s) : function \"\" not exist or not callable in \"\"", ERROR_STR, __func__, function_name, module_name);
         }
      }

      if(pFunction) {
         Py_DECREF(pFunction);
         pFunction=NULL;
      }
   }
   else {
      VERBOSE(5) {
         mea_log_printf("%s (%s) : can't load \"\"", ERROR_STR, __func__, module_name);
      }
   }
   
   if(pModule) {
      Py_DECREF(pModule);
      pModule=NULL;
   }

   if(p) {
      Py_DECREF(p);
      p=NULL;
   }
   
   return r;
}


PyObject *mea_jsonObjectToPyDict(cJSON *j)
{
   PyObject *p = PyDict_New();

   cJSON *e=j;
   while(e) {
      PyObject *s = NULL;
      s=mea_jsonToPyObject(e);
      if(s) {
         PyDict_SetItemString(p, e->string, s);
         Py_DECREF(s);
      }
      e=e->next;
   }
   return p;
}


PyObject *mea_jsonArrayToPyList(cJSON *j)
{
   PyObject *p = PyList_New(0);
   
   cJSON *e=j;
   while(e) {
      PyObject *s=mea_jsonToPyObject(e);
      if(s) {
         PyList_Append(p,s);
         Py_DECREF(s);
      }
      e=e->next;
   }
   return p;
}


PyObject *mea_jsonToPyObject(cJSON *e)
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
         p = PYSTRING_FROMSTRING(e->valuestring);
         break;
         
      case cJSON_ByteArray:
         p = PyByteArray_FromStringAndSize(e->valuestring, (long)e->valueint);
         break;
               
      case cJSON_Array:
         p = mea_jsonArrayToPyList(e->child);
         break;

      case cJSON_Object:
         p=mea_jsonObjectToPyDict(e->child);
         break;
      
      default:
         return NULL;
   }
   
   return p;
}


cJSON *mea_PyTupleToJsonArray(PyObject *p)
{
   cJSON *j = NULL;
   
   j=cJSON_CreateArray();
   
   int l=(int)PyTuple_Size(p);
   for(int i=0;i<l;i++) {
      cJSON *v=mea_PyObjectToJson(PyTuple_GetItem(p,i));
      if(v)
         cJSON_AddItemToArray(j, v);
   }
      
   return j;
}


cJSON *mea_PyListToJsonArray(PyObject *p)
{
   cJSON *j = NULL;
   
   j=cJSON_CreateArray();
   
   int l=(int)PyList_Size(p);
   for(int i=0;i<l;i++) {
      cJSON *v=mea_PyObjectToJson(PyList_GetItem(p,i));
      if(v)
         cJSON_AddItemToArray(j, v);
   }
      
   return j;
}


cJSON *mea_PyObjectToJsonObject(PyObject *p)
{
   cJSON *j = NULL;
   
   j=cJSON_CreateObject();
   
   PyObject *key, *value;
   Py_ssize_t pos = 0;

   while (PyDict_Next(p, &pos, &key, &value)) {
      cJSON *v=mea_PyObjectToJson(value);
      if(v)
         cJSON_AddItemToObject(j, PYSTRING_ASSTRING(key), v);
   }
   
   return j;
}


cJSON *mea_PyObjectToJson(PyObject *p)
{
   cJSON *j=NULL;

   if(p==Py_None) {
      j=cJSON_CreateNull();
   }
   else if(PyBool_Check(p)) {
      if(p == Py_True) {
         j=cJSON_CreateTrue();
      }
      else {
         j=cJSON_CreateFalse();
      }
   }
   else if(PyFloat_Check(p)) {
      j=cJSON_CreateNumber(PyFloat_AsDouble(p));
   }
   else if(PyLong_Check(p)) {
      j=cJSON_CreateNumber(PyLong_AsDouble(p));
   }
   else if(PYSTRING_CHECK(p)) {
      j=cJSON_CreateString(PYSTRING_ASSTRING(p));
   }
   else if(PyDict_Check(p)) {
      j=mea_PyObjectToJsonObject(p);
   }
   else if(PyTuple_Check(p)) {
      j=mea_PyTupleToJsonArray(p);
   }
   else if(PyList_Check(p)) {
      j=mea_PyListToJsonArray(p);
   }
   else if(PyByteArray_Check(p)) {
      int l_bytes=(int)PyByteArray_Size(p);
      j=cJSON_CreateByteArray(PyByteArray_AsString(p), l_bytes);
   }

   return j;
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
         if(strcmp(e->string, XPLSOURCE_STR_C)==0  ||
            strcmp(e->string, XPLTARGET_STR_C)==0  ||
            strcmp(e->string, XPLMSGTYPE_STR_C)==0 ||
            strcmp(e->string, XPLSCHEMA_STR_C)==0) {
            s=PYSTRING_FROMSTRING(e->valuestring);
            PyDict_SetItemString(pyXplMsg, e->string, s);
            Py_DECREF(s);
         }
         else {
            if (e->valuestring != NULL)
               s=PYSTRING_FROMSTRING(e->valuestring);
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


int python_cmd_json(char *module, int type, cJSON *data)
{
   plugin_queue_elem_t *plugin_elem = (plugin_queue_elem_t *)malloc(sizeof(plugin_queue_elem_t));
   if(plugin_elem) {
      plugin_elem->type_elem=type;
      plugin_elem->aJsonDict = data;
      plugin_elem->aDict = NULL;
      pythonPluginServer_add_cmd(module, (void *)plugin_elem, sizeof(plugin_queue_elem_t));
      free(plugin_elem);
      plugin_elem=NULL;
   }
   return 0;
}


cJSON *python_call_function_json_alloc(char *module, char *function, cJSON *data)
{
   cJSON *result=NULL;
   
   plugin_queue_elem_t *plugin_elem = (plugin_queue_elem_t *)malloc(sizeof(plugin_queue_elem_t));
   if(plugin_elem) {
      plugin_elem->type_elem=CUSTOM_JSON;
      plugin_elem->aJsonDict=data;
      plugin_elem->aDict=NULL;
      result=pythonPluginServer_exec_cmd(module, function, (void *)plugin_elem, sizeof(plugin_queue_elem_t), 0);
      free(plugin_elem);
      plugin_elem=NULL;
   }
   
   return result;
}


void json_test()
{
   mea_log_printf("JSON_TEST_START\n");
   
   char *json="{ \"string\": \"string\", \"int\":10, \"float\":20.1234, \"boolean\": true, \"array\": [ 1, 2, 3, { \"object1\": \"object1\", \"object2\": \"object2\" } ] }";
   
   cJSON *j=cJSON_Parse(json);
   cJSON *r=mea_call_python_function_json_alloc("json_test", "json_test", j);
   char *s=cJSON_Print(r);
   
   mea_log_printf("json_test result: %s\n", s);
   
   cJSON_Delete(r);
   cJSON_Delete(j);
   free(s);
   
   mea_log_printf("JSON_TEST_END\n");
}
