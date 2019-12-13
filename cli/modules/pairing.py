import sys
import json
import argparse

from optparse import OptionParser
from lib import http
from lib import session
from lib import display
from lib.mea_utils import *

from modules import interface


def get_pairing_interfaces_status(host, port, sessionid):
    return GetUrl("http://"+str(host)+":"+str(port)+"/rest/pairing",sessionid)


def set_pairing_interface(host, port, sessionid, name, state):
    return PostUrl("http://"+str(host)+":"+str(port)+"/rest/pairing/"+str(name),sessionid,{ "state": state })


def get_pairable_interfaces(host, port, sessionid):
    code, res=get_pairing_interfaces_status(host, port, sessionid)
    _res=[]
    for i in res:
       _res.append(i) 
    return code, _res


def get_pairable_interface(host, port, sessionid, name):
    code, res=interface.get_interface(host, port, sessionid, name)
    if code==200:
       url="http://"+str(host)+":"+str(port)+"/rest/pairing/"+str(name)
       headers={"Mea-session": sessionid}
       code, res=http.get(url, headers)
       res=json.loads(res)
       if res==0 or res==1:
          return code, True
       else:
          return code, False
    return code, None


def _stop(host, port, sessionid, options, args):
   if args.name!=None:
      code, result=set_pairing_interface(host, port, sessionid, args.name, 0)
      display.formated(result, args.format)
      if(code==200):
         return True
      else:
         return False
   else:
      display.error("parameter error")
      return False

 
def _start(host, port, sessionid, args, _args):
   if args.name!=None:
      code, result=set_pairing_interface(host, port, sessionid, args.name, 1)
      display.formated(result, args.format)
      if(code==200):
         return True
      else:
         return False
   else:
      display.error("parameter error")
      return False


def _status(host, port, sessionid, args, _args):
   code,result=get_pairing_interfaces_status(host, port, sessionid)
   if args.name==None:
      display.formated(result,args.format)
      return True
   else:
      if args.name.upper() in result:
         display.formated(result[args.name.upper()], args.format)
         return True
      display.formated(None,args.format)
      return False

 
def _pairable(host, port, sessionid, args, _args):
   _result=None
   if args.name==None:
      code, result=get_pairable_interfaces(host, port, sessionid)
   else:
      code, result=get_pairable_interface(host, port, sessionid, args.name)
   display.formated(result, args.format)
   return True


actions={}
actions["get"]=_pairable
actions["start"]=_start
actions["stop"]=_stop
actions["status"]=_status

cli_epilog='''
Actions:
   get:
'''

cli_description='''
CLI pairing operation for compatible interface
'''

def parser(args_subparser, parent_parser):
   pairing_parser=args_subparser.add_parser('pairing', parents=[parent_parser], add_help=False, description=cli_description, epilog=cli_epilog, formatter_class=argparse.RawDescriptionHelpFormatter)
   pairing_parser
   pairing_parser.add_argument("action", choices=['get', 'start', 'stop', 'restart' ])
   pairing_parser.add_argument("name", help="interface name", nargs="?")
   return pairing_parser


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
