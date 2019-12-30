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
#include "mea_python_json_utils.h"


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
         if((double)((long)e->valuedouble)==e->valuedouble) {
            p = PyLong_FromLong((long)e->valuedouble);
         }
         else {
            p = PyFloat_FromDouble((double)e->valuedouble);
         }
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
      if(v) {
         cJSON_AddItemToArray(j, v);
      }
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
      if(v) {
         cJSON_AddItemToArray(j, v);
      }
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
      if(v) {
         cJSON_AddItemToObject(j, PYSTRING_ASSTRING(key), v);
      }
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
   else if(PyInt_Check(p)) {
      j=cJSON_CreateNumber((double)PyInt_AsLong(p));
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
*/
