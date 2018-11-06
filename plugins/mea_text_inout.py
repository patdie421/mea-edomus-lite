# coding: utf8

import re
import string
import sys
import subprocess

try:
   import mea
except:
   import mea_simulation as mea

from sets import Set

import mea_utils
from mea_utils import verbose


def mea_dataPreprocessor(data):
   fn_name= __name__ + "/" + str(sys._getframe().f_code.co_name)
#   verbose(9, "DEBUG (", fn_name, ") data = ", data)
 
   return data['data']
 
##
# mea_xplCmndMsg : traitement d'un message xPL transmis par mea-edomus
#
def mea_xplCmndMsg(data):

   fn_name= __name__ + "/" + str(sys._getframe().f_code.co_name)

   verbose(9, "DEBUG (", fn_name, ") data = ", data)

   try:
      id_actuator=data["device_id"]
   except:
      verbose(2, "ERROR (", fn_name, ") - device_id not found")
      return False

   try:
      device_name=data["device_name"]
      interface_id=data["interface_id"]
      parameters=data["device_parameters"]
      parameters=parameters.strip().lower()
      params=mea_utils.parseKeyValueDatasToDictionary(parameters, ",", ":")
      typeoftype=data["typeoftype"]
      api_key=data["api_key"]

   except:
      verbose(2, "ERROR (", fn_name, ") - parameters error")
      return False

   mem_actuator=mea.getMemory(id_actuator)
   mem_interface=mea.getMemory("interface"+str(interface_id))

   x=data["xplmsg"]
   body=x["body"]

   target="*"
   if "source" in x:
      target=x["source"]

   if x["schema"]=="control.basic":
      try:
         cmnd=params["cmnd"];
      except:
         verbose(2, "ERROR (", fn_name, ") - no cmnd")
         return False

      _cmnd=[cmnd]
      p = subprocess.Popen(_cmnd)


def mea_dataFromSensor(data):
   fn_name= __name__ + "/" + str(sys._getframe().f_code.co_name)

   try:
      device_id=data["device_id"]
   except:
      verbose(2, "ERROR (", fn_name, ") - device_id not found")
      return False

   try:
      device_name=data["device_name"]
      interface_id=data["interface_id"]
      parameters=data["device_parameters"]
      parameters=parameters.strip().lower()
      params=mea_utils.parseKeyValueDatasToDictionary(parameters, ",", ":")
      typeoftype=data["typeoftype"]
      api_key=data["api_key"]

   except:
      verbose(2, "ERROR (", fn_name, ") - invalid data")
      return False

   if not "value" in params:
      verbose(2, "ERROR (", fn_name, ") - no value parameter")
      return False

   value = params["value"]
 
   mem_device=mea.getMemory(device_id)
   mem_interface=mea.getMemory("interface"+str(interface_id))

   pdata = mea_utils.parseKeyValueDatasToDictionary(str(data['data']).strip().lower(),";","=")

   if value in pdata:
      v = pdata[value]
      f = 0.0
      if type(v) == str:
         try:
            f=float(v)
         except:
            return False

         mem_device["current"]=f

         mea.addDataToSensorsValuesTable(device_id, f, 0,0,"")

         xplMsg=mea_utils.xplMsgNew("me", "*", "xpl-trig", "sensor", "basic")
         mea_utils.xplMsgAddValue(xplMsg, "device", device_name)
         mea_utils.xplMsgAddValue(xplMsg, "current", str(f))
         mea.xplSendMsg(xplMsg)

#         mea.interfaceAPI(api_key, "mea_writeData", "{{{OK}}}")
 
      else:
         return -1


def mea_init(data):
   fn_name=sys._getframe().f_code.co_name

   verbose(9,"INFO  (", fn_name, ") - lancement mea_init")

   try:
      interface_id=data["interface_id"]
   except:
      verbose(2, "ERROR (", fn_name, ") - invalid data")
      return False

   mem_interface=mea.getMemory("interface_"+str(interface_id))

   return True
