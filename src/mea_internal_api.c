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

#include "python_utils.h"
#include "interfacesServer.h"

#include "mea_internal_api.h"


PyObject *mea_memory=NULL;
PyObject *mea_module=NULL;


static PyObject *mea_api_getMemory(PyObject *self, PyObject *args);
static PyObject *mea_sendAtCmdAndWaitResp(PyObject *self, PyObject *args);
static PyObject *mea_sendAtCmd(PyObject *self, PyObject *args);
//static PyObject *mea_sendEnoceanPacketAndWaitResp(PyObject *self, PyObject *args);
//static PyObject *mea_enoceanCRC(PyObject *self, PyObject *args);
//static PyObject *mea_sendEnoceanRadioErp1Packet(PyObject *self, PyObject *args);
static PyObject *mea_xplGetVendorID(PyObject *self, PyObject *args);
static PyObject *mea_xplGetDeviceID(PyObject *self, PyObject *args);
static PyObject *mea_xplGetInstanceID(PyObject *self, PyObject *args);
static PyObject *mea_xplSendMsg2(PyObject *self, PyObject *args);
static PyObject *mea_addDataToSensorsValuesTable(PyObject *self, PyObject *args);
static PyObject *mea_interface_api(PyObject *self, PyObject *args);
static PyObject *mea_interface_api_json(PyObject *self, PyObject *args);


static PyMethodDef MeaMethods[] = {
   {"getMemory",                    mea_api_getMemory,                METH_VARARGS, "Return a dictionary"},
   {"sendXbeeCmdAndWaitResp",       mea_sendAtCmdAndWaitResp,         METH_VARARGS, "Envoie d'une commande AT et recupere la reponse"},
   {"sendXbeeCmd",                  mea_sendAtCmd,                    METH_VARARGS, "Envoie d'une commande AT sans attendre de reponse"},
//   {"sendEnoceanRadioErp1Packet",   mea_sendEnoceanRadioErp1Packet,   METH_VARARGS, "Envoie un message ERP1"},
   {"xplGetVendorID",               mea_xplGetVendorID,               METH_VARARGS, "VendorID"},
   {"xplGetDeviceID",               mea_xplGetDeviceID,               METH_VARARGS, "DeviceID"},
   {"xplGetInstanceID",             mea_xplGetInstanceID,             METH_VARARGS, "InstanceID"},
   {"xplSendMsg",                   mea_xplSendMsg2,                  METH_VARARGS, "Envoie un message XPL"},
   {"addDataToSensorsValuesTable",  mea_addDataToSensorsValuesTable,  METH_VARARGS, "Envoi des donnees dans la table sensors_values"},
//   {"sendSerialData",               mea_write,                        METH_VARARGS, "Envoi des donnees vers une ligne serie"},
//   {"receiveSerialData",            mea_read,                         METH_VARARGS, "recupere des donnees depuis une ligne serie"},
   {"interfaceAPI",                 mea_interface_api_json,                METH_VARARGS, "appel api d'interface"},
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
// /!\ a ecrire pour librérer tous le contenu de la memoire partagé ...
   if(mea_memory)
      Py_DECREF(mea_memory);
}


static PyObject *mea_api_getMemory(PyObject *self, PyObject *args)
{
   return mea_getMemory(self, args, mea_memory);
}


static uint32_t _indianConvertion(uint32_t val_x86)
{
   uint32_t val_xbee;
   char *val_x86_ptr;
   char *val_xbee_ptr;
   
   val_x86_ptr = (char *)&val_x86;
   val_xbee_ptr = (char *)&val_xbee;
   
   // conversion little vers big indian
   for(int16_t i=0,j=3;i<sizeof(uint32_t);i++)
      val_xbee_ptr[i]=val_x86_ptr[j-i];

   return val_xbee;
}


