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
      parameters=data["device_parameters"]
      parameters=parameters.strip().lower()
      params=mea_utils.parseKeyValueDatasToDictionary(parameters, ",", ":")
      typeoftype=data["typeoftype"]

   except:
      verbose(2, "ERROR (", fn_name, ") - parameters error")
      return False

   mem_actuator=mea.getMemory(id_actuator)
   mem_interface=mea.getMemory("interface"+str(interface_id))

   # validation du parametre
   pin=0
   if "pin" in params:
      pin=params["pin"]
   else:
      verbose(9, "DEBUG (", fn_name, ") - parameters error : no pin")
      return False
      
   if len(pin) != 2:
      verbose(2, "ERROR (", fn_name, ") - PIN parameter error") 
      return False

   if pin[0]!='l' and pin[0]!='a':
      verbose(2, "ERROR (", fn_name, ") - PIN type error") 
      return False
   else:
      pinType=pin[0]

   if pin[1]<'0' or pin[1]>'9':
      verbose(2, "ERROR (", fn_name, ") - PIN id error") 
      return False
   else:
      pinNum=int(pin[1])

      
   # validation du message xpl
   try: 
      x=data["xplmsg"]
      schema=x["schema"]
      msgtype=x["msgtype"]
      body=x["body"]
      device=body["device"]
   except:
      verbose(2, "ERROR (", fn_name, ") - no or incomplet xpl message error") 
      return False

   if msgtype!="xpl-cmnd":
      verbose(9, data)
      return False

   current=False
   request=False
   if "current" in body:
      current=body["current"]
   elif "request" in body:
      request=body["request"]
   else:
      verbose(2, "ERROR (", fn_name, ") - no \"current\" or \"request\" in message body") 
      return False

   # préparation de la commande
   _cmnd=str(pinNum)+","

   if schema=="control.basic" and typeoftype==1:
      if current==False:
         verbose(2, "ERROR (", fn_name, ") - no \"current\" in message body") 
         return False
      try:
         type=body["type"]
      except:
         verbose(2, "ERROR (", fn_name, ") - no \"type\" in message body") 
         return False

      if (pinNum==0 or pinNum==3):
         verbose(2, "ERROR (", fn_name, ") - PIN 0 and 3 are input only") 
         return False

      if type=="output" and pinType=='l':
         if current=="pulse":
            if "data1" in body:
               data1=body["data1"]
               if not data1.isdigit():
                  verbose(2, "ERROR (", fn_name, ") - data1 not numeric value")
                  return False
               data1=int(data1)
            else:
               data1=200

            _cmnd=_cmnd+"P,"+str(data1)
         elif current=="high" or (current.isdigit() and int(current)==1):
            _cmnd=_cmnd+"L,H"
         elif current=="low" or (current.isdigit() and int(current)==0):
            _cmnd=_cmnd+"L,L"

      elif type=="variable" and pinType=='a':
         if pinNum != 1 and pinNum != 2:
            verbose(2, "ERROR (", fn_name, ") - Analog output(PWM) only available on pin 1 and 2")
            return False
         else:
            value=0
            if 'current' in mem_interface[pinNum]:
               value=mem_interface[pinNum]['current']

            if current=="dec":
               value=value-1
            elif current=="inc":
               value=value+1
            elif current[0]=='-' or current[0]=='+':
               if current[1:].isdigit():
                  value=int(current)
               else:
                  verbose(2, "ERROR (", fn_name, ") - analog value error (",current,")")
                  return False
            elif current.isdigit():
               value=int(current)
            else:
               verbose(2, "ERROR (", fn_name, ") - analog value error (",current,")")
               return False

            if value<0:
               value=0;
            if value>255:
               value=255;
            _cmnd=_cmnd+"A,"+str(value)
      else:
         verbose(2, "ERROR (", fn_name, ") - xpl command error")
         return False

   elif schema=="sensor.basic":
      if request==False:
         verbose(2, "ERROR (", fn_name, ") - no \"request\" in message body") 
         return False
      if request!="current":
         verbose(2, "ERROR (", fn_name, ") - xpl error request!=current")
         return False
      if typeoftype==0:
         if pinType=='a' and  pinNum < 4:
            verbose(2, "ERROR (", fn_name, ") - Analog input only available on pin 4 to pin 9")
            return False
         _cmnd=_cmnd+pinType.upper()+",G"
      else:
         _cmnd=False
         # interrogation de l'état d'une sortie, on récupére de l'info aux niveaux de l'interface si dispo
         if not "current" in  mem_interface[pinNum]:
            return False
         current_value=mem_interface[pinNum]["current"]
         if pinType=='l':
            if current_value==0:
               current_value="low"
            else:
               current_value="high"
         xplMsg=mea_utils.xplMsgNew("me", "*", "xpl-trig", "sensor", "basic")
         mea_utils.xplMsgAddValue(xplMsg, "device", device_name)
         mea_utils.xplMsgAddValue(xplMsg, "current", current_value)
         mea.xplSendMsg(xplMsg)
         return True
   else:
      verbose(9, data)
      verbose(2, "ERROR (", fn_name, ") - xpl schema incompatible with sensor/actuator (", schema, ")")
      return False

   if _cmnd != False:
      cmnd="~~~~CMD:##"+_cmnd+"##\r\nEND\r\n"
