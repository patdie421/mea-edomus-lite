import sys
import json
import getpass

from optparse import OptionParser

from lib import http
from lib import session
from lib import display


def get_services(host, port, sessionid):
   url="http://"+str(host)+":"+str(port)+"/rest/service"
   headers={"Mea-session": sessionid}
   code, res=http.get(url, headers)
   res=json.loads(res)
   return code, res


def get_service(host, port, sessionid, service):
   url="http://"+str(host)+":"+str(port)+"/rest/service/"+str(service)
   headers={"Mea-session": sessionid}
   code, res=http.get(url, headers)
   res=json.loads(res)
   return code, res


def _get(host, port, sessionid, options, args):
   result=None
   if len(args)==0:
      code,result=get_services(host, port, sessionid)
   elif len(args)==1:
      code,result=get_service(host, port, sessionid, args[0])
   else:
      display.error("too many parameters")
      return False
   display.formated(result, options.format)
   return True


actions={}
actions["get"]=_get

def do(host, port, sessionid, args):
   try:
      action=args.pop(0).lower()
   except:
      action=False

   usage = "\
usage: %prog user service [<pid>] [options]"

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

   parser.print_help()
   return False