static PyObject *mea_interface_api(PyObject *self, PyObject *args)
{
   PyObject *arg;

//   int id_driver=-1;
   int id_interface=-1;
//   char *data=NULL;
//   int l_data=0;
   int nb_args=0;
   // récupération des paramètres et contrôle des types

   nb_args=(int)PyTuple_Size(args);
   if(nb_args<2)
      goto mea_interface_api_arg_err;
/*
   arg=PyTuple_GetItem(args, 0);
   if(PyNumber_Check(arg))
      id_driver=(int)PyLong_AsLong(arg);
   else
      goto mea_interface_api_arg_err;
*/
   arg=PyTuple_GetItem(args, 0);
   if(PyNumber_Check(arg))
      id_interface=(int)PyLong_AsLong(arg);
   else
      goto mea_interface_api_arg_err;

   char *cmnd = NULL;
   arg=PyTuple_GetItem(args, 1);
   if(PYSTRING_CHECK(arg))
      cmnd=(char *)PYSTRING_ASSTRING(arg);
   else
      goto mea_interface_api_arg_err;

   char err[255];
   int16_t nerr;
   PyObject *res = NULL;

   int ret=interfacesServer_call_interface_api(id_interface, cmnd, (void *)args, nb_args - 2, (void **)&res, &nerr, err, sizeof(err));
   switch(ret) {
      case -252:
      case -253:
      case -254:
         PyErr_SetString(PyExc_RuntimeError, err);
         return NULL;
      case -255:
         PyErr_BadArgument();
         return NULL;
   }
 
   PyObject *t=PyTuple_New(3);
   PyTuple_SetItem(t, 0, PyLong_FromLong(ret));
   PyTuple_SetItem(t, 1, PyLong_FromLong((long)nerr));
   if(res == NULL)
      res = PyLong_FromLong(0L);
   PyTuple_SetItem(t, 2, res);

   return t;

mea_interface_api_arg_err:
   PyErr_BadArgument();
   return NULL;
}


static PyObject *mea_interface_api_json(PyObject *self, PyObject *args)
{
   PyObject *arg;

   int id_interface=-1;
   int nb_args=0;
   // récupération des paramètres et contrôle des types

   nb_args=(int)PyTuple_Size(args);
   if(nb_args<2)
      goto mea_interface_api_arg_err;

   arg=PyTuple_GetItem(args, 0);
   if(PyNumber_Check(arg))
      id_interface=(int)PyLong_AsLong(arg);
   else
      goto mea_interface_api_arg_err;

   char *cmnd = NULL;
   arg=PyTuple_GetItem(args, 1);
   if(PYSTRING_CHECK(arg))
      cmnd=(char *)PYSTRING_ASSTRING(arg);
   else
      goto mea_interface_api_arg_err;

   char err[255];
   int16_t nerr;
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
   if(res == NULL)
      res = PyLong_FromLong(0L);

   PyObject *t=PyTuple_New(3);
   PyTuple_SetItem(t, 0, PyLong_FromLong(ret));
   PyTuple_SetItem(t, 1, PyLong_FromLong((long)nerr));
   PyTuple_SetItem(t, 2, res);

   return t;

mea_interface_api_arg_err:
   PyErr_BadArgument();
   return NULL;
}


