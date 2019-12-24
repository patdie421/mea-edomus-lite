import sys
import json
import argparse

from lib import http
from lib import session
from lib import display
from lib.mea_utils import *


def get_metrics(host, port, sessionid):
   return GetUrl("http://"+str(host)+":"+str(port)+"/rest/metric",sessionid)


def get_metric(host, port, sessionid, pid):
   return GetUrl("http://"+str(host)+":"+str(port)+"/rest/metric/"+str(pid),sessionid)


def _get(host, port, sessionid, args, _args):
   _pid=args.pid
   if _pid==None:
      code,result=get_metrics(host, port, sessionid)
   else:
      code,result=get_metric(host, port, sessionid, _pid)
   display.formated(result, args.format)
   return True


actions={}
actions["get"]=_get

cli_epilog='''
Actions:
   get:
'''

cli_description='''
CLI get mea-edomus metrics
'''

def parser(args_subparser, parent_parser):
   type_parser=args_subparser.add_parser('metric', parents=[parent_parser], add_help=False, description=cli_description, epilog=cli_epilog, formatter_class=argparse.RawDescriptionHelpFormatter)
   type_parser
   type_parser.add_argument("action", choices=['get'])
   type_parser.add_argument("pid", help="process id", nargs="?")
   return type_parser


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
