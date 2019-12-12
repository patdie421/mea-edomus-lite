import sys
import json

from optparse import OptionParser

from lib import http
from lib import session
from lib import display
from lib.mea_utils import *
from modules import type


interface_states={ "0":0, "1":1, "2":2, "enabled":1, "disabled":0, "delegated":2, "inactive":0, "active":1 }
device_states={ "0":0, "1":1, "enabled":1, "disabled":0, "inactive":0, "active":1 }


def get_available_devices_types(host, port, sessionid):
   code, res=type.get_types(host, port, sessionid)
   _available_types={}
   if code==200:
      for i in res:
         if int(res[i]["typeoftype"])!=10:
            _available_types[i]=res[i]["id_type"]
            _available_types[str(res[i]["id_type"])]=res[i]["id_type"]
   return _available_types


def get_available_interfaces_types(host, port, sessionid):
   code, res=type.get_types(host, port, sessionid)
   _available_types={}
   if code==200:
      for i in res:
         if int(res[i]["typeoftype"])==10:
            _available_types[i]=res[i]["id_type"]
            _available_types[str(res[i]["id_type"])]=res[i]["id_type"]
   return _available_types


def printValids(message, validList):
   valids=""
   for i in validList:
      if len(valids)==0:
         valids=str(i)
      else:
         valids=valids+", "+str(i)
   display.error(message+": "+valids)


def update_interface_device(host, port, sessionid, interface, device, properties):
   return PutUrl("http://"+str(host)+":"+str(port)+"/rest/interface/"+str(interface)+"/device/"+str(device),sessionid,properties)
   return code, result


def add_interface_device(host, port, sessionid, interface, device, properties):
   properties["name"]=device
   return PostUrl("http://"+str(host)+":"+str(port)+"/rest/interface/"+str(interface)+"/device",sessionid,properties)
   return code, result


def delete_interface_device(host, port, sessionid, interface, device):
   return DeleteUrl("http://"+str(host)+":"+str(port)+"/rest/interface/"+str(interface)+"/device/"+str(device),sessionid)


def get_interfaces(host, port, sessionid):
   return GetUrl("http://"+str(host)+":"+str(port)+"/rest/interface",sessionid)


def get_interface(host, port, sessionid, interface):
   return GetUrl("http://"+str(host)+":"+str(port)+"/rest/interface/"+str(interface),sessionid)


def delete_interface(host, port, sessionid, interface):
   return DeleteUrl("http://"+str(host)+":"+str(port)+"/rest/interface/"+str(interface),sessionid)


def add_interface(host, port, sessionid, interface, properties):
   properties["name"]=interface
   return PostUrl("http://"+str(host)+":"+str(port)+"/rest/interface",sessionid,properties)


def update_interface(host, port, sessionid, interface, properties):
   return PutUrl("http://"+str(host)+":"+str(port)+"/rest/interface/"+interface,sessionid,properties)


def get_devices(host, port, sessionid, interface):
   return GetUrl("http://"+str(host)+":"+str(port)+"/rest/interface/"+str(interface)+"/device", sessionid)


def get_device(host, port, sessionid, interface, device):
   return GetUrl("http://"+str(host)+":"+str(port)+"/rest/interface/"+str(interface)+"/device/"+str(device),sessionid)


def _update_interface_device(host, port, sessionid, options, interface, args):
   v=args.pop(0).strip().lower()
   if v!="device":
      return -1, None
   else:
      if len(args)==0:
         return -1, None
      else:
         j={}
         device=args.pop(0).strip().lower()
         possibles=["id_type", "state", "description", "parameters"]
         for i in args:
            s=str(i).split(':',1)
            if len(s) != 2:
               display.error("property syntax error: "+str(i))
               return -1, None
            v=s[0].strip().lower()
            if not v in possibles:
               display.error("unknown property: "+str(v))
               return -1, None
            j[s[0].strip().lower()]=s[1].strip()

         if len(j)>0:
            if "id_type" in j:
               _available_types=get_available_devices_types(host, port, sessionid)
               if j["id_type"] in _available_types:
                  j["id_type"]=_available_types[j["id_type"]]
               else:
                  display.error("Not valid id_type values")
                  printValids("Valid id_type values", _available_types)
                  return -1, None
            
            if "state" in j:
               if j["state"] in device_states:
                  j["state"]=device_states[j["state"]]
               else:
                  display.error("Not valid id_type values")
                  printValids("Valid id_type values", device_states)
                  return -1, None

            code, result = update_interface_device(host, port, sessionid, interface, device, j)
            return code, result

         display.error("no properties")
         return -1, None
      display.error("unknown error")
      return -1, None


def _add_interface_device(host, port, sessionid, options, interface, args):
   v=args.pop(0).strip().lower()
   if v!="device":
      return -1, None
   else:
      if len(args)==0:
         return -1, None
      else:
         j={}
         device=args.pop(0).strip().lower()
         possibles=["id_type", "state", "description", "parameters"]
         for i in args:
            s=str(i).split(':',1)
            if len(s) != 2:
               display.error("property syntax error: "+str(i))
               return -1, None
            v=s[0].strip().lower()
            if not v in possibles:
               display.error("unknown property: "+str(v))
               return -1, None
            j[s[0].strip().lower()]=s[1].strip()

         if len(j)>0:
            if not "id_type" in j:
               display.error("id_type is mandatory")
               return -1, None
            _available_types=get_available_devices_types(host, port, sessionid)
            if j["id_type"] in _available_types:
               j["id_type"]=_available_types[j["id_type"]]
            else:
               display.error("Not valid id_type values")
               printValids("Valid id_type values", _available_types)
               return -1, None
            
            if not "state" in j:
               display.error("state is mandatory")
               return -1, None
            if j["state"] in device_states:
               j["state"]=device_states[j["state"]]
            else:
               display.error("Not valid id_type values")
               printValids("Valid id_type values", device_states)
               return -1, None

            code, result = add_interface_device(host, port, sessionid, interface, device, j)
            return code, result

         display.error("no properties")
         return -1, None
      display.error("unknown error")
      return -1, None