/*
static PyObject *mea_enoceanCRC(PyObject *self, PyObject *args)
{
   PyObject *arg;

   unsigned long *prev;
   unsigned long *c;
   
   if(PyTuple_Size(args)!=2)
      goto mea_enoceanCRC_arg_err;

   arg=PyTuple_GetItem(args, 0);
   if(PyLong_Check(arg))
      prev=(unsigned long *)PyLong_AsVoidPtr(arg);
   else
      goto mea_enoceanCRC_arg_err;
      
   arg=PyTuple_GetItem(args, 1);
   if(PyLong_Check(arg))
      c=(unsigned long *)PyLong_AsVoidPtr(arg);
   else
      goto mea_enoceanCRC_arg_err;
      
   uint8_t _prev=(uint8_t)(*prev & 0xFF);
   uint8_t _c=(uint8_t)(*c & 0xFF);
   uint8_t _crc = proc_crc8(_prev,_c);
   
   return PyLong_FromLong((unsigned long)_crc);
   
mea_enoceanCRC_arg_err:
   PyErr_BadArgument();
   return NULL;
}


// sendEnoceanRadioErp1Packet(enocean_ed, rorg, sub_id, dest_addr, buffer);
static PyObject *mea_sendEnoceanRadioErp1Packet(PyObject *self, PyObject *args)
{
   PyObject *arg;
   enocean_ed_t *ed;
   int16_t nerr;
   int16_t ret;

   // récupération des paramètres et contrôle des types
   if(PyTuple_Size(args)!=5)
      goto mea_sendEnoceanRadioErp1Packet_arg_err;

   // enocean_ed
   arg=PyTuple_GetItem(args, 0);
   if(PyLong_Check(arg))
      ed=(enocean_ed_t *)PyLong_AsLong(arg);
   else
      goto mea_sendEnoceanRadioErp1Packet_arg_err;

   // rorg
   uint32_t rorg;
   arg=PyTuple_GetItem(args, 1);
   if(PyNumber_Check(arg))
      rorg=(uint32_t)PyLong_AsLong(arg);
   else
      goto mea_sendEnoceanRadioErp1Packet_arg_err;

   // sub_id
   uint32_t sub_id;
   arg=PyTuple_GetItem(args, 2);
   if(PyNumber_Check(arg))
      sub_id=(uint32_t)PyLong_AsLong(arg);
   else
      goto mea_sendEnoceanRadioErp1Packet_arg_err;

   // dest addr
   uint32_t dest_addr;
   arg=PyTuple_GetItem(args, 3);
   if(PyNumber_Check(arg))
      dest_addr=(uint32_t)PyLong_AsLong(arg);
   else
      goto mea_sendEnoceanRadioErp1Packet_arg_err;

   Py_buffer py_packet;
   arg=PyTuple_GetItem(args, 4);
   if(PyObject_CheckBuffer(arg)) {
      ret=PyObject_GetBuffer(arg, &py_packet, PyBUF_SIMPLE);
      if(ret<0)
         goto mea_sendEnoceanRadioErp1Packet_arg_err;
   }
   else
      goto  mea_sendEnoceanRadioErp1Packet_arg_err;

   // enocean_send_radio_erp1_packet(enocean_ed_t *ed, uint8_t rorg, uint32_t source, uint32_t sub_id, uint32_t dest, uint8_t *data, uint16_t l_data, uint8_t status, int16_t *nerr)
   ret = enocean_send_radio_erp1_packet(ed, rorg, ed->id, sub_id, dest_addr, py_packet.buf, py_packet.len, 0, &nerr);

   // réponse
   PyObject *t=PyTuple_New(2);
   PyTuple_SetItem(t, 0, PyLong_FromLong(ret));
   PyTuple_SetItem(t, 1, PyLong_FromLong(nerr));

   return t;

mea_sendEnoceanRadioErp1Packet_arg_err:
   PyErr_BadArgument();
   return NULL;
}


static PyObject *mea_sendEnoceanPacketAndWaitResp(PyObject *self, PyObject *args)
{
   PyObject *arg;
   enocean_ed_t *ed;
   int16_t nerr;
   int16_t ret;
   
   // récupération des paramètres et contrôle des types
   if(PyTuple_Size(args)!=3)
      goto mea_sendEnoceanPacketAndWaitResp_arg_err;

   // enocean_ed
   arg=PyTuple_GetItem(args, 0);
   if(PyLong_Check(arg))
      ed=(enocean_ed_t *)PyLong_AsLong(arg);
   else
      goto mea_sendEnoceanPacketAndWaitResp_arg_err;

   // packet
   arg=PyTuple_GetItem(args, 1);
   Py_buffer py_packet;
   if(PyObject_CheckBuffer(arg)) {
      ret=PyObject_GetBuffer(arg, &py_packet, PyBUF_SIMPLE);
      if(ret<0)
         goto mea_sendEnoceanPacketAndWaitResp_arg_err;
   }
   else
      goto mea_sendEnoceanPacketAndWaitResp_arg_err;

   // response
   arg=PyTuple_GetItem(args, 2);
   Py_buffer py_response;
   if(PyObject_CheckBuffer(arg)) {
      ret=PyObject_GetBuffer(arg, &py_response, PyBUF_SIMPLE | PyBUF_WRITABLE);
      if(ret<0)
         goto mea_sendEnoceanPacketAndWaitResp_arg_err;
   }
   else
      goto mea_sendEnoceanPacketAndWaitResp_arg_err;

   uint16_t l_response=py_response.len;
   ret=enocean_send_packet(ed,
                           py_packet.buf,
                           py_packet.len,
                           py_response.buf,
                           &l_response,
                           &nerr);

   // réponse
   PyObject *t=PyTuple_New(3);
   PyTuple_SetItem(t, 0, PyLong_FromLong(ret));
   PyTuple_SetItem(t, 1, PyLong_FromLong(nerr));
   PyTuple_SetItem(t, 2, PyLong_FromLong(l_response));
   return t;
   
mea_sendEnoceanPacketAndWaitResp_arg_err:
   PyErr_BadArgument();
   return NULL;
}
*/


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


