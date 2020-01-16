//
//  mea_api.c
//
//  Created by Patrice Dietsch on 09/05/13.
//
//
#ifdef __APPLE__
#include <Python/Python.h>
#else
#include <Python.h>
#endif
#include <stdio.h>

#include "globals.h"
#include "xPLServer.h"
#include "tokens.h"
#include "tokens_da.h"
#include "xbee.h"
#include "enocean.h"
#include "mea_verbose.h"

#include "mea_python_json_utils.h"
#include "interfacesServer.h"

#include "mea_internal_api.h"


PyObject *mea_memory=NULL;
PyObject *mea_module=NULL;


static PyObject *mea_api_getMemory(PyObject *self, PyObject *args);
static PyObject *mea_xplGetVendorID(PyObject *self, PyObject *args);
static PyObject *mea_xplGetDeviceID(PyObject *self, PyObject *args);
static PyObject *mea_xplGetInstanceID(PyObject *self, PyObject *args);
static PyObject *mea_xplSendMsg(PyObject *self, PyObject *args);
static PyObject *mea_interface_api(PyObject *self, PyObject *args);
static PyObject *mea_get_interface(PyObject *self, PyObject *args);


static PyMethodDef MeaMethods[] = {
   {"getMemory",        mea_api_getMemory,    METH_VARARGS, "Return a dictionary"},
   {"xplGetVendorID",   mea_xplGetVendorID,   METH_VARARGS, "VendorID"},
   {"xplGetDeviceID",   mea_xplGetDeviceID,   METH_VARARGS, "DeviceID"},
   {"xplGetInstanceID", mea_xplGetInstanceID, METH_VARARGS, "InstanceID"},
   {"xplSendMsg",       mea_xplSendMsg,       METH_VARARGS, "send xPL message"},
   {"interfaceAPI",     mea_interface_api,    METH_VARARGS, "call interface api"},
   {"getInterface",     mea_get_interface,    METH_VARARGS, "get full interface description"},
   {NULL, NULL, 0, NULL}
};


void mea_api_init()
{
   mea_memory=PyDict_New(); // initialisation de la mémoire

#if PY_MAJOR_VERSION >= 3
   static struct PyModuleDef moduledef = {
      PyModuleDef_HEAD_INIT,
      "mea",               /* m_name */
      "mea",               /* m_doc */
      -1,                  /* m_size */
      MeaMethods      ,    /* m_methods */
      NULL,                /* m_reload */
      NULL,                /* m_traverse */
      NULL,                /* m_clear */
      NULL,                /* m_free */
   };
   mea_module = PyModule_Create(&moduledef);
#else
   mea_module=Py_InitModule("mea", MeaMethods);  
#endif
}


void mea_api_release()
{
// /!\ a ecrire pour librérer tous le contenu de la mémoire partagé ...
   if(mea_memory) {
      Py_DECREF(mea_memory);
   }
}


