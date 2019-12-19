import sys
#if sys.version_info[0] < 3:
#   sys.stderr.write("Only compatible with Python 3 interpreter\n")
#   sys.stderr.write(str(sys.version_info)+"\n")
#   sys.exit(1)
import base64
import json
import os
import getpass
import argparse

from datetime import datetime
from lib import http
from lib import session

from modules import interface
from modules import pairing
from modules import configuration
from modules import user
from modules import service
from modules import type

cli_epilog='''
Available resources:
  configuration: display/change mea-edomus configuration properties
  user:          manage mea-edomus user, profile and password
  service:       manage services
  type:          manage types of interfaces and devices
  interface:     manage interfaces and associated devices
  pairing:       manage pairing processes
'''

cli_description='''
CLI for configuring mea-edomus.
'''

objects_functions={}
objects_functions["interface"]=interface.do
objects_functions["pairing"]=pairing.do
objects_functions["configuration"]=configuration.do
objects_functions["user"]=user.do
objects_functions["service"]=service.do
objects_functions["type"]=type.do

_parsers={}

#interface.parser(args_parser_interface)

def getArgs():
   args_common = argparse.ArgumentParser()
   args_common.add_argument("-H", "--host", dest="host", help="mea-edomus host (default: localhost)", default="localhost")
   args_common.add_argument("-P", "--port", dest="port", help="mea-edomus communication port (default: 8083)", default=8083)
   args_common.add_argument("-U", "--user", dest="_user", metavar="USER", help="user connection id", default=None)
   args_common.add_argument("-W", "--password", dest="password", help="user connection password", default=None)
   args_common.add_argument("-f", "--format", dest="format", choices=['json','text'], help="ouput format (default: json)", default="json")
   args_common.add_argument("-r", "--reset-session", action="store_true", dest="clean", help="remove local connection data before command execution", default=False)

   args_parser = argparse.ArgumentParser(description=cli_description, epilog=cli_epilog, formatter_class=argparse.RawDescriptionHelpFormatter, parents=[args_common], add_help=False)
   args_subparser = args_parser.add_subparsers(dest='resource')

   _parsers['configuration'] = configuration.parser(args_subparser, args_common)
   _parsers['interface']     = interface.parser(args_subparser, args_common)
   _parsers['service']       = service.parser(args_subparser, args_common)
   _parsers['user']          = user.parser(args_subparser, args_common)
   _parsers['type']          = type.parser(args_subparser, args_common)
   _parsers['pairing']       = pairing.parser(args_subparser, args_common)

   args, _args = args_parser.parse_known_args()
   
   return args_parser, args, _args


if __name__ == "__main__":

   homedir = os.path.expanduser("~")
   dotfile = os.path.join(homedir, ".mea-edomus")

   args_parser, args, _args = getArgs()
   if args.clean==True:
      try:
         os.remove(dotfile)
      except:
         pass

   if args.resource == None:
      args_parser.print_help()
      sys.exit(1)
      
   o=args.resource.lower()
#   if not o in objects_functions:
#      parser.print_help()
#      sys.exit(1)

   if args.clean==True:
      try:
         os.remove(dotfile)
      except:
         pass

   defaultsession={
      "sessionid":False,
      "host":args.host,
      "port":args.port,
      "user":args._user}

   if(not os.path.isfile(dotfile)):
      mysession=defaultsession
   else:
      with open(dotfile) as json_file:
         mysession = json.load(json_file) 
         if mysession["host"] != args.host:
            mysession["host"]=args.host
            mysession["sessionid"]=False
         if mysession["port"] != args.port:
            mysession["port"]=args.port
            mysession["sessionid"]=False
         if mysession["user"] != args._user:
            mysession["user"]=args._user
            mysession["sessionid"]=False

   if args.password!=None:
      mysession["sessionid"]=False
   else:
      try:
         sessionid=mysession["sessionid"]
      except:
         sessionid=None
         pass

   if(mysession["sessionid"]==False or session.check(args.host, args.port, mysession["sessionid"])==False):
      if args._user==None:
         if sys.version_info[0]<3:
            user=raw_input('user: ')
         else:
            user=input('user: ')
      else:
         user=args._user

      if args.password==None:
         password=getpass.getpass("password:")
      else:
         password=args.password

      code,_session=session.open(args.host, args.port, user, password)
      if code==200:
         _session=json.loads(_session)
         mysession["sessionid"]=_session["Mea-SessionId"]
         with open(dotfile, 'w') as outfile:
            json.dump(mysession, outfile)
      else:
         mysession["sessionid"]=None
         with open(dotfile, 'w') as outfile:
            json.dump(mysession, outfile)
         sys.stderr.write(str(_session))
         sys.exit(1)

   sessionid=mysession["sessionid"]

   if o in objects_functions:
      r=objects_functions[o](args.host, args.port, sessionid, args, _args=_args, _parser=_parsers[o])
      if r==True:
         sys.exit(0)
      else:
         sys.exit(1)
   else:
      sys.exit(1)
