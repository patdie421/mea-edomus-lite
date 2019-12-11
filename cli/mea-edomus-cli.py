import sys
import base64
import json
import pprint
import urllib2
import os
import getpass
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

objects_functions={}
objects_functions["interface"]=interface.do
objects_functions["pairing"]=pairing.do
objects_functions["configuration"]=configuration.do
objects_functions["user"]=user.do
objects_functions["service"]=service.do
objects_functions["type"]=type.do

if __name__ == "__main__":

   homedir = os.path.expanduser("~")
   dotfile = os.path.join(homedir, ".mea-edomus")

   usage = "usage: %prog <object> <action> [object] [options]"

   parser = PassThroughOptionParser.PassThroughOptionParser(usage)

   parser.add_option("-c", "--clean-session", action="store_true", dest="clean", help="", default=False)
   parser.add_option("-H", "--host", dest="host", help="", default="localhost")
   parser.add_option("-P", "--port", dest="port", help="", default=8083)
   parser.add_option("-U", "--user", dest="user", help="connection user", default=None)
   parser.add_option("-W", "--password", dest="password", help="user password", default=None)
   (options, args) = parser.parse_args()

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
      "user":options.user }

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
         sessionid=_session["Mea-SessionId"]
         mysession["sessionid"]=sessionid
#         mysession["sessiondate"]=datetime.now().strftime("%d/%m/%Y %H:%M:%S")
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
