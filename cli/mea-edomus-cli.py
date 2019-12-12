import sys
import base64
import json
import pprint
import urllib2
import os
import getpass
import argparse

from datetime import datetime
from lib import http
from lib import session
from lib import PassThroughOptionParser

from modules import interface
from modules import pairing
from modules import configuration
from modules import user
from modules import service
from modules import type

cli_epilog='''
Available artefacts:
  configuration: display/change mea-edomus configuration properties
  user:          manage mea-edomus user, profile and password
  service:       manage services
  type:          manage types of interfaces and devices
  interface:     manage interfaces and associated devices
  pairing:       manage pairing processes
'''

cli_description='''
CLI for managing mea-edomus-lite
'''

objects_functions={}
objects_functions["interface"]=interface.do
objects_functions["pairing"]=pairing.do
objects_functions["configuration"]=configuration.do
objects_functions["user"]=user.do
objects_functions["service"]=service.do
objects_functions["type"]=type.do


args_parser = argparse.ArgumentParser(description=cli_description, epilog=cli_epilog, formatter_class=argparse.RawDescriptionHelpFormatter)
args_subparsers = args_parser.add_subparsers(dest='artefact')

args_parser_interface = args_subparsers.add_parser('interface')
args_parser_configuration = args_subparsers.add_parser('configuration')
args_parser_service = args_subparsers.add_parser('service')
args_parser_user = args_subparsers.add_parser('user')
args_parser_type = args_subparsers.add_parser('type')
args_parser_pairing = args_subparsers.add_parser('pairing')

interface.parser(args_parser_interface)

def getArgs():
   args_parser.add_argument("-c", "--clean-session", action="store_true", dest="clean", help="remove local connection data", default=False)
   args_parser.add_argument("-H", "--host", dest="host", help="", default="localhost")
   args_parser.add_argument("-P", "--port", dest="port", help="", default=8083)
   args_parser.add_argument("-U", "--user", dest="user", help="connection user id", default=None)
   args_parser.add_argument("-W", "--password", dest="password", help="connection password", default=None)
#   args_parser.add_argument("artefact", help="<object> to manipulate")
#   args_parser.add_argument("action", help="<action> to be applied on the object")
#   args_parser.add_argument("option", nargs="*", help="options and properties for artefact/action")
   args, _args = args_parser.parse_known_args()
   print args
   
   _args = [args.artefact]+[args.action]+args.option+_args
   return args, _args


if __name__ == "__main__":

   homedir = os.path.expanduser("~")
   dotfile = os.path.join(homedir, ".mea-edomus")

   (options, args) = getArgs()
   parser=args_parser
   if options.clean==True:
      try:
         os.remove(dotfile)
      except:
         pass

   if len(args)<1:
      sys.exit(0)
 
   o=args.pop(0).lower()
   if not o in objects_functions:
      parser.print_help()
      sys.exit(1)

   defaultsession={
      "sessionid":False,
      "host":options.host,
      "port":options.port,
      "user":options.user}

   if(not os.path.isfile(dotfile)):
      mysession=defaultsession
   else:
      with open(dotfile) as json_file:
         mysession = json.load(json_file) 
         if mysession["host"] != options.host:
            mysession["host"]=options.host
            mysession["sessionid"]=False
         if mysession["port"] != options.port:
            mysession["port"]=options.port
            mysession["sessionid"]=False
         if mysession["user"] != options.user:
            mysession["user"]=options.user
            mysession["sessionid"]=False
   
   if options.password!=None:
      mysession["sessionid"]=False
   else:
      try:
         sessionid=mysession["sessionid"]
      except:
         sessionid=None
         pass


   if(mysession["sessionid"]==False or session.check(options.host, options.port, mysession["sessionid"])==False):
      if options.user==None:
         user=raw_input('user: ')
      else:
         user=options.user

      if options.password==None:
         password=getpass.getpass("password:")
      else:
         password=options.password

      code,_session=session.open(options.host, options.port, user, password)
      if code==200:
         _session=json.loads(_session)
         mysession["sessionid"]=_session["Mea-SessionId"]
         with open(dotfile, 'w') as outfile:
            json.dump(mysession, outfile)
      else:
         mysession["sessionid"]=None
         with open(dotfile, 'w') as outfile:
            json.dump(mysession, outfile)
         sys.stderr.write(_session)
         sys.exit(1)
   else:
      sessionid=mysession["sessionid"]

   if o in objects_functions:
      r=objects_functions[o](options.host, options.port, sessionid, args)
      if r==True:
         sys.exit(0)
      else:
         sys.exit(1)
   else:
      parser.print_help()
      sys.exit(1)
