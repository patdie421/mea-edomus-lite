import sys
import json
import argparse

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


def add_interface_device(host, port, sessionid, interface, device, properties):
   properties["name"]=device
   return PostUrl("http://"+str(host)+":"+str(port)+"/rest/interface/"+str(interface)+"/device",sessionid,properties)


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


def _update_interface_device(host, port, sessionid, interface, args, _args):
   v=args.option.pop(0).strip().lower()
   if v!="device":
      return -1, None
   else:
      if len(args.option)==0:
         return -1, None
      else:
         j={}
         device=args.option.pop(0).strip().lower()
         possibles=["id_type", "state", "description", "parameters"]
         for i in args.option:
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


def _add_interface_device(host, port, sessionid, interface, args, _args):
   v=args.option.pop(0).strip().lower()
   if v!="device":
      return -1, None
   else:
      if len(args.option)==0:
         return -1, None
      else:
         j={}
         device=args.option.pop(0).strip().lower()
         possibles=["id_type", "state", "description", "parameters"]
         for i in args.option:
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


def _get_interface_device(host, port, sessionid, interface, args, _args):
   v=args.option.pop(0).strip().lower()
   if v!="device":
      return -1, None
   else:
      if len(args.option)==0:
         code, result = get_devices(host, port, sessionid, interface)
         return code, result
      elif len(args.option)==1:
         code, result = get_device(host, port, sessionid, interface, args.option[0])
         return code, result
      else:
         display.error("too many parameters")
         return -1, None


def _get(host, port, sessionid, args, _args):
   result=None
   if args.name==None:
      code,result=get_interfaces(host, port, sessionid)
   elif len(args.option)==0:
      code,result=get_interface(host, port, sessionid, args.name)
   else:
      interface=args.name
      code,result=_get_interface_device(host, port, sessionid, interface, args, _args)
      if code==-1:
         return False
   display.formated(result, args.format)
   return True


def _update(host, port, sessionid, args, _args):
   interface=args.name
   if interface==None:
      display.error("no interface")
      return False
   j={}

   if len(args.option)>1 and args.option[0].strip().lower()=="device":
      code, result = _update_interface_device(host, port, sessionid, interface, args, _args)
   else:
      possibles=["id_type", "state", "dev", "description", "parameters"]
      for i in args.option:
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
      else:
         display.error("no property")
         return False
   display.formated(result, args.format)
   return True


def _delete(host, port, sessionid, args, _args):
   interface=args.name
   if len(args.option)==0:
      code, result = delete_interface(host, port, sessionid, interface)
      display.formated(result, args.format)
      return True
   if len(args.option)==2 and args.option[0].strip().lower() == "device":
      device=args.option[1].strip().lower()
      code, result = delete_interface_device(host, port, sessionid, interface, device)
      display.formated(result, args.format)
      return True
   else:
      display.error("bad parameters")
      return False


def _add(host, port, sessionid, args, _args):
   interface=args.name
   if interface==None:
      display.error("no interface")
      return False
   j={}

   if len(args.option)>1 and args.option[0].strip().lower()=="device":
      code, result = _add_interface_device(host, port, sessionid, interface, args, _args)
      if code==-1:
         return False
   else:
      possibles=["id_type", "state", "dev", "description", "parameters"]
      for i in args.option:
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
   display.formated(result, args.format)
   return True


actions={}
actions["get"]=_get
actions["add"]=_add
actions["update"]=_update
actions["delete"]=_delete


cli_epilog='''
Actions and options:
   get      [<interface name>] [device [<device name>]]
   delete    <interface name>  [device <device name>]
   add       <interface name>  type_id:<type_id> state:<state> dev:<dev> [parameters:<parameters>] [description:<description>]
   add       <interface name>  device <device name> [type_id:<type_id>] [state:<state>] [parameters:<parameters>] [description:<description>]
   update    <interface name>  [type_id:<type_id>] [state:<state>] [dev:<dev>] [parameters:<parameters>] [description:<description>]
   update    <interface name>  device <device name> [type_id:<type_id>] [state:<state>] [parameters:<parameters>] [description:<description>]
   commit
   rollback
'''

cli_description='''
CLI interfaces and devices managment
'''

def parser(args_subparser, parent_parser):
   interface_parser=args_subparser.add_parser('interface', parents=[parent_parser], add_help=False, description=cli_description, epilog=cli_epilog, formatter_class=argparse.RawDescriptionHelpFormatter)
   interface_parser.add_argument('action', choices=['get', 'add', 'update', 'delete'], help="available actions")
   interface_parser.add_argument('name', help="interface name", nargs="?")
#   interface_parser.add_argument("keyvalue", metavar='key:value', help="<key>/<value> pairs for set", nargs="*")
   interface_parser.add_argument('option', nargs="*", help="options or properties for interface/device action (see below)")
   return interface_parser


def do(host, port, sessionid, args, _args=[], _parser=None):
   try:
      action=args.action
   except:
      display.error("No action", errtype="ERROR", errno=1)
      user_parser.print_help()
      return False

   if action in actions:
      if actions[action](host, port, sessionid, args, _args)==False:
         return False
      else:
         return True
   else:
      if _parser:
         display.error("Bad action", errtype="ERROR", errno=1)
         _parser.print_help()
   return False
