import sys
import json

from optparse import OptionParser
from lib import http
from lib import session
from lib import display
from modules import interface


def get_pairing_interfaces_status(host, port, sessionid):
    url="http://"+str(host)+":"+str(port)+"/rest/pairing"
    headers={"Mea-session": sessionid}
    code, res=http.get(url, headers)
    res=json.loads(res)
    return code, res


def set_pairing_interface(host, port, sessionid, name, state):
    url="http://"+str(host)+":"+str(port)+"/rest/pairing/"+str(name)
    body={ "state": state }
    headers={"Mea-session": sessionid}
    code,res=http.post(url, body, headers)
    res=json.loads(res)
    return code, res


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
   if len(args)==1:
      code, result=set_pairing_interface(host, port, sessionid, args[0], 0)
      display.formated(result, options.format)
      if(code==200):
         return True
      else:
         return False
   else:
      display.error("parameter error")
      return False

 
def _start(host, port, sessionid, options, args):
   if len(args)==1:
      code, result=set_pairing_interface(host, port, sessionid, args[0], 1)
      display.formated(result, options.format)
      if(code==200):
         return True
      else:
         return False
   else:
      display.error("parameter error")
      return False


def _status(host, port, sessionid, options, args):
   code,result=get_pairing_interfaces_status(host, port, sessionid)
   if len(args)==0:
      display.formated(result,options.format)
      return True
   elif len(args)==1:
      if args[0].upper() in result:
         display.formated(result[args[0].upper()], options.format)
         return True
      display.formated(None, options.format)
      return True
   else:
      return False
 
def _pairable(host, port, sessionid, options, args):
   _result=None
   if len(args)==0:
      code, result=get_pairable_interfaces(host, port, sessionid)
   elif len(args)==1:
      code, result=get_pairable_interface(host, port, sessionid, args[0])
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

   usage = "usage: %prog pairing action [object] [options]"
   parser = OptionParser(usage)
   parser.add_option("-f", "--format", dest="format", help="ouput format : [json|text]", default="json")
   (options, args) = parser.parse_args(args=args)

   if not action in ["get", "start", "stop", "status"]:
      parser.print_help()
      return False

   if action=="get":
      return _pairable(host, port, sessionid, options, args)
   elif action=="status":
      return _status(host, port, sessionid, options, args)
   elif action=="start":
     return _start(host, port, sessionid, options, args)
   elif action=="stop":
     return _stop(host, port, sessionid, options, args)
