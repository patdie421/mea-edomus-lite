import base64
import json
import sys

def auth_str(user, pw):
    auth = base64.b64encode('%s:%s' % (user, pw))
    return auth

if sys.version_info[0] < 3:

   import urllib2
   
   def _httpRequest(url, method, body, headers={}):
      httpCode=None
      httpData=None
      
      method=method.upper()
      if method in ['GET', 'DELETE']:
         body=None

      handler = urllib2.HTTPHandler()
      opener = urllib2.build_opener(handler)
      if body!=None:
         data = json.dumps(body) 
         r = urllib2.Request(url, data=data)
      else:
         r = urllib2.Request(url)
      r.add_header("Content-Type",'application/json')
      for i in headers:
         r.add_header(i,headers[i])
      r.get_method = lambda: method
      try:
         connection = opener.open(r)
         httpData = connection.read()
         httpCode = connection.code
      except urllib2.HTTPError as e:
         httpCode = e.code
         httpData = e.read()
      except urllib2.URLError as e:
         httpCode=-1
         httpData=e.reason

      return httpCode,httpData
   
  
   def post(url, body, headers={}):
      return _httpRequest(url, 'POST', body, headers)
 
   def put(url, body, headers={}):
      return _httpRequest(url, 'PUT', body, headers)
 
   def get(url, headers={}):
      return _httpRequest(url, 'GET', None, headers)

   def delete(url, headers={}):
      return _httpRequest(url, 'DELETE', None, headers)

else:
   
   import urllib.request
   import urllib.error

   def _httpRequest(url, headers={}, method='GET', body=None):
      httpCode=None
      httpData=""
      
      method=method.upper()
      if method in ['GET', 'DELETE']:
         body=None
   
      _headers={}
      for i in headers:
         _headers[str(i)]=str(headers[i])      
      headers["Content-Type"]='application/json'
      try:
         if body==None:
            request = urllib.request.Request(url, headers=_headers, method=method)
         else:
            data=bytes(json.dumps(body),'utf-8')
            request = urllib.request.Request(url, headers=headers, method=method, data=data)         
         response = urllib.request.urlopen(request)
         httpData=response.read()
         httpCode=response.code

      except urllib.error.HTTPError as e:
         # Return code error (e.g. 404, 501, ...)
         httpCode=e.code
         httpData=e.read()

      except urllib.error.URLError as e:
         # Not an HTTP-specific error (e.g. connection refused)
         httpCode=-1
         httpData=e.reason
   
      return httpCode, httpData
   
   
   def get(url, headers={}):
      return _httpRequest(url, headers, method='GET')
      
   def delete(url, headers={}):
      return _httpRequest(url, headers, method='DELETE')
   
   def post(url, body={}, headers={}):
      return _httpRequest(url, headers, method='POST', body=body)
   
   def put(url, body={}, headers={}):
      return _httpRequest(url, headers, method='PUT', body=body)
