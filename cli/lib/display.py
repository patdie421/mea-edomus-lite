import json
import sys

def format():
   return ["text", "json"]


def error(msg, errtype=None, errno=None):
   _msg=""
   if errtype:
      _msg=str(errtype).upper()+": "+str(msg)
   else:
      _msg=str(msg)
   if errno:
      _msg=_msg+"\n(errno: "+str(errno)+")"
   sys.stderr.write("\n"+_msg+"\n\n")


def formated(result, f):
   if f=="json":
      print json.dumps(result, indent=3)

   elif format=="text":
      print result

   else:
      print json.dumps(result)
