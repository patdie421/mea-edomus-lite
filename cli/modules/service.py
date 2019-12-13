import sys
import json
import argparse

from lib import http
from lib import session
from lib import display
from lib.mea_utils import *


def get_services(host, port, sessionid):
   return GetUrl("http://"+str(host)+":"+str(port)+"/rest/service",sessionid)


def get_service(host, port, sessionid, service):
   return GetUrl("http://"+str(host)+":"+str(port)+"/rest/service/"+str(service),sessionid)


def do_action(host, port, sessionid, serviceid, action):
   return PutUrl("http://"+str(host)+":"+str(port)+"/rest/service/"+str(serviceid),sessionid,{"action":action})


def _actions(action, host, port, sessionid, args, _args):
   if args.pid==None:
      display.error("pid is mandatory")
      return False
   if len(_args)>0:
      display.error("too many parameters")
      return False

   code, result=do_action(host, port, sessionid, args.pid, action)
   display.formated(result, args.format)
   return True


def _get(host, port, sessionid, args, _args):
   result=None
   
   if len(_args)!=0:
      display.error("too many parameters")
      return False

   if args.pid==None:
      code,result=get_services(host, port, sessionid)
   else:
      code,result=get_service(host, port, sessionid, args.pid)

   display.formated(result, args.format)
   return True


def _start(host, port, sessionid, args, _args):
   _actions("start", host, port, sessionid, args, _args)
   return True


def _stop(host, port, sessionid, args, _args):
   _actions("stop", host, port, sessionid, args, _args)
   return True


def _restart(host, port, sessionid, args, _args):
   _actions("restart", host, port, sessionid, args, _args)
   return True


cli_epilog='''
Actions and options:
   get    [pid]
   start   pid
   stop    pid
   restart pid
'''

cli_description='''
CLI service control
'''

def parser(args_subparser, parent_parser):
   service_parser=args_subparser.add_parser('service', parents=[parent_parser], add_help=False, description=cli_description, epilog=cli_epilog, formatter_class=argparse.RawDescriptionHelpFormatter)
   service_parser
   service_parser.add_argument("action", help="get information, start, stop or restart sevices", choices=['get','start','stop','restart'])
   service_parser.add_argument("pid", help="service process id. pid is mandatory for \"start\", \"stop\" or \"restart\" action.", nargs="?")
   return service_parser


actions={}
actions["get"]=_get
actions["start"]=_start
actions["stop"]=_stop
actions["restart"]=_restart


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
