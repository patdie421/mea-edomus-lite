import base64
import json
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
   except urllib.error.HTTPError as e:
      # Return code error (e.g. 404, 501, ...)
      httpCode=e.code
      httpData=e.read()
   except urllib.error.URLError as e:
      # Not an HTTP-specific error (e.g. connection refused)
      httpCode=-1
      httpData=e.reason

   if httpCode==None:
      httpData=response.read()
      httpCode=response.code

   return httpCode, httpData


def get(url, headers={}):
   return _httpRequest(url, headers, method='GET')
   
def delete(url, headers={}):
   return _httpRequest(url, headers, method='DELETE')

def post(url, body={}, headers={}):
   return _httpRequest(url, headers, method='POST', body=body)

def put(url, body={}, headers={}):
   return _httpRequest(url, headers, method='PUT', body=body)
