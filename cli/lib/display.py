import json

def format():
   return ["text", "json"]


def error(msg):
   print msg


def formated(result, f):
   if f=="json":
      print json.dumps(result, indent=3)

   elif format=="text":
      print result

   else:
      print json.dumps(result)
