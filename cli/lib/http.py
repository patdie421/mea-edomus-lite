import base64
import json
import urllib2

def auth_str(user, pw):
    """
    Arguments:
        user - (Required)
        pw - (Required)
    """
    auth = base64.b64encode('%s:%s' % (user, pw))
    return auth


def delete(url, headers={}):
    method = "DELETE"
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
    except:
       pass
    return connection.code,data


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
    except:
       pass
    return connection.code,data


def put(url, body, headers={}):
    method = "PUT"
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
    except:
       pass
    return connection.code,data
