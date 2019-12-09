import sys
import json

from optparse import OptionParser

from lib import http
from lib import session
from lib import display


def get_interfaces(host, port, sessionid):
   url="http://"+str(host)+":"+str(port)+"/rest/interface"
   headers={"Mea-session": sessionid}
   code, res=http.get(url, headers)
   res=json.loads(res)
   return code, res


def get_interface(host, port, sessionid, interface):
   url="http://"+str(host)+":"+str(port)+"/rest/interface/"+str(interface)
   headers={"Mea-session": sessionid}
   code, res=http.get(url, headers)
   res=json.loads(res)
   return code, res


def _get(host, port, sessionid, options, args):
   result=None
   if len(args)==0:
      code,result=get_interfaces(host, port, sessionid)
   elif len(args)==1:
      code,result=get_interface(host, port, sessionid, args[0])
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

   usage = "usage: %prog interface <action> [object] [options]"
   parser = OptionParser(usage)
   parser.add_option("-f", "--format", dest="format", help="ouput format : [json|txt]", default="json")
   (options, args) = parser.parse_args(args=args)

   if action==False:
      parser.print_help()
      return False

   if action in ["get" ]:
      if action=="get":
         if _get(host, port, sessionid, options, args)==False:
            parser.print_help()
            return False
         else:
            return True
   else:
      parser.print_help()
      return False
