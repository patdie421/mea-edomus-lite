# coding: utf8

import re
import string
import sys
import subprocess
import json

try:
   import mea
except:
   import mea_simulation as mea

from sets import Set

import mea_utils
from mea_utils import verbose


def mea_dataPreprocessor(data):
   fn_name= __name__ + "/" + str(sys._getframe().f_code.co_name)
   verbose(9, "DEBUG (", fn_name, ") data = ", data)
 
   return data['data']
 
##
# mea_xplCmndMsg : traitement d'un message xPL transmis par mea-edomus
#
def mea_xplCmndMsg(data):

   fn_name= __name__ + "/" + str(sys._getframe().f_code.co_name)

   verbose(9, "DEBUG (", fn_name, ") data = ", data)

   try:
      id_device=data["device_id"]
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

   mem_device=mea.getMemory(id_device)
   mem_interface=mea.getMemory("interface"+str(interface_id))
   x=data["xplmsg"]
   body=x["body"]

   target="*"
   if "source" in x:
      target=x["source"]

   if x["msgtype"]!='xpl-cmnd':
      return False

   if x["schema"]=="sensor.request":
      if "request" in body:
         if body["request"]=="current":
            mea.interfaceAPI(api_key, "mea_writeData", "[[["+device_name+"?current]]]\n")
            return True
      return False

   elif x["schema"]=="control.basic":
      if typeoftype!=1:
         verbose(2, "ERROR (", fn_name, ") - device ("+devicename+") is not an output")
         return False
      if not "current" in body:
         verbose(2, "ERROR (", fn_name, ") - no current in body")
         return False
      v=body["current"].lower()
      try:
         f=float(v)
      except:
         if v in ["on", "off", "high", "low", "true", "false" ]:
            f=v
         else:
            verbose(2, "ERROR (", fn_name, ") - invalid current ("+v+") value")
            return False
      mea.interfaceAPI(api_key, "mea_writeData", "[[["+device_name+"="+str(f)+"]]]\n")
      return True
   return False


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
      v = pdata[value].lower()
      f = 0
      if type(v) == str:
         try:
            f=float(v)
         except:
            if v in ["on", "off", "high", "low", "true", "false" ]:
               f=v
            else:
               return False
         xplMsg=mea_utils.xplMsgNew("me", "*", "xpl-trig", "sensor", "basic")
         mea_utils.xplMsgAddValue(xplMsg, "device", device_name)
         mea_utils.xplMsgAddValue(xplMsg, "current", str(f))
         mea.xplSendMsg(xplMsg)

         mea.interfaceAPI(api_key, "mea_writeData", "[[["+device_name+"!OK]]]\n")
 
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


def is_number(s):
    """ Returns True is string is a number. """
    try:
        float(s)
        return True
    except ValueError:
        return False


if __name__ == "__main__":
   import re
   import random

   FIFO_IN  = '/tmp/meapipe-out'
   FIFO_OUT = '/tmp/meapipe-in'

   actuators=["I010A001"]
   sensors=["I010S001", "I010S002", "o02a"]
   values=["false", "true", "off", "on", "low", "high"]
   results=["OK", "KO"]
   actions=["current"]
   stats={}
   stats["I010A001"]="false"

   for i in range(len(actuators)):
      actuators[i]=actuators[i].upper()

   for i in range(len(sensors)):
      sensors[i]=sensors[i].upper()

   for i in range(len(values)):
      values[i]=values[i].upper()

   for i in range(len(results)):
      results[i]=results[i].upper()

   for i in range(len(actions)):
      actions[i]=actions[i].upper()

   with open(FIFO_IN) as fifoin:
      while True:
         data = fifoin.readline()
         if len(data) == 0:
            break
         data=data.strip()
         print "IN:", data
         _data=data.upper()

         if _data[:3]=="[[[" and _data[-3:]=="]]]":
            cmnd=re.split("([\?=\!])", _data[3:-3])
            if not cmnd[0] in sensors and not cmnd[0] in actuators:
               continue
            if not cmnd[1] in [ '?', '=', '!' ]:
               continue
#            if not cmnd[2] in values and not cmnd[2] in results and not cmnd[2] in actions and not is_number(cmnd[2]):
#               continue

         if cmnd[1]=='=' and (cmnd[2] in values or is_number(cmnd[2])):
            stats[cmnd[0]]=str(cmnd[2])
            fifoout=open(FIFO_OUT, "w")
            print "OUT:", "{{{"+cmnd[0]+"="+stats[cmnd[0]]+"}}}"
            fifoout.write("{{{"+data[3:-3]+"}}}")
            fifoout.close()

         if cmnd[1]=='?':
            if cmnd[2]=='CURRENT':
               try:
                  if cmnd[0] in actuators:
                     print "OUT:", "{{{"+cmnd[0]+"="+stats[cmnd[0]]+"}}}"
                     fifoout=open(FIFO_OUT, "w")
                     fifoout.write("{{{"+cmnd[0]+"="+stats[cmnd[0]]+"}}}")
                     fifoout.close()
                  elif cmnd[0] in sensors:
                     v=random.uniform(0, 20)
                     print "OUT:", "{{{"+cmnd[0]+"="+str(v)+"}}}"
                     fifoout=open(FIFO_OUT, "w")
                     fifoout.write("{{{"+cmnd[0]+"="+str(v)+"}}}")
                     fifoout.close()
               except:
                  pass
