import sys
import json
import argparse

from lib import http
from lib import session
from lib import display
from lib.mea_utils import *


def get_configurations(host, port, sessionid):
   return GetUrl("http://"+str(host)+":"+str(port)+"/rest/configuration",sessionid)   


def get_configuration(host, port, sessionid, parameter):
   return GetUrl("http://"+str(host)+":"+str(port)+"/rest/configuration/"+str(parameter),sessionid)


#def set_configuration_parameter(host, port, sessionid, parameter, value):
#    return PostUrl("http://"+str(host)+":"+str(port)+"/rest/configuration/"+str(name),sessionid)


def set_configuration_parameters(host, port, sessionid, parameters):
    return PutUrl("http://"+str(host)+":"+str(port)+"/rest/configuration",sessionid, parameters)


def _set(host, port, sessionid, args, _args):
   result=None
   if args.key==None:
      return False
   keyvalue=[args.key]+args.keyvalue
   if len(keyvalue)==0:
      return False
   else:
      j={}
      for i in keyvalue:
         s=i.split(':')
         if len(s) != 2:
            return False
         j[s[0].strip()]=s[1].strip()

      if len(j)>0:
         code,result = set_configuration_parameters(host, port, sessionid, j)
         display.formated(result, args.format)
         return True
   return False
          

def _get(host, port, sessionid, args, _args):
   result=None
   if len(_args)>0:
      display.error("too many parameters")
      return False
   if args.key==None:
      code,result=get_configurations(host, port, sessionid)
   else:
      code,result=get_configuration(host, port, sessionid, args.key)
   display.formated(result, args.format)
   return True


cli_epilog='''
Actions:
   get [key]
   set <key>:<value> [<propertie>:<value> [<propertie>:<value>] ...]]
'''

cli_description='''
CLI configuration get/set
'''

def parser(args_subparser, parent_parser):
   configuration_parser=args_subparser.add_parser('configuration', parents=[parent_parser], add_help=False, description=cli_description, epilog=cli_epilog, formatter_class=argparse.RawDescriptionHelpFormatter)
   configuration_parser.add_argument("action", help="", choices=['get','set'])
   configuration_parser.add_argument("key", help="<key> for get", nargs="?")
   configuration_parser.add_argument("keyvalue", metavar='key:value', help="<key>/<value> pairs for set", nargs="*")

   return configuration_parser


actions={}
actions["get"]=_get
actions["set"]=_set

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
