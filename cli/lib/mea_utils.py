import json
from lib import http


def PostUrl(url,sessionid,body):
   headers={"Mea-session": sessionid}
   code, res=http.post(url, body, headers)
   res=json.loads(res)
   return code, res


def PutUrl(url,sessionid,body):
   headers={"Mea-session": sessionid}
   code, res=http.put(url, body, headers)
   res=json.loads(res)
   return code, res


def GetUrl(url,sessionid):
   headers={"Mea-session": sessionid}
   code, res=http.get(url, headers)
   res=json.loads(res)
   return code, res


def DeleteUrl(url,sessionid):
   headers={"Mea-session": sessionid}
   code, res=http.delete(url, headers)
   res=json.loads(res)
   return code, res

