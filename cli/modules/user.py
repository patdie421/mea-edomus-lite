import sys
import json
import getpass
import argparse

from lib import http
from lib import session
from lib import display

from dateutil.parser import parse


def get_users(host, port, sessionid):
   url="http://"+str(host)+":"+str(port)+"/rest/user"
   headers={"Mea-session": sessionid}
   code, res=http.get(url, headers)
   res=json.loads(res)
   return code, res


def get_user(host, port, sessionid, username):
   url="http://"+str(host)+":"+str(port)+"/rest/user/"+str(username)
   headers={"Mea-session": sessionid}
   code, res=http.get(url, headers)
   res=json.loads(res)
   return code, res


def delete_user(host, port, sessionid, username):
   url="http://"+str(host)+":"+str(port)+"/rest/user/"+str(username)
   headers={"Mea-session": sessionid}
   code, res=http.delete(url, headers)
   res=json.loads(res)
   return code,res


def create_user(host, port, sessionid, user):
   url="http://"+str(host)+":"+str(port)+"/rest/user"
   headers={"Mea-session": sessionid}
   _user={"action":"create","parameters":user}
   code, res=http.post(url, _user, headers)
   res=json.loads(res)
   return code, res


def commit_users(host, port, sessionid):
   url="http://"+str(host)+":"+str(port)+"/rest/user"
   headers={"Mea-session": sessionid}
   _user={"action":"commit"}
   code, res=http.post(url, _user, headers)
   res=json.loads(res)
   return code, res


def rollback_users(host, port, sessionid, timestamp=None):
   url="http://"+str(host)+":"+str(port)+"/rest/user"
   headers={"Mea-session": sessionid}
   body={"action":"rollback"}
   if timestamp!=None:
      body["parameters"]={"timestamp":timestamp}
   code, res=http.post(url, body, headers)
   res=json.loads(res)
   return code, res


def update_user(host, port, sessionid, username, user):
   url="http://"+str(host)+":"+str(port)+"/rest/user/"+str(username)
   headers={"Mea-session": sessionid}
   _user={"action":"update", "parameters":user}
   code, res=http.put(url, _user, headers)
   res=json.loads(res)
   return code, res


def update_password(host, port, sessionid, oldpassword, newpassword):
   url="http://"+str(host)+":"+str(port)+"/rest/user"
   headers={"Mea-session": sessionid}
   passwords={"action": "password", "parameters": { "oldpassword":str(oldpassword), "newpassword":str(newpassword)}}
   code, res=http.put(url, passwords, headers)
   res=json.loads(res)
   return code, res


def _commit(host, port, sessionid, args, _args):
   if args.user!=None:
      display.error("too many parameters")
      return False
   code,result=commit_users(host, port, sessionid)
   display.formated(result, args.format)
   return True


def _rollback(host, port, sessionid, args, _args):
   if args.user==None:
      code,result=rollback_users(host, port, sessionid)
   elif len(args.keyvalue)==0:
      date_str=args.user
      try:
         _datetime = parse(date_str)
      except:
         display.error("bad date format: "+date_str)
         return False
      timestamp=_datetime.strftime('%s')

      code,result=rollback_users(host, port, sessionid, timestamp)
   else:
      display.error("too many parameters")
      return False
   display.formated(result, args.format)
   return True


def _delete(host, port, sessionid, args, _args):
   if len(args.keyvalue)>0:
      display.error("too many parameters")
      return False
   code,result=delete_user(host, port, sessionid, args.user)
   display.formated(result, args.format)
   return True


def _get(host, port, sessionid, args, _args):
   result=None
   if args.user==None:
      code,result=get_users(host, port, sessionid)
   else:
      code,result=get_user(host, port, sessionid, args.user)
   display.formated(result, args.format)
   return True


def _password(host, port, sessionid, args, _args):
   j={}
   possible=["oldpassword", "newpassword"]
   for i in args.keyvalue:
      s=i.split(':')
      if len(s) != 2:
         display.error("property syntax error: "+str(i))
         return False

      v=s[0].strip().lower()
      if not v in possible:
         display.error("unknown property: "+str(v))
         return False
      j[s[0].strip().lower()]=v

   if "oldpassword" in j:
      oldpassword=j["oldpassword"]
   else:
      oldpassword=getpass.getpass("old password:")

   if "newpassword" in j:
      newpassword=j["newpassword"]
   else:
      newpassword=getpass.getpass("new password:")

   code,result= update_password(host, port, sessionid, oldpassword, newpassword)
   display.formated(result, args.format)
   return True

 
def _update(host, port, sessionid, args, _args):
   username=args.user
   j={}
   possible=["password", "fullname", "profile"]

   for i in args.keyvalue:
      s=str(i).split(':')
      if len(s) != 2:
         display.error("property syntax error - "+str(i))
         return False

      v=s[0].strip().lower()
      if not v in possible:
         display.error("unknown property - "+str(v))
         return False

      j[s[0].strip().lower()]=s[1].strip()

   if len(j)>0:
      user={}
      if "fullname" in j:
         user["fullname"]=j["fullname"]
      if "password" in j:
         user["password"]=j["password"]
      if "profile" in j:
         user["profile"]=int(j["profile"])

      code,result= update_user(host, port, sessionid, username, user)
      display.formated(result, args.format)
      return True
   else:
      display.error("no parameters")
      return False

   display.error("sorry, unknown error")
   return False


def _create(host, port, sessionid, args, _args):
   username=args.user
   j={}
   possible=["password", "fullname", "profile"]
   for i in args.keyvalue:
      s=str(i).split(':')
      if len(s) != 2:
         display.error("property syntax error: "+str(i))
         return False
      v=s[0].strip().lower()
      if not v in possible:
         display.error("unknown property: "+str(v))
         return False

      j[s[0].strip().lower()]=s[1].strip()

   if len(j)>0:
      if not "password" in j:
         display.error("password is mandatory")
         return False

      if not "profile" in j:
         profile=0
      else:
         profile=int(j["profile"])

      if not "fullname" in j:
         fullname=""
      else:
         fullname=j["fullname"]

      user={"username":username, "password":j["password"], "fullname":fullname, "profile":profile}
      code,result= create_user(host, port, sessionid, user)
      display.formated(result, args.format)
      return True
   else:
      display.error("no parameters")
      return False

   display.error("unknown error")
   return False

actions={}
actions["get"]=_get
actions["delete"]=_delete
actions["add"]=_create
actions["update"]=_update
actions["password"]=_password
actions["commit"]=_commit
actions["rollback"]=_rollback


cli_epilog='''
Actions and options:
   get      [<username>]
   delete   <username>  [options]
   add      <username>   password:<password>  [profile:<profileid>] [fullname:\"full user name\"]
   update   <username>  [password:<password>] [profile:<profileid>] [fullname:\"full user name\"]
   password <username>  [oldpassword:<oldpassword>] [newpassword:<newpassword>]
   commit
   rollback [date]
'''

cli_description='''
CLI users managment
'''

def parser(args_subparser, parent_parser):
   user_parser=args_subparser.add_parser('user', parents=[parent_parser], add_help=False, description=cli_description, epilog=cli_epilog, formatter_class=argparse.RawDescriptionHelpFormatter)
   user_parser.add_argument("action", help="For more details see below.", choices=['get','add','update','delete','password','commit','rollback'])
   user_parser.add_argument("user", metavar='name|date', help="user name or date (for commit)", nargs="?")
   user_parser.add_argument("keyvalue", metavar='key:value', help="<key>/<value> pairs for action. For more details see below. ", nargs="*")
   return user_parser


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