static PyObject *mea_xplSendMsg2(PyObject *self, PyObject *args)
{
   PyObject *PyXplMsg;
   PyObject *item;
   PyObject *body;
            
//   char vendor_id[40]="";
//   char device_id[40]="";
//   char instance_id[40]="";
   
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
//   char str[256];
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
//         char *skey=PyString_AS_STRING(key);
//         char *svalue=PyString_AS_STRING(value);
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
   if(xplMsgJson)
      cJSON_Delete(xplMsgJson); 
   
   return NULL; // retour un exception python
}


static PyObject *mea_sendAtCmdAndWaitResp(PyObject *self, PyObject *args)
{
   unsigned char at_cmd[81];
   uint16_t l_at_cmd;
   
   unsigned char resp[81];
   uint16_t l_resp;
   
   int16_t ret;
   PyObject *arg;

   xbee_host_t *host=NULL;

   // récupération des paramètres et contrôle des types
   if(PyTuple_Size(args)!=5)
      goto mea_AtCmdToXbee_arg_err;

   xbee_xd_t *xd;
   arg=PyTuple_GetItem(args, 0);
   if(PyLong_Check(arg))
      xd=(xbee_xd_t *)PyLong_AsLong(arg);
   else
      goto mea_AtCmdToXbee_arg_err;
   
   uint32_t addr_h;
   arg=PyTuple_GetItem(args, 1);
   if(PyNumber_Check(arg))
      addr_h=(uint32_t)PyLong_AsLong(arg);
   else
      goto mea_AtCmdToXbee_arg_err;
   
   uint32_t addr_l;
   arg=PyTuple_GetItem(args, 2);
   if(PyNumber_Check(arg))
      addr_l=(uint32_t)PyLong_AsLong(arg);
   else
      goto mea_AtCmdToXbee_arg_err;
   
   char *at;
   arg=PyTuple_GetItem(args, 3);
   if(PYSTRING_CHECK(arg))
      at=(char *)PYSTRING_ASSTRING(arg);
   else
      goto mea_AtCmdToXbee_arg_err;

   at_cmd[0]=at[0];
   at_cmd[1]=at[1];
   
   arg=PyTuple_GetItem(args, 4);
   if(PyNumber_Check(arg)) {
      uint32_t val=(uint32_t)PyLong_AsLong(arg);
      uint32_t val_xbee=_indianConvertion(val);
      char *val_xbee_ptr=(char *)&val_xbee;
      
      for(int16_t i=0;i<sizeof(uint32_t);i++)
         at_cmd[2+i]=val_xbee_ptr[i];
      l_at_cmd=6;
   }
   else if (PYSTRING_CHECK(arg)) {
      char *at_arg=(char *)PYSTRING_ASSTRING(arg);
      uint16_t i;
      for(i=0;i<strlen(at_arg);i++)
         at_cmd[2+i]=at_arg[i];
      if(i>0)
         l_at_cmd=2+i;
      else
         l_at_cmd=2;
   }
   else
      goto mea_AtCmdToXbee_arg_err;

   // recuperer le host dans la table (necessite d'avoir accès à xd
   int16_t err;
   
   host=NULL;
   host=(xbee_host_t *)malloc(sizeof(xbee_host_t)); // description de l'xbee directement connecté
   xbee_get_host_by_addr_64(xd, host, addr_h, addr_l, &err);
   if(err==XBEE_ERR_NOERR) {
   }
   else {
      DEBUG_SECTION mea_log_printf("%s (%s) : host not found.\n", DEBUG_STR ,__func__);
      if(host) {
         free(host);
         host=NULL;
      }
      PyErr_BadArgument(); // à remplacer
      return NULL;
   }
   
   int16_t nerr;
   ret=xbee_atCmdSendAndWaitResp(xd, host, at_cmd, l_at_cmd, resp, &l_resp, &nerr);
   if(ret==-1) {
      DEBUG_SECTION mea_log_printf("%s (%s) : error %d.\n", DEBUG_STR, __func__, nerr);
      if(host) {
         free(host);
         host=NULL;
      }
      PyErr_BadArgument(); // à remplacer
      return NULL;
   }
   
   struct xbee_remote_cmd_response_s *mapped_resp=(struct xbee_remote_cmd_response_s *)resp;
   
   void *data;
   long l_data;

   PyObject *t=PyTuple_New(3);
#if PY_MAJOR_VERSION >= 3
   Py_buffer view;
   PyObject *py_cmd=PyBytes_FromStringAndSize(resp, l_resp);
   if(!py_cmd) {
      PyTuple_SetItem(t, 0, PyLong_FromLong(mapped_resp->cmd_status));
      PyTuple_SetItem(t, 1, py_cmd);
      PyTuple_SetItem(t, 2, PyLong_FromLong(l_resp));
   }
#else
   PyObject *py_cmd=PyBuffer_New(l_resp);
   if (!PyObject_AsWriteBuffer(py_cmd, &data, (Py_ssize_t *)&l_data)) {
      memcpy(data,resp,l_data);
      PyTuple_SetItem(t, 0, PyLong_FromLong(mapped_resp->cmd_status));
      PyTuple_SetItem(t, 1, py_cmd);
      PyTuple_SetItem(t, 2, PyLong_FromLong(l_resp));
   }
   else {
      Py_DECREF(py_cmd);
      Py_DECREF(t);
   }
#endif   

   free(host);
   host=NULL;

   return t; // return True
   
mea_AtCmdToXbee_arg_err:
   if(host) {
      free(host);
      host=NULL;
   }
   
   PyErr_BadArgument();
   return NULL;
}