def _get_interface_device(host, port, sessionid, options, interface, args):
   v=args.pop(0).strip().lower()
   if v!="device":
      return -1, None
   else:
      if len(args)==0:
         code, result = get_devices(host, port, sessionid, interface)
         return code, result
      elif len(args)==1:
         code, result = get_device(host, port, sessionid, interface,args[0])
         return code, result
      return -1, None


def _get(host, port, sessionid, options, args):
   result=None
   if len(args)==0:
      code,result=get_interfaces(host, port, sessionid)
   elif len(args)==1:
      code,result=get_interface(host, port, sessionid, args[0])
   else:
      interface=args.pop(0)
      code,result=_get_interface_device(host, port, sessionid, options, interface, args)
      if code==-1:
         return False
   display.formated(result, options.format)
   return True


def _update(host, port, sessionid, options, args):
   interface=args.pop(0)
   j={}

   if len(args)>2 and args[0].strip().lower()=="device":
      code, result = _update_interface_device(host, port, sessionid, options, interface, args)
   else:
      possibles=["id_type", "state", "dev", "description", "parameters"]
      for i in args:
         s=str(i).split(':',1)
         if len(s) != 2:
            display.error("property syntax error: "+str(i))
            return False
         v=s[0].strip().lower()
         if not v in possibles:
            display.error("unknown property: "+str(v))
            return False
         j[s[0].strip().lower()]=s[1].strip()

      if len(j)>0:
         if "id_type" in j:
            _available_types=get_available_interfaces_types(host, port, sessionid)
            if j["id_type"] in _available_types:
               j["id_type"]=_available_types[j["id_type"]]
            else:
               display.error("Not valid id_type values")
               printValids("Valid id_type values", _available_types)
               return False
         if "state" in j:   
            if j["state"] in interface_states:
               j["state"]=interface_states[j["state"]]
            else:
               display.error("Not valid id_type values")
               printValids("Valid id_type values", interface_states)
               return False
         code, result = update_interface(host, port, sessionid, interface, j)
#         display.formated(result, options.format)
#         return True
      else:
         display.error("no property")
         return False
   display.formated(result, options.format)
   return True


def _delete(host, port, sessionid, options, args):
   interface=args.pop(0)
   if len(args)==0:
      code, result = delete_interface(host, port, sessionid, interface)
      display.formated(result, options.format)
      return True
   if len(args)==2 and args[0].strip().lower() == "device":
      device=args[1].strip().lower()
      code, result = delete_interface_device(host, port, sessionid, interface, device)
      display.formated(result, options.format)
      return True
   else:
      display.error("bad parameters")
      return False


def _add(host, port, sessionid, options, args):
   interface=args.pop(0)
   j={}

   if len(args)>2 and args[0].strip().lower()=="device":
      code, result = _add_interface_device(host, port, sessionid, options, interface, args)
      if code==-1:
         return False
   else:
      possibles=["id_type", "state", "dev", "description", "parameters"]
      for i in args:
         s=str(i).split(':',1)
         if len(s) != 2:
            display.error("property syntax error: "+str(i))
            return False
         v=s[0].strip().lower()
         print v
         if not v in possibles:
            display.error("unknown property: "+str(v))
            return False
         j[s[0].strip().lower()]=s[1].strip()

      if len(j)>0:
         if not "id_type" in j:
            display.error("id_type is mandatory")
            return False
         _available_types=get_available_interfaces_types(host, port, sessionid)
         if j["id_type"] in _available_types:
            j["id_type"]=_available_types[j["id_type"]]
         else:
            display.error("Not valid id_type values")
            printValids("Valid id_type values", _available_types)
            return False
         
         if not "state" in j:
            display.error("state is mandatory")
            return False
         if j["state"] in interface_states:
            j["state"]=interface_states[j["state"]]
         else:
            display.error("Not valid id_type values")
            printValids("Valid id_type values", interface_states)
            return False

         if not "dev" in j:
            display.error("dev is mandatory")
            return False

         code, result = add_interface(host, port, sessionid, interface, j)
      else:
         display.error("no property")
         return False

   display.formated(result, options.format)
   return True


actions={}
actions["get"]=_get
actions["add"]=_add
actions["update"]=_update
actions["delete"]=_delete


def do(host, port, sessionid, args):
   try:
      action=args.pop(0).lower()
   except:
      action=False

   usage = "usage: %prog interface <action> [object] [options]"
   parser = OptionParser(usage)
   parser.add_option("-f", "--format", dest="format", help="ouput format : [json|txt]", default="json")
   (options, args) = parser.parse_args(args=args)

   if action==False:
      parser.print_help()
      return False

   if action in actions:
      if actions[action](host, port, sessionid, options, args)==False:
         parser.print_help()
         return False
      else:
         return True
   else:
      parser.print_help()
      return False
