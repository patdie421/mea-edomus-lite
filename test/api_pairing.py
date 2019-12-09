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

    data=None
    try:
       data = connection.read()
#       print "data:", data
    except:
       pass
    return connection.code,data


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

    data=None
    try:
       data = connection.read()
#       print "data:", data
    except:
       pass
    return connection.code,data


def post_pairing(host, port, name, state, sessionid):
    url="http://"+str(host)+":"+str(port)+"/rest/pairing/"+str(name)
    body={ "state": state }
    headers={"Mea-session": sessionid}
    code,res=post(url, body, headers)
    return code, res


def get_pairing(host, port, sessionid):
    url="http://"+str(host)+":"+str(port)+"/rest/pairing"
    headers={"Mea-session": sessionid}
    code,res=get(url, headers)
    return code, res


def get_pairing_interface(host, port, name, sessionid):
    url="http://"+str(host)+":"+str(port)+"/rest/pairing/"+str(name)
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

#code,interfaces=get_interfaces(host, port, sessionid)
#if interfaces!=None:
#   interfaces=json.loads(interfaces)
#   print json.dumps(interfaces, indent=4)

code,pairing=get_pairing(host, port, sessionid)
if pairing!=None:
   print code, pairing

print "I010 ?"
code,pairing=get_pairing_interface(host, port, "I010", sessionid)
print code, pairing

print "TOTO ?"
code,pairing=get_pairing_interface(host, port, "TOTO", sessionid)
print code, pairing

print "I010 ON"
code,pairing=post_pairing(host, port, "I010", 1, sessionid)
if pairing!=None:
   print code, pairing

print "I010 ?"
code,pairing=get_pairing(host, port, sessionid)
if pairing!=None:
   print code, pairing

# print "I010 OFF"
# code,pairing=post_pairing(host, port, "I010", 0, sessionid)
# if pairing!=None:
#   print code, pairing

# print "I010 ?"
# code,pairing=get_pairing(host, port, sessionid)
# if pairing!=None:
#    print code, pairing

print "TOTO ON"
code,pairing=post_pairing(host, port, "TOTO", 1, sessionid)
if pairing!=None:
   print code, pairing
