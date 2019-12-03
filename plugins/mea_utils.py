from sets import Set
import string
import re
import sys
from datetime import datetime

try:
   import mea
except:
   import mea_simulation as mea 

allowed_chars  = Set(string.ascii_lowercase + string.digits)
allowed_chars_other = Set("-")


current_verbose_level=9

def verbose(level, *args):
   if level <= current_verbose_level:
      sys.stderr.write("[")
      now = datetime.now()
      sys.stderr.write(now.strftime('%Y-%m-%d %H:%M:%S'))
      sys.stderr.write("] ")
      for i in args:
         sys.stderr.write(str(i))
      sys.stderr.write("\n")
      sys.stderr.flush()


# retourne True si tous les caracteres sont autorises en tenant compte de
# dash_allowe et si la longueur est inferieure a max_str_len
def isXplValidCharacters(s, max_str_len, dash_allowed=True):
   if len(s) < 1 or len(s) > max_str_len:
      return False
   
   if dash_allowed == False:
      return Set(s).issubset(allowed_chars)
   else:
      return Set(s).issubset(allowed_chars) or Set(s).issubset(allowed_chars_other)


def isXplMessageType(s):
   if s=="xpl-cmnd" or s=="xpl-stat" or s=="xpl-trig":
      return True
   else:
      return False


def isXplAddress(addr):
   if addr=="*":
      return True
   if not re.match("^[0-9a-z]*-[0-9a-z]*\.[0-9a-z\-]*$", addr):
      return False
   (vendorIdAndDeviceId, instanceId) = addr.split(".")
   (vendorId, deviceId) = vendorIdAndDeviceId.split("-")
   if isXplValidCharacters(vendorId, 8, False) and isXplValidCharacters(deviceId, 8, False) and isXplValidCharacters(instanceId, 16, True):
      return True
   else:
      return False


def xplAddressNew(id_vendor, id_device, id_instance):
   if isXplValidCharacters(id_vendor.lower(),8,False):
      return None
   elif not isXplValidCharacters(id_device.lower(),8, False):
      return None
   elif not isXplValidCharacters(id_instance.lower(),16, True):
      return None
   addr = id_vendor.lower()+ "-" + id_device.lower() + "." + id_instance.lower()
   return addr


def xplMsgNew(source_addr, target_addr, xpl_message_type, xpl_class, xpl_type):
   # car "xpl" autorises : a-z, 0-9 and "-"
   # controle des parameters :
   # source_addr
   # target_addr
   # msgtype = [ "xpl-cmnd" | "xpl-stat" | "xpl-trig" ]
   # xpl_class 8 car "xpl"
   # xpl_type 8 car "xpl"

   if source_addr.lower() == "me":
      source_addr = mea.xplGetVendorID() + "-" + mea.xplGetDeviceID() + "." + mea.xplGetInstanceID()

   if isXplAddress(source_addr) == False or isXplAddress(target_addr) == False:
      return None
   if source_addr == "*":
      return None
   if not isXplMessageType(xpl_message_type.lower()):
      return None
   if not isXplValidCharacters(xpl_class.lower(),8, True):
      return None
   if not isXplValidCharacters(xpl_type.lower(),8, True):
      return None

   # le format des donnees est valide, on peut creer le message
   xplMsg={} # creation du dictionnaire
   xplMsg["xplmsg"]=True # pour des besoins internes (controle)
   xplMsg["msgtype"]=xpl_message_type.lower()
   xplMsg["hop"]="1"
   xplMsg["source"]= source_addr
   xplMsg["target"]= target_addr
   xplMsg["schema"]=xpl_class.lower() + "." + xpl_type.lower()
   body={}
   xplMsg["xpl-body"]=body

   return xplMsg


def xplMsgAddValue(xplMsg, key, value):
   if not isXplValidCharacters(key.lower(),16,True):
      return False
   
   if "xplmsg" in xplMsg:
      if xplMsg["xplmsg"] != True:
         return False
   
   xpl_body = xplMsg.get("xpl-body")
   if xpl_body == None:
      xpl_body={}
   
   xpl_body[key.lower()]=str(value)
   
   xplMsg["xpl-body"]=xpl_body
   
   return True


def xplMsgAddValues(xplMsg, aDict):
   if "xplmsg" in xplMsg:
      if xplMsg["xplmsg"] != True:
         return False
   
   for i in aDict:
      if not isXplValidCharacters(i.lower(),16,True):
         return False
   
   xpl_body= xplMsg.get("xpl-body")
   if xpl_body == None:
      xpl_body={}
   
   for i in aDict:
      xpl_body[i.lower()]=str(aDict[i])
   
   xplMsg["xpl-body"]=xpl_body
   
   return True


def xplMsgSetValues(xplMsg, aDict):
   for i in aDict:
      if not isXplValidCharacters(i,16,True):
         return False
   
   xplMsg["xpl-body"]=aDict.copy();
   
   return True


def xplMsgPrint(xplMsg):
   try:
      sys.stderr.write(xplMsg["msgtype"])
      sys.stderr.write("{")
      sys.stderr.write("hop=" + xplMsg["hop"])
      sys.stderr.write("source=" + xplMsg["source"])
      sys.stderr.write("target=" + xplMsg["target"])
      sys.stderr.write("}")
      sys.stderr.write(xplMsg["schema"])
      sys.stderr.write("{");
      try:
         body=xplMsg["xpl-body"]
         for i in body:
            sys.stderr.write(i+"="+body[i])
      except:
         sys.stderr.write("<probably no body now !!!>")
      sys.stderr.write("}")
   except:
      sys.stderr.write("not a xpl message")


def xplMsgToString(xplMsg):
   try:
      s=xplMsg["msgtype"] + "\n{\n" + "hop=" + xplMsg["hop"] + "\nsource=" + xplMsg["source"] + "\ntarget=" + xplMsg["target"] + "\n}\n" + xplMsg["schema"] + "\n{\n"
      body=xplMsg["xpl-body"]
      for i in body:
         s = s + i + "=" + body[i] + "\n"
      s = s + "}\n"
      return s
   except:
      return None


def parseKeyValueDatasToDictionary(aString, sep_assoc=";", sep_keyvalue="="):
   assocList=aString.split(sep_assoc)
   if assocList:
      keysValuesDict={}
      for i in assocList:
         keyValue=i.split(sep_keyvalue)
         if len(keyValue) == 1:
            keysValuesDict[(keyValue[0].strip()).lower()]=None
         elif len(keyValue) == 2:
            keysValuesDict[(keyValue[0].strip()).lower()]=keyValue[1].strip()
         else:
            return None
      return keysValuesDict
   return None


def parseKeyValueDatasToList(aString, sep_assoc=";", sep_keyvalue="="):
   assocList=aString.split(sep_assoc)
   if assocList:
      keysValuesList=[]
      for i in assocList:
         keyValue=i.split(sep_keyvalue)
         if len(keyValue) == 1:
            keysValuesList.append(((keyValue[0].strip()).lower(),None))
         elif len(keyValue) == 2:
            keysValuesList.append(((keyValue[0].strip()).lower(),keyValue[1].strip()))
         else:
            return None
      return keysValuesList
   return None