static PyObject *mea_sendAtCmd(PyObject *self, PyObject *args)
{
   unsigned char at_cmd[81];
   uint16_t l_at_cmd;
   
   PyObject *arg;
   
   xbee_host_t *host=NULL;
   
   // récupération des paramètres et contrôle des types
   if(PyTuple_Size(args)!=5)
      goto mea_atCmdSend_arg_err;
   
   xbee_xd_t *xd;
   arg=PyTuple_GetItem(args, 0);
   if(PyLong_Check(arg))
      xd=(xbee_xd_t *)PyLong_AsLong(arg);
   else
      goto mea_atCmdSend_arg_err;
   
   uint32_t addr_h;
   arg=PyTuple_GetItem(args, 1);
   if(PyNumber_Check(arg))
      addr_h=(uint32_t)PyLong_AsLong(arg);
   else
      goto mea_atCmdSend_arg_err;
   
   uint32_t addr_l;
   arg=PyTuple_GetItem(args, 2);
   if(PyNumber_Check(arg))
      addr_l=(uint32_t)PyLong_AsLong(arg);
   else
      goto mea_atCmdSend_arg_err;
   
   char *at;
   arg=PyTuple_GetItem(args, 3);
   if(PYSTRING_CHECK(arg))
      at=(char *)PYSTRING_ASSTRING(arg);
   else
      goto mea_atCmdSend_arg_err;
   
   at_cmd[0]=at[0];
   at_cmd[1]=at[1];
   
   arg=PyTuple_GetItem(args, 4);
   if(PyNumber_Check(arg)) {
      uint32_t val=(uint32_t)PyLong_AsLong(arg);
      uint32_t val_xbee=_indianConvertion(val);
      char *val_xbee_ptr=(char *)&val_xbee;
      
      for(int16_t i=0;i<sizeof(uint32_t);i++)
         at_cmd[2+i]=val_xbee_ptr[i];
      l_at_cmd=6;
   }
   else if (PYSTRING_CHECK(arg)) {
      char *at_arg=(char *)PYSTRING_ASSTRING(arg);
      uint16_t i;
      for(i=0;i<strlen(at_arg);i++)
         at_cmd[2+i]=at_arg[i];
      if(i>0)
         l_at_cmd=2+i;
      else
         l_at_cmd=2;
   }
   else
      goto mea_atCmdSend_arg_err;
   
   int16_t err;
   
   if(addr_l != 0xFFFFFFFF && addr_h != 0xFFFFFFFF) {
      host=(xbee_host_t *)malloc(sizeof(xbee_host_t)); // description de l'xbee directement connecté
      xbee_get_host_by_addr_64(xd, host, addr_h, addr_l, &err);
      if(err!=XBEE_ERR_NOERR) {
         DEBUG_SECTION mea_log_printf("%s (%s) : host not found.\n", DEBUG_STR,__func__);
         goto mea_atCmdSend_arg_err;
      }
   }
   else
      host=NULL;
   
   int16_t nerr;
   // ret=
   xbee_atCmdSend(xd, host, at_cmd, l_at_cmd, &nerr);
   
   if(host) {
      free(host);
      host=NULL;
   }
   return PyLong_FromLong(1L); // return True

mea_atCmdSend_arg_err:
   DEBUG_SECTION mea_log_printf("%s (%s) : arguments error\n", DEBUG_STR,__func__);
   PyErr_BadArgument();
   if(host) {
      free(host);
      host=NULL;
   }
   return NULL;
}


static PyObject *mea_addDataToSensorsValuesTable(PyObject *self, PyObject *args)
{
   return PyLong_FromLong(1L);
}