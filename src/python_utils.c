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

/*
static void mea_addLong_to_pydict(PyObject *data_dict, char *key, long value)
{
   PyObject * val = PyLong_FromLong((long)value);
   PyDict_SetItemString(data_dict, key, val);
   Py_DECREF(val);
}


static void mea_addDouble_to_pydict(PyObject *data_dict, char *key, double value)
{
   PyObject * val = PyFloat_FromDouble((double)value);
   PyDict_SetItemString(data_dict, key, val);
   Py_DECREF(val);
}


static void mea_addString_to_pydict(PyObject *data_dict, char *key, char *value)
{
   PyObject *val = PYSTRING_FROMSTRING(value);
   PyDict_SetItemString(data_dict, key, val);
   Py_DECREF(val);
}


static void mea_addpydict_to_pydict(PyObject *data_dict, char *key, PyObject *adict)
{
   PyDict_SetItemString(data_dict, key, adict);
   Py_DECREF(adict);
}
*/
/*
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
*/

static PyObject *mea_jsonObjectToPyDict(cJSON *j)
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


static PyObject *mea_jsonArrayToPyList(cJSON *j)
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
//         p = PyBytes_FromStringAndSize(e->valuestring, (long)e->valueint);
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


static cJSON *mea_PyTupleToJsonArray(PyObject *p)
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


static cJSON *mea_PyListToJsonArray(PyObject *p)
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


static cJSON *mea_PyObjectToJsonObject(PyObject *p)
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
   else if(PyByteArray_Check(p) || PyBytes_Check(p)) {
      int l_bytes=(int)PyByteArray_Size(p);
      j=cJSON_CreateByteArray(PyByteArray_AsString(p), l_bytes);
   }

   return j;
}

/*
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
*/

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
