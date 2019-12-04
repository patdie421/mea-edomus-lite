import base64
import json
import pprint
import urllib2


def auth_str(user, pw):
    """
    Arguments:
        user - (Required)
        pw - (Required)
    """
    auth = base64.b64encode('%s:%s' % (user, pw))
    return auth


def get(url, headers={}):
    method = "GET"
    handler = urllib2.HTTPHandler()
    opener = urllib2.build_opener(handler)
    r = urllib2.Request(url)
    r.add_header("Content-Type",'application/json')
    for i in headers:
       r.add_header(i,headers[i])
    r.get_method = lambda: method
    try:
       connection = opener.open(r)
    except urllib2.HTTPError,e:
       connection = e

    if connection.code == 200:
       data = connection.read()
       return connection.code,data
    else:
       return connection.code,None


def post(url, body, headers={}):
    method = "POST"
    handler = urllib2.HTTPHandler()
    opener = urllib2.build_opener(handler)
    data = json.dumps(body) 
    r = urllib2.Request(url, data=data)
    r.add_header("Content-Type",'application/json')
    for i in headers:
       r.add_header(i,headers[i])
    r.get_method = lambda: method
    try:
       connection = opener.open(r)
    except urllib2.HTTPError,e:
       connection = e

    if connection.code == 200:
       data = connection.read()
       return connection.code,data
    else:
       return connection.code,None


def post_pairing(host, port, body, sessionid):
    url="http://"+str(host)+":"+str(port)+"/rest/pairing"
    headers={"Mea-session": sessionid}
    code,res=post(url, body, headers)
    return code, res


def get_pairing(host, port, sessionid):
    url="http://"+str(host)+":"+str(port)+"/rest/pairing"
    headers={"Mea-session": sessionid}
    code,res=get(url, headers)
    return code, res


def get_interfaces(host, port, sessionid):
    url="http://"+str(host)+":"+str(port)+"/rest/interface"
    headers={"Mea-session": sessionid}
    code,res=get(url, headers)
    return code, res


def open_session(host, port, user, password):
    url="http://"+str(host)+":"+str(port)+"/rest/session"
    body={ "user":user, "password":password }
    code,res=post(url, body)
    return code, res

    
host="localhost"
port=8083
user="admin"
password="admin"

code,session=open_session(host, port, user, password)
session=json.loads(session)
sessionid=session["Mea-SessionId"]

code,interfaces=get_interfaces(host, port, sessionid)
if interfaces!=None:
   interfaces=json.loads(interfaces)
   print json.dumps(interfaces, indent=4)

code,pairing=get_pairing(host, port, sessionid)
if pairing!=None:
   print pairing

body={}
code,pairing=post_pairing(host, port, body, sessionid)
if pairing!=None:
   print pairing

