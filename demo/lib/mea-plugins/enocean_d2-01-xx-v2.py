import re
import string
import sys
import json

try:
    import mea
except:
    import mea_simulation as mea

import mea_utils
from mea_utils import verbose

from sets import Set
import string

debug=0

# Exemple "Trame D2 0x04"
# 00 0x55  85 Synchro
# 01 0x00   0 Header1 Data Lenght
# 02 0x09   9 Header2 Data Lenght
# 03 0x07   7 Header3 Optionnal lenght
# 04 0x01   1 Header4 Packet type
# 05 0x56  86 CRC8H
# 06 0xd2 210 RORG : D2
# [
# 07 0x04   4 Power Failure(B7)/Power Detection(B6)/Command(B3-B0) : 4 = Actuator status
# 08 0x60  96 Over Current Switch off (B7)/Error lvl(B6-B5)/I/O Channel(B4-B0) : 0 = channel
# 09 0xe4 228 Local control(B7)/Output Value(B6-B0) : 1101100
# ]
# 10 0x01   1 ID1
# 11 0x94 148 ID2
# 12 0xc9 201 ID3
# 13 0x40 064 ID4
# 14 0x00   0 (status)
# 15 0x01   1 (number sub telegram)
# 16 0xff 255 Optionnal2 Destination ID
# 17 0xff 255 Optionnal3 Destination ID
# 18 0xff 255 Optionnal4 Destination ID
# 19 0xff 255 Optionnal5 Destination ID
# 20 0x4d  77 Optionnal6 Dbm
# 21 0x00  0  Optionnal7 Security level
# 22 0x43  67 CRC8D


def mea_xplCmndMsg(data):
   fn_name=sys._getframe().f_code.co_name

   try:
      id_sensor=data["device_id"]
      parameters=data["device_parameters"]
      typeoftype=data["typeoftype"]
      api_key=data["api_key"]
   except:
      verbose(2, "ERROR (", fn_name, ") - invalid data")
      return 0

   mem=mea.getMemory(id_sensor)

   paramsDict=mea_utils.parseKeyValueDatasToDictionary(parameters, ",", ":")

   x=data["xplmsg"]
   body=x["body"]

   target="*"
   if "source" in x:
      target=x["source"]

   channel = -1;
   current = -1;
   if x["schema"]=="control.basic":
     try:
        if body["type"] == "output":
           if body["current"]=="high":
              current = 100;
           elif body["current"]=="low":
              current = 0; 

           if current <> -1:
              channel = ord(paramsDict["channel"][0]) - ord('A');
              buf=bytearray()
              buf.append(0x01)
              buf.append(channel & 0b11111)
              buf.append(current & 0b1111111)

              mea.interfaceAPI(api_key, "sendEnoceanRadioErp1Packet", 0xD2, 0, data["ENOCEAN_ADDR"], buf)

              try:
                 if paramsDict["force_status"]=="yes":
                    channel = ord(paramsDict["channel"][0]) - ord('A');
                    buf=bytearray()
                    buf.append(0x03)
                    buf.append(channel & 0b11111)
                    mea.interfaceAPI(api_key, "sendEnoceanRadioErp1Packet", 0xD2, 0, data["ENOCEAN_ADDR"], buf)
              except Exception as e:
                 verbose(2, "ERROR (", fn_name, ") - can't query device: ", str(e)) 
                 pass

     except Exception as e:
        verbose(2, "ERROR (", fn_name, ") - can't create/send enocean packet: ", str(e))
        return False
 
   elif x["schema"]=="sensor.request":
      if body["request"]=="current":
         try:
            buf=bytearray()
            channel = ord(paramsDict["channel"][0]) - ord('A');
            buf.append(0x03);
            buf.append(channel & 0b11111);
            mea.interfaceAPI(api_key,"sendEnoceanRadioErp1Packet", 0xD2, 0, data["ENOCEAN_ADDR"], buf)
         except Exception as e:
            verbose(2, "ERROR (", fn_name, ") - can't query device: ", str(e))
            return False

      else:
         verbose(2, "ERROR (", fn_name, ") - invalid request (", body["request"],")")
         return False

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
      typeoftype=data["typeoftype"]
   except:
      verbose(2, "ERROR (", fn_name, ") - invalid data")
      return False

   mem=mea.getMemory(id_sensor)
   paramsDict=mea_utils.parseKeyValueDatasToDictionary(parameters, ",", ":")

   f = "highlow"
   try:
      f=paramsDict["format"]
   except:
      pass

   current=""
   try:
      device=paramsDict["xpldevice"];
   except:
      device=-1;

   if packet[4]==1: # ENOCEAN_RADIO_ERP1
      if packet[6]==0xD2:
         if (packet[7] & 0b00001111) == 4: # reception packet status
            try:
               data_channel_num = packet[8] & 0b00011111;
               device_channel_num = ord(paramsDict["channel"][0]) - ord('A');
 
               #if (paramsDict["channel"]=="A" and (packet[8] & 0b00011111) == 0) or (paramsDict["channel"]=="B" and (packet[8] & 0b00011111) == 1): 
               if data_channel_num == device_channel_num:
                  if f == "highlow":
                     if (packet[9] & 0b01111111) == 0:
                        current='low'
                     else:
                        current='high'
                  elif f == "value":
                     current = packet[9] & 0b01111111;
            except:
               return False 

            if current != "":
               try:
                  mem["last"]=mem["current"]
               except:
                  mem["last"]=False
               mem["current"]=current

               xplMsg=mea_utils.xplMsgNew("me", "*", "xpl-trig", "sensor", "basic")
               mea_utils.xplMsgAddValue(xplMsg,"device", data["device_name"].lower())
               mea_utils.xplMsgAddValue(xplMsg,"current", mem["current"])
               mea_utils.xplMsgAddValue(xplMsg,"type", "input")
               mea_utils.xplMsgAddValue(xplMsg,"last", mem["last"])
               mea.xplSendMsg(xplMsg)
   
               return True
   return False