PyObject *mea_getMemory(PyObject *self, PyObject *args, PyObject *mea_memory)
{
   PyObject *key=NULL;
   PyObject *mem=NULL;
   
   if(PyTuple_Size(args)!=1) {
      DEBUG_SECTION mea_log_printf("%s (%s) :  arguments error.\n", DEBUG_STR ,__func__);
      PyErr_BadArgument();
      return NULL;
   }
   
   key=PyTuple_GetItem(args, 0);
   if(!key) {
      DEBUG_SECTION mea_log_printf("%s (%s) :  bad mem id.\n", DEBUG_STR ,__func__);
      PyErr_BadArgument();
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


static PyObject *mea_api_getMemory(PyObject *self, PyObject *args)
{
   return mea_getMemory(self, args, mea_memory);
}


static PyObject *mea_get_interface(PyObject *self, PyObject *args)
{
   PyObject *arg=NULL;

   int id_interface=-1;
   int nb_args=0;

   nb_args=(int)PyTuple_Size(args);
   if(nb_args!=1) {
      PyErr_BadArgument();
      return NULL;
   }

   arg=PyTuple_GetItem(args, 0);
   if(PyNumber_Check(arg)) {
      id_interface=(int)PyLong_AsLong(arg);
   }
   else {
      PyErr_BadArgument();
      return NULL;
   }

   cJSON *interfaceJson=getInterfaceById_alloc(id_interface);
   PyObject *interfacePython=NULL;
   if(interfaceJson!=NULL) {
      interfacePython=mea_jsonToPyObject(interfaceJson);
   }
   else {
      interfacePython=Py_None;
      Py_INCREF(interfacePython);
   }
   cJSON_Delete(interfaceJson);

   return interfacePython;
}


static PyObject *mea_interface_api(PyObject *self, PyObject *args)
{
   PyObject *arg=NULL;

   int id_interface=-1;
   int nb_args=0;
   
   // récupération des paramètres et contrôle des types
   nb_args=(int)PyTuple_Size(args);
   if(nb_args<2) {
      goto mea_interface_api_arg_err;
   }

   arg=PyTuple_GetItem(args, 0);
   if(PyNumber_Check(arg)) {
      id_interface=(int)PyLong_AsLong(arg);
   }
   else {
      goto mea_interface_api_arg_err;
   }

   char *cmnd = NULL;
   arg=PyTuple_GetItem(args, 1);
   if(PYSTRING_CHECK(arg)) {
      cmnd=(char *)PYSTRING_ASSTRING(arg);
   }
   else {
      goto mea_interface_api_arg_err;
   }

   char err[255]="";
   int16_t nerr=0;
   cJSON *_res = NULL;
   cJSON *_args = mea_PyObjectToJson(args);

   int ret=interfacesServer_call_interface_api(id_interface, cmnd, (void *)_args, nb_args - 2, (void **)&_res, &nerr, err, sizeof(err));

   cJSON_Delete(_args);
   
   switch(ret) {
      case -252:
      case -253:
      case -254:
         if(_res) {
            cJSON_Delete(_res);
            _res=NULL;
         }
         PyErr_SetString(PyExc_RuntimeError, err);
         return NULL;
      case -255:
         if(_res) {
            cJSON_Delete(_res);
            _res=NULL;
         }
         PyErr_BadArgument();
         return NULL;
   }

   PyObject *res=NULL;
   if(_res) {
      res=mea_jsonToPyObject(_res);
      cJSON_Delete(_res);
   }
   if(res == NULL) {
      res = PyLong_FromLong(0L);
   }

   PyObject *t=PyTuple_New(3);
   PyTuple_SetItem(t, 0, PyLong_FromLong(ret));
   PyTuple_SetItem(t, 1, PyLong_FromLong((long)nerr));
   PyTuple_SetItem(t, 2, res);

   return t;

mea_interface_api_arg_err:
   PyErr_BadArgument();
   return NULL;
}


static PyObject *mea_xplGetVendorID(PyObject *self, PyObject *args)
{
   return PYSTRING_FROMSTRING(mea_getXPLVendorID());
}


static PyObject *mea_xplGetDeviceID(PyObject *self, PyObject *args)
{
   return  PYSTRING_FROMSTRING(mea_getXPLDeviceID());
}


static PyObject *mea_xplGetInstanceID(PyObject *self, PyObject *args)
{
   return PYSTRING_FROMSTRING(mea_getXPLInstanceID());
}


static PyObject *mea_xplSendMsg(PyObject *self, PyObject *args)
{
   PyObject *PyXplMsg;
   PyObject *item;
   PyObject *body;
            
   if(PyTuple_Size(args)!=1) {
      PyErr_SetString(PyExc_RuntimeError, "ERROR (mea_xplMsgSend) : need one argument");
      return NULL;
   }
   
   // recuperation du parametre
   PyXplMsg=PyTuple_GetItem(args, 0);
   
   if(!PyDict_Check(PyXplMsg)) { // le parametre doit etre de type “dictionnaire”
      PyErr_SetString(PyExc_RuntimeError, "ERROR (mea_xplMsgSend) : argument not a dictionary");
      return NULL;
   }
   
   // on peut commencer à recuperer les elements necessaires
   // il nous faut :
   // - le type de message xpl (xpl-cmnd, xpl-trig ou xpl-stat)
   // - le schema
   // - les donnees
   cJSON *xplMsgJson = cJSON_CreateObject();
 
   // recuperation du type de message
   item = PyDict_GetItemString(PyXplMsg, "msgtype");
   if(!item) {
       PyErr_SetString(PyExc_RuntimeError, "ERROR (mea_xplMsgSend) : xpl message no type");
       goto mea_xplSendMsg_clean_exit;
   }
   char *p=PYSTRING_ASSTRING(item);
   if(p) {
       cJSON_AddItemToObject(xplMsgJson, XPLMSGTYPE_STR_C, cJSON_CreateString(p));
   }

   item = PyDict_GetItemString(PyXplMsg, XPLSOURCE_STR_C);
   if(!item) {
       PyErr_SetString(PyExc_RuntimeError, "ERROR (mea_xplMsgSend) : xpl message no source");
       goto mea_xplSendMsg_clean_exit;
   }
   p=PYSTRING_ASSTRING(item);
   if(p) {
       cJSON_AddItemToObject(xplMsgJson, XPLSOURCE_STR_C, cJSON_CreateString(p));
   }

   // recuperation de la class et du type du message XPL
   item = PyDict_GetItemString(PyXplMsg, XPLSCHEMA_STR_C);
   if(!item) {
      PyErr_SetString(PyExc_RuntimeError, "ERROR (mea_xplMsgSend) : xpl schema not found");
      goto mea_xplSendMsg_clean_exit;
   }
   p=PYSTRING_ASSTRING(item);
   if(p) {
      cJSON_AddItemToObject(xplMsgJson, XPLSCHEMA_STR_C, cJSON_CreateString(p));
   }

   item = PyDict_GetItemString(PyXplMsg, XPLTARGET_STR_C);
   if(!item) {
   }
   p=PYSTRING_ASSTRING(item);
   if(p) {
      cJSON_AddItemToObject(xplMsgJson, XPLTARGET_STR_C, cJSON_CreateString(p));
   }
 
   body = PyDict_GetItemString(PyXplMsg, "xpl-body");
   if(body) {
      if(!PyDict_Check(body)) {
         PyErr_SetString(PyExc_RuntimeError, "ERROR (mea_xplMsgSend) : body not a dictionary.\n");
         goto mea_xplSendMsg_clean_exit;
      }
      
      // parcours de la liste
      PyObject *key, *value;
      Py_ssize_t pos = 0;
      while (PyDict_Next(body, &pos, &key, &value)) {
         char *skey=PYSTRING_ASSTRING(key);
         char *svalue=PYSTRING_ASSTRING(value);
         
         if(!skey || !svalue) {
            PyErr_SetString(PyExc_RuntimeError, "ERROR (mea_xplMsgSend) : incorrect data in body.");
            goto mea_xplSendMsg_clean_exit;
         }

         cJSON_AddItemToObject(xplMsgJson, skey, cJSON_CreateString(svalue));
      }
   }
   else {
      PyErr_SetString(PyExc_RuntimeError, "ERROR (mea_xplMsgSend) : xpl body data not found.");
      goto mea_xplSendMsg_clean_exit;
   }

   mea_sendXPLMessage2(xplMsgJson);

   cJSON_Delete(xplMsgJson);
   
   return PyLong_FromLong(1L); // return True
   
mea_xplSendMsg_clean_exit:
   if(xplMsgJson) {
      cJSON_Delete(xplMsgJson);
   }
   
   return NULL; // retour un exception python
}