#      verbose(9, cmnd)
      mea.sendSerialData(data['fd'],bytearray(cmnd))

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
      parameters=data["device_parameters"]
      parameters=parameters.strip().lower()
      params=mea_utils.parseKeyValueDatasToDictionary(parameters, ",", ":")
      typeoftype=data["typeoftype"]
   except:
      verbose(2, "ERROR (", fn_name, ") - invalid data")
      return False

   mem_device=mea.getMemory(device_id)
   mem_interface=mea.getMemory("interface"+str(interface_id))

   # validation du parametre
   pin=0
   try:
      pin=params["pin"]
   except:
      verbose(2, "ERROR (", fn_name, ") - ", device_name, " parameters error : no PIN defined")
      return False
      
   if len(pin) != 2:
      verbose(2, "ERROR (", fn_name, ") - ", device_name, " PIN format error") 
      return False

   if pin[0]!='l' and pin[0]!='a':
      verbose(2, "ERROR (", fn_name, ") - ", device_name, " PIN type error") 
      return Flase
   else:
      pinType=pin[0]

   if pin[1]<'0' or pin[1]>'9':
      verbose(2, "ERROR (", fn_name, ") - ", device_name, " PIN number error") 
      return False
   else:
      pinNum=int(pin[1])

   # on va traiter les données prémachées par mea_serialDataPre
   if "sendFlag"in mem_interface[pinNum] and mem_interface[pinNum]["sendFlag"] == True:
      if "current" in  mem_interface[pinNum]:
         current_value=mem_interface[pinNum]["current"];
         if pinType=='l':
            if mem_interface[pinNum]["current"]==0:
               current_value="low"
            else:
               current_value="high"

         mea.addDataToSensorsValuesTable(device_id, mem_interface[pinNum]["current"],0,0,"")

         xplMsg=mea_utils.xplMsgNew("me", "*", "xpl-trig", "sensor", "basic")
         mea_utils.xplMsgAddValue(xplMsg, "device", device_name)
         mea_utils.xplMsgAddValue(xplMsg, "current", current_value)
         mea.xplSendMsg(xplMsg)
      return True
   return False


##
# mea_serialPre : pré-traitement (au niveau de l'interface) de données en provenance d'une ligne serie
#
def mea_serialDataPre(data):
   fn_name= __name__ + "/" + str(sys._getframe().f_code.co_name)

   verbose(9, data)
   
   try:
       serial_data=data["data"]
       interface_id=data["interface_id"]
   except:
      verbose(2, "ERROR (", fn_name, ") - invalid data")
      return False

   mem_interface=mea.getMemory("interface"+str(interface_id))

   try:
      s=serial_data.decode('latin-1')
   except:
      return False

   for i in range(0, 10):
      mem_interface[i]["sendFlag"]=False

   retour=False
   if s.find(u"CMT: ") >= 0: # un sms dans le buffer ?
      retour=True # oui, dans tous les cas on passera le buffer on passera le buffer

   if s.find("\r\n> ") >=0:
      retour=True # oui, dans tous les cas on passera le buffer on passera le buffer

   while len(s)>0:
      ptrStart=s.find(u"$$")
      if ptrStart<0:
         break
      ptrStart=ptrStart+2
      ptrEnd=s[ptrStart:].find(u"$$")
      if ptrEnd<0:
         break
      info=s[ptrStart:ptrStart+ptrEnd]
     
      # niveau du signal radio
      if info.find(u"SIG=") == 0:
         sig = info[4:]
         try:
            sig=float(sig)
            verbose(9, "INFO  SIM900 SIGNAL LEVEL=", sig)
            mem_interface['siglvl']=sig
         except:
            pass
      else:
         # retour d'I/O
         a=info.split(";")
         for e in a:
            if len(e)<=0:
               continue               
            e=e.lower()
            if e[0]!='l' and e[0]!='p' and e[0] != 'a':
               continue 
            type=e[0]
            if e[1]!="/":
               continue 
            if e[2]<'0' or e[2]>'9':
               continue
            pinNum=int(e[2])
            if e[3]!='=':
               continue
            value=e[4:]

            if value=='h' or value=='l' or value.isdigit():
               if value=='h':
                  value=1
                  _value='high'
               elif value=='l':
                  value=0
                  _value='low'
               else:
                  _value=value
                  value=int(value)

               mem_interface[pinNum]["current"]=int(value)
               mem_interface[pinNum]["sendFlag"]=True
               retour=True;

            else:
               continue

      # on recommence avec la suite de la chaine
      s=s[ptrStart+ptrEnd+2:]  

   return retour

 
##
# mea_init : initialisation du plugin
#
def mea_init(data):
   fn_name=sys._getframe().f_code.co_name

   verbose(9,"INFO  (", fn_name, ") - lancement mea_init v2")

   try:
      interface_id=data["interface_id"]
   except:
      verbose(2, "ERROR (", fn_name, ") - invalid data")
      return False

   mem_interface=mea.getMemory("interface"+str(interface_id))
   for i in range(0, 10):
      mem_interface[i]={}

   return True
