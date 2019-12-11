import sys
import json
import getpass

from optparse import OptionParser

from lib import http
from lib import session
from lib import display


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


def rollback_users(host, port, sessionid):
   url="http://"+str(host)+":"+str(port)+"/rest/user"
   headers={"Mea-session": sessionid}
   _user={"action":"rollback"}
   code, res=http.post(url, _user, headers)
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


def _commit(host, port, sessionid, options, args):
   if len(args)==0:
      code,result=commit_users(host, port, sessionid)
   else:
      display.error("too many parameters")
      return False
   display.formated(result, options.format)
   return True


def _rollback(host, port, sessionid, options, args):
   if len(args)==0:
      code,result=rollback_users(host, port, sessionid)
   else:
      display.error("too many parameters")
      return False
   display.formated(result, options.format)
   return True


def _delete(host, port, sessionid, options, args):
   if len(args)==1:
      code,result=delete_user(host, port, sessionid, args[0])
   else:
      display.error("user not provided")
      return False
   display.formated(result, options.format)
   return True


def _get(host, port, sessionid, options, args):
   result=None
   if len(args)==0:
      code,result=get_users(host, port, sessionid)
   elif len(args)==1:
      code,result=get_user(host, port, sessionid, args[0])
   else:
      display.error("too many parameters")
      return False
   display.formated(result, options.format)
   return True


def _password(host, port, sessionid, options, args):
   j={}
   possible=["oldpassword", "newpassword"]
   for i in args:
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
   display.formated(result, options.format)
   return True

   display.error("parameter error")
   return False

 
def _update(host, port, sessionid, options, args):
   username=args.pop(0)
   j={}
   possible=["password", "fullname", "profile"]

   for i in args:
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
      user={}
      if "fullname" in j:
         user["fullname"]=j["fullname"]
      if "password" in j:
         user["password"]=j["password"]
      if "profile" in j:
         user["profile"]=int(j["profile"])

      code,result= update_user(host, port, sessionid, username, user)
      display.formated(result, options.format)
      return True
   else:
      display.error("no parameters")
      return False

   display.error("unknown error")
   return False


def _create(host, port, sessionid, options, args):
   username=args.pop(0)
   j={}
   possible=["password", "fullname", "profile"]
   for i in args:
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
      display.formated(result, options.format)
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

def do(host, port, sessionid, args):
   try:
      action=args.pop(0).lower()
   except:
      action=False

   usage = "\
usage: %prog user get     [<username>] [options]\n\
       %prog user delete   <username>  [options]\n\
       %prog user add      <username>  password:<password>   [profile:<profileid>] [fullname:\"full user name\"] [options]\n\
       %prog user update   <username>  [password:<password>] [profile:<profileid>] [fullname:\"full user name\"] [options]\n\
       %prog user password <username>  [password:<password>] [options]\n\
       %prog user commit   [options]\n\
       %prog user rollback [options]\n"

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
