import base64
import json
from lib import http

def check(host, port, sessionid):
    url="http://"+str(host)+":"+str(port)+"/rest/session"
    headers={"Mea-session": sessionid}
    code,res=http.get(url,headers)
    if code==200:
       return True 
    else:
       return False 

def close(host, port, sessionid):
    url="http://"+str(host)+":"+str(port)+"/rest/session"
    headers={"Mea-session": sessionid}
    code,res=http.delete(url,headers)
    return code, res

def open(host, port, user, password):
    url="http://"+str(host)+":"+str(port)+"/rest/session"
    body={ "user":user, "password":password }
    code,res=http.post(url, body)
    return code, res
