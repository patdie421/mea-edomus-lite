import re
import string
import sys

try:
    import mea
except:
    import mea_simulation as mea

import mea_utils
from mea_utils import verbose

from sets import Set
import string

debug=0

# Exemple "Trame F6"
# 00 0x55  85 Synchro
# 01 0x00   0 Header1 Data Lenght
# 02 0x07   7 Header2 Data Lenght
# 03 0x07   7 Header3 Optionnal lenght
# 04 0x01   1 Header4 Packet type
# 05 0x7A 122 CRC8H
# 06 0xF6 246 Data1 RORG
# 07 0x30  48 Data2 = B00110000
# 08 0x00   0 Data3 ID1
# 09 0x25  37 Data4 ID2
# 10 0x86 134 Data5 ID3
# 11 0x71 113 Data6 ID4
# 12 0x30  48 Data7 Status
# 13 0x02   2 Optionnal1 Number sub telegramme
# 14 0xFF 255 Optionnal2 Destination ID
# 15 0xFF 255 Optionnal3 Destination ID
# 16 0xFF 255 Optionnal4 Destination ID
# 17 0xFF 255 Optionnal5 Destination ID
# 18 0x2D  45 Optionnal6 Dbm
# 19 0x00   0 Optionnal7 Security level
# 20 0x38  56 CRC8D

def mea_xplCmndMsg(data):
   fn_name=sys._getframe().f_code.co_name
   try:
      id_sensor=data["device_id"]
   except:
      verbose(2, "ERROR (", fn_name, ") - device_id not found")
      return 0

   mem=mea.getMemory(id_sensor)

   x=data["xplmsg"]
   body=x["body"]

   target="*"
   if "source" in x:
      target=x["source"]

   if x["schema"]=="sensor.request":
      try:
         xplMsg=mea_utils.xplMsgNew("me", target, "xpl-stat", "sensor", "basic")
         mea_utils.xplMsgAddValue(xplMsg,"device", data["device_name"].lower())
         if body["request"]=="current":
            mea_utils.xplMsgAddValue(xplMsg,"current",mem["current"])
         elif body["request"]=="last":
            mea_utils.xplMsgAddValue(xplMsg,"last",mem["last"])
         else:
            verbose(2, "ERROR (", fn_name, ") - invalid request (", body["request"],")")
            return False
         mea_utils.xplMsgAddValue(xplMsg,"type","input")
      except:
         verbose(2, "ERROR (", fn_name, ") - can't create xpl message")
         return False

      mea.xplSendMsg(xplMsg)
      return True

   return False


def mea_dataFromSensor(data):
   fn_name=sys._getframe().f_code.co_name

   try:
      id_sensor=data["device_id"]
      verbose(1, "DEBUG (", fn_name, ") - id_sensor=", id_sensor)
      packet=data["data"]
      l_packet=data["l_data"]
      parameters=data["device_parameters"]
   except:
      verbose(2, "ERROR (", fn_name, ") - invalid data")
      return False

   mem=mea.getMemory(id_sensor)
   paramsDict=mea_utils.parseKeyValueDatasToDictionary(parameters, ",", ":")

   device1=0
   device2=0
   try:
      device=paramsDict["xpldevice"];
   except:
      device=-1;

   if packet[4]==1: # ENOCEAN_RADIO_ERP1
      if packet[6]==0xF6:
         t21 = (packet[12] & 0b00100000) >> 5
         nu =  (packet[12] & 0b00010000) >> 4
         
         if t21 == 1:
            if nu == 1:
               button1Num = (packet[7] & 0b11100000) >> 5
               button2Num = -1;
               action   = (packet[7] & 0b00010000) >> 4
               if packet[7] & 0b00000001:
                  button2Num=(packet[7] & 0b00001110) >> 1
               current=""
               try:
                  if paramsDict["channel"]=="A" and action == 1:
                     if (button1Num == 0 or button2Num == 0):
                        current="high"
                     elif (button1Num == 1 or button2Num == 1):
                        current="low"

                  if paramsDict["channel"]=="B" and action == 1:
                     if (button1Num == 2 or button2Num == 2):
                        current="high"
                     elif (button1Num == 3 or button2Num == 3):
                        current="low"
               except:
                  return False

               if current != "":
                  try:
                     mem["last"]=mem["current"]
                  except:
                     mem["last"]=False
                  mem["current"]=current

                  mem["button1"]=button1Num
                  mem["button2"]=button2Num

                  xplMsg=mea_utils.xplMsgNew("me", "*", "xpl-trig", "sensor", "basic")
                  mea_utils.xplMsgAddValue(xplMsg,"device", data["device_name"].lower())
                  mea_utils.xplMsgAddValue(xplMsg,"current", mem["current"])
                  mea_utils.xplMsgAddValue(xplMsg,"type", "input")
                  mea_utils.xplMsgAddValue(xplMsg,"last",mem["last"])
                  mea.xplSendMsg(xplMsg)
                  return True

#            else: # bouton relache
#               if mem["button1"] != -1:
#                  verbose(2, "Relachement de : ", mem["button1"])
#               if mem["button2"] != -1:
#                  verbose(2, "Relachement de : ", mem["button2"])
#               mem["button1"]=-1
#               mem["button2"]=-1
#               verbose(2,"Nb boutons appuyes = ", (packet[7] & 0b11100000) >> 5)
#               verbose(2,"Energy bow = ", (packet[7] & 0b00010000) >> 4)
#               return True
   return False
