# coding: utf8

import re
import string
import sys

try:
   import mea
except:
   import mea_simulation as mea

from sets import Set

import mea_utils
from mea_utils import verbose


def clean_devices(api_key):
   return True


def update_devices(api_key, devices):
   clean_devices(api_key)
   return init_devices(api_key, devices)


def init_devices(api_key, devices):

   fn_name= __name__ + "/" + str(sys._getframe().f_code.co_name)

   for i in devices:
      verbose(2, "ERROR (", fn_name, ") - ", device["name"])

   return True


##
# mea_xplCmndMsg : traitement d'un message xPL transmis par mea-edomus
#
def mea_xplCmndMsg(data):

   fn_name= __name__ + "/" + str(sys._getframe().f_code.co_name)

   try:
      id_actuator=data["device_id"]
   except:
      verbose(2, "ERROR (", fn_name, ") - device_id not found")
      return False

   try:
      device_name=data["device_name"]
      interface_id=data["interface_id"]
      typeoftype=data["typeoftype"]
      api_key=data["api_key"]
      parameters=data["device_parameters"]
      parameters=parameters.strip().lower()
      params=mea_utils.parseKeyValueDatasToDictionary(parameters, ",", ":")

   except:
      verbose(2, "ERROR (", fn_name, ") - parameters error")
      return False

   mem_actuator=mea.getMemory(id_actuator)
   mem_interface=mea.getMemory("interface"+str(interface_id))

   return True


##
# mea_serialData : traitement de données en provenance d'une ligne serie
#
#def mea_serialData(data):
def mea_dataFromSensor(data):
   fn_name= __name__ + "/" + str(sys._getframe().f_code.co_name)

#   verbose(9, "DEBUG (", fn_name, ") data = ", data)

   try:
      device_id=data["device_id"]
   except:
      verbose(2, "ERROR (", fn_name, ") - device_id not found")
      return False

   try:
      device_name=data["device_name"]
      interface_id=data["interface_id"]
      api_key=data["api_key"]
      typeoftype=data["typeoftype"]
      parameters=data["device_parameters"]
      parameters=parameters.strip().lower()
      params=mea_utils.parseKeyValueDatasToDictionary(parameters, ",", ":")
   except:
      verbose(2, "ERROR (", fn_name, ") - invalid data")
      return False

   mem_device=mea.getMemory(device_id)
   mem_interface=mea.getMemory("interface"+str(interface_id))

   else:
      pinNum=int(pin[1])

   return True


##
# mea_updateDevices : la configuration a changé, faire ce qu'il faut ...
#
def mea_updateDevices(data):
   fn_name= __name__ + "/" + str(sys._getframe().f_code.co_name)

   verbose(9, data)
   try:
      interface_id=data["interface_id"]
      api_key=data["api_key"]
   except:
      verbose(2, "ERROR (", fn_name, ") - invalid data")
      return False

   mem_interface=mea.getMemory("interface"+str(interface_id))

   retour=True

   return retour


##
# mea_serialPre : pré-traitement (au niveau de l'interface) de données en provenance d'une ligne serie
#
def mea_dataPreprocessor(data):
   fn_name= __name__ + "/" + str(sys._getframe().f_code.co_name)

   verbose(9, data)
   try:
      serial_data=data["data"]
      interface_id=data["interface_id"]
      api_key=data["api_key"]
   except:
      verbose(2, "ERROR (", fn_name, ") - invalid data")
      return False

   mem_interface=mea.getMemory("interface"+str(interface_id))

   retour=True

   return retour

 
##
# mea_init : initialisation du plugin
#
def mea_init(data):
   fn_name=sys._getframe().f_code.co_name

   verbose(9,"INFO  (", fn_name, ") - lancement mea_init")

   try:
      interface_id=data["interface_id"]
      api_key=data["api_key"]
   except:
      verbose(2, "ERROR (", fn_name, ") - invalid data")
      return False

   mem_interface=mea.getMemory("interface"+str(interface_id))

   return init_devices(api_key, mea_getInterface(interface_id))
