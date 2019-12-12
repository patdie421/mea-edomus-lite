import sys
import json

from optparse import OptionParser

from lib import http
from lib import session
from lib import display
from lib.mea_utils import *


def get_configurations(host, port, sessionid):
   return GetUrl("http://"+str(host)+":"+str(port)+"/rest/configuration",sessionid)   


def get_configuration(host, port, sessionid, parameter):
   return GetUrl("http://"+str(host)+":"+str(port)+"/rest/configuration/"+str(parameter),sessionid)


def set_configuration_parameter(host, port, sessionid, parameter, value):
    return PostUrl("http://"+str(host)+":"+str(port)+"/rest/configuration/"+str(name),sessionid)


def set_configuration_parameters(host, port, sessionid, parameters):
    return PutUrl("http://"+str(host)+":"+str(port)+"/rest/configuration",sessionid)


def _set(host, port, sessionid, options, args):
   result=None
   if len(args)==0:
      return False
   else:
      j={}
      for i in args:
         s=i.split(':')
         if len(s) != 2:
            return False
         j[s[0].strip()]=s[1].strip()
      if len(j)>0:
         code,result = set_configuration_parameters(host, port, sessionid, j)
         display.formated(result, options.format)
         return True
   return False
          

def _get(host, port, sessionid, options, args):
   result=None
   if len(args)==0:
      code,result=get_configurations(host, port, sessionid)
   elif len(args)==1:
      code,result=get_configuration(host, port, sessionid, args[0])
   else:
      display.error("parameter error")
      return False
   display.formated(result, options.format)
   return True


def do(host, port, sessionid, args):
   
   try:
      action=args.pop(0).lower()
   except:
      action=False

   usage = "\
usage: %prog configuration get [parameter] [options]\n\
       %prog configuration set <parameter:value>+ [options]"
   parser = OptionParser(usage)
   parser.add_option("-f", "--format", dest="format", help="ouput format : [json|txt]", default="json")
   (options, args) = parser.parse_args(args=args)

   if action==False:
      parser.print_help()
      return False

   if action in ["get","set"]:
      if action=="get":
         if _get(host, port, sessionid, options, args)==False:
            parser.print_help()
            return False
         else:
            return True
      elif action=="set":
         if _set(host, port, sessionid, options, args)==False:
            parser.print_help()
            return False
         else:
            return True
   else:
      parser.print_help()
      return False
