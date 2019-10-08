var http = require('http');
var https = require('https');
var Xpl = require("xpl-api");
var commander = require('commander');
var crypto = require('crypto');
var url = require("url");
// var async = require('async');

const contentType={'content-type':'application/json'}
const defaultTimeOut=5000
const HTTP_PORT=7101

commander.option("--networkInterface <name>", "Specify the network interface name");
commander.option("--hubPingDelaySecond <sec>", "");
commander.option("--xplSource <name>", "Specify the source in XPL message");
commander.option("--xplTarget <name>", "Specify the target in XPL message");
commander.option("--xplLog", "Verbose XPL layer");

commander.parse(process.argv);

var xpl = new Xpl(commander);

/*
var msgHistoryDb={}
setInterval(manageMsgHistoryDb, 5*60*1000);
*/
var responses_queue=[]
setInterval(manageResponsesQueue, 100);
var callbacks_queue={}
setInterval(manageCallbacksQueue, 5*60*1000);


var server = http.createServer(requestHandler);

server.listen(HTTP_PORT, function() {
      console.log("Server listening on port #"+HTTP_PORT);
   }
);


/*
{
   "body": {"device": "test1", "request": "current"},
   "target":"*",
   "schema":"sensor.basic",
   "result": {
   	  "values": ["current"],
      "conditions" : { "device": "^test1$", "schema": "^sensor\\..*$", "source":"^nodejs\\.sldcfrdxc090-.*$", "type":"^.*-stat$" },
      "timeout": "500"
    }
}
*/

function isArray(a) {
    return (!!a) && (a.constructor === Array);
}


function manageCallbacksQueue() {
   for(var i in callbacks_queue) {

      if(callbacks_queue[i].errorCounter>3) {
         delete callbacks_queue[i];
         continue;
      }

      if(callbacks_queue[i].counter==0) {
         delete callbacks_queue[i];
         continue;
      }
   }
}


function manageResponsesQueue() {
   var i = responses_queue.length
   var now=Date.now()
   while (i--) {
      if((now - responses_queue[i].timestamp) > responses_queue[i].timeout) {
         responses_queue[i].response.writeHead(200, contentType);
         responses_queue[i].response.end('{"status":"timeout","errorcode":5}');
         responses_queue.splice(i,1)
      }
   }
}


function checkXplRequestData(xplRequestData) {

   if(typeof(xplRequestData.body) == "object") {
      for(var i in xplRequestData.body) {
         if(typeof(xplRequestData.body[i]) != "string") {
            return "body data error";
         }
      }
   }
   else {
      return "body is not a dictionary";
   }
   if(xplRequestData.target && typeof(xplRequestData.target) != "string") {
      return "target is not a string";
   }
   if(xplRequestData.schema && typeof(xplRequestData.schema) != "string") {
      return "schema is not a string";
   }
   if(xplRequestData.result && typeof(xplRequestData.result) == "object") {
      if(xplRequestData.result.values) {
         if(typeof(xplRequestData.result.values) != "object") {
            return "result.values is not an array";
         }
         if(!isArray(xplRequestData.result.values)) {
            return "result.values is not an array";
         }
         for(var i in xplRequestData.result.values) {
            if(typeof(xplRequestData.result.values[i]) != "string") {
               return "result.values data error";
            }
         }
      }
      if(xplRequestData.result.conditions) {
         if(typeof(xplRequestData.result.conditions)!="object") {
           return "result.conditions is not a dictionary";
         }
         for(var i in xplRequestData.result.conditions) {
            if(typeof(i)!="string" || typeof(xplRequestData.result.conditions[i])!="string") {
               return "result.conditions data error";
            }
         }
      }

      if(xplRequestData.result.timeout && isNaN(xplRequestData.result.timeout)) {
         return "result.timeout is not a number";
      }
   }

   return true;
}


function doRequest(surl, request, response, data) {
   var method = request.method;

   if(surl.length != 0) {
      response.writeHead(400);
      response.end();
      return -1;
   }

   var xplRequestData=data;
   var ret=checkXplRequestData(xplRequestData);
   if(ret!==true) {
      response.writeHead(400, contentType);
      response.end('{"status":"error","errorcode":6, "message":"'+ret+'"}');
      return -1;
   }
   var timeout=defaultTimeOut;
   switch(method) {
      case 'POST':
         var tsp=Date.now()
         // verifier xplRequestData ici
         if(!xplRequestData.schema || typeof(xplRequestData.schema) !== "string") {
            xplRequestData["schema"]="sensor.request";
         }

         if(!xplRequestData.target || typeof(xplRequestData.target) !== "string") {
            xplRequestData["target"]="*";
         }

         if(!isNaN(xplRequestData.result.timeout)) {
            timeout=xplRequestData.result.timeout;
         }

         responses_queue.push({xplRequestData: xplRequestData, timestamp: tsp, response: response, timeout: timeout});
         xpl.sendXplCmnd(xplRequestData.body, xplRequestData["schema"], xplRequestData["target"]);

         return 0;

      default:
         response.writeHead(400);
         response.end();
         break;
   }

   return -1;
}


function doSendMessage(op, surl, request, response, data) {

   if(surl.length != 0) {
      response.writeHead(400);
      response.end();
      return -1;
   }

   if(request.method != 'POST') {
      response.writeHead(400);
      response.end();
      return -1;
   }

   var xplRequestData=data;

   var ret=checkXplRequestData(xplRequestData);
   if(ret!==true) {
      response.writeHead(400, contentType);
      response.end('{"status":"data error","errorcode":7, "message":"'+ret+'"}');
      return -1;
   }

   if(!xplRequestData.schema || typeof(xplRequestData.schema) !== "string") {
      xplRequestData["schema"]="sensor.request";
   }

   if(!xplRequestData.target || typeof(xplRequestData.target) !== "string") {
      xplRequestData["target"]="*";
   }

   console.log(op, xplRequestData.body, xplRequestData["schema"], xplRequestData["target"])
   switch(op) {
      case "cmnd": 
         xpl.sendXplCmnd(xplRequestData.body, xplRequestData["schema"], xplRequestData["target"]); 
         break;
      case "stat": 
         xpl.sendXplStat(xplRequestData.body, xplRequestData["schema"], xplRequestData["target"]); 
         break;
      case "trig": 
         xpl.sendXplTrig(xplRequestData.body, xplRequestData["schema"], xplRequestData["target"]); 
         break;
   }

   response.writeHead(200, contentType);
   response.end('{"status":"OK"}');
   return 0;
}


function deleteCallback(id) {
   var callback=callbacks_queue[id];
   if(callback==undefined) {
      return {status: "error", errorcode: 8, message: "no valid id"};
   }
   delete callbacks_queue[id];
   return {status: "OK"};
}


function returnCallback(id) {
   var callback=callbacks_queue[id];
   if(callback==undefined) {
      return {status: "error", errorcode: 8, message: "no valid id"};
   }

   return callback
}


function updateCallback(id, data) {
   var _data=JSON.parse(data);
   if(callbacks_queue[id]==undefined) {
      return {status: "error", errorcode: 8, message: "no valid id"};
   }

   if(_data.counter!=undefined) {
      if (isNaN(_data.counter) || _data.counter<0 || _data.counter>1000 ) {
         return {status: "error", errorcode: 9, message: "invalid counter value"};
      }
      else {
         callbacks_queue[id].counter=_data.counter;         
      } 
   }
   else {
      return {status: "error", errorcode: 9, message: "counter value"};
   }


   return {status: "OK"}
}


function createCallback(data) {
   var id=crypto.randomBytes(16).toString("hex");
   var now=Date.now()
   var _url=data.url;

   if(_url==undefined) {
      return {status: "error", errorcode: 10, message: "no valid url"};
   }

   var options = url.parse(_url);
   if(options.protocol!='http:' && options.protocol!='https:') {
      return {status: "error", errorcode: 10, message: "no valid url"}
   }

   for(var i in callbacks_queue) {
      if(callbacks_queue[i].url==_url) {
         return {status: "error", errorcode: 1, callbackId: i, message: "callback already defined for url: "+_url};
      }
   }

   var filter={}
   if(data.filter!=undefined) {
      filter=data.filter;
   }
   var timeout=5000;
   if(data.timeout!=undefined && !isNaN(data.timeout)) {
     timeout=data.timeout+0; 
   }
   var counter=1; 
   if(data.counter!=undefined && !isNaN(data.counter)) {
      counter=data.counter;
   }

   if(counter<0)
      counter=1;
   if(counter>1000)
      counter=1000;
   counter=Math.floor(counter);

   var callback={}
   callback.id=id;
   callback.url=_url;
   callback.created=now;
   callback.filter=filter;
   callback.timeout=timeout;
   callback.counter=counter;
   callback.errorCounter=0;
   callback.status="OK"

   callbacks_queue[id]=callback;

   return callback;
}


function doCallback(surl, request, response, data) {
   var method = request.method;

   switch(method) {
      case "POST":
         if(surl.length!=1) {
            response.writeHead(400, contentType);
            response.end('{"status":"error","errorcode":2, "message":"request error"}');
         }
         else {
            var rep = updateCallback(surl[0], data);
            if(rep.status!="error")
               response.writeHead(200, contentType);
            else
               response.writeHead(400, contentType);
            response.end(JSON.stringify(rep));
         }         
         break;
      case "PUT":
         if(surl.length!=0) {
            response.writeHead(400, contentType);
            response.end('{"status":"error","errorcode":2, "message":"request error"}');
         }
         else {
            var rep = createCallback(data);
            if(rep.status!="error")
               response.writeHead(200, contentType);
            else
               response.writeHead(400, contentType);
            response.end(JSON.stringify(rep));
         }
         break;
      case "GET":
         if(surl.length==0) {
            response.writeHead(200, contentType);
            response.end(JSON.stringify(callbacks_queue));
            break;
         }
         break;
      case "DELETE":
         if(surl.length!=1) {
            response.writeHead(400, contentType);
            response.end('{"status":"error","errorcode":2, "message":"request error"}');
         }
         else {
            if(method=="GET")
               rep=returnCallback(surl[0]);
            else {
               rep=deleteCallback(surl[0]);
            }

            if(rep.status!="error")
               response.writeHead(200, contentType);
            else
               response.writeHead(400, contentType);
            response.end(JSON.stringify(rep));
         }
         break;
      default:
            response.writeHead(400);
            response.end();
         break;
   }
}


function doXpl(surl, request, response, data)
{
   if(surl.length==0) {
      response.writeHead(200);
      response.end("TODO");
   }
   else {
      var op=surl[0];
      surl.splice(0,1)

      switch(op) {
         case "request":
            doRequest(surl, request, response, data);
            break;
         case "cmnd":
         case "stat":
         case "trig":
            doSendMessage(op, surl, request, response, data);
            break;
         case "callback":
            doCallback(surl, request, response, data);
            break;
         default:
            response.writeHead(400);
            response.end();
            break;
      }
   }
   return 0;
}


function requestHandler(request, response)
{
   console.log( "request informations: " +
               " method=" + request.method +
               " url=" + request.url +
               " content-type=" + request.headers['content-type'] +
               " authorization=" + request.headers['authorization']);
  var surl = request.url.toLowerCase().split('/');
  var data=''

  request.on("readable", function() {
     var _data=request.read();
     if(_data)
        data+=_data.toString('utf8');
  });

   request.on('end', function() {
      try {
         var selector=surl[1]
         try {
            data=JSON.parse(data);
         }
         catch(e) {
            data=undefined
         }
         surl.splice(0,2)
         switch(selector) {
            case "xpl":
               doXpl(surl, request, response, data)
               break;
            case "test":
               response.writeHead(200, contentType);
               response.end(JSON.stringify(data));
               break;
            default:
               response.writeHead(404);
               response.end();
               break;
         }
      }
      catch(e) {
         console.log(e);
         response.writeHead(400);
         response.end();
      }
  });
}


xpl.on("close", () => {
  process.exit(0);
});


xpl.on("message", xplMessage);


xpl.bind( xplIsInit );


function xplIsInit() {
   console.log("xpl is initialized");
}

/*
function manageMsgHistoryDb() {
   var toDelete=[]
   var now=Date.now()
   for(var i in msgHistoryDb) {
      var j = msgHistoryDb[i].length
      while (j--) {
         if((now - msgHistoryDb[i][j].timestamp) > 1000*60*60) {
            msgHistory[i].slice(j,1);
         }
         else
            break;
      }
      if(msgHistoryDb[i].length == 0) {
         toDelete.push(i);
      }
   }
   for(var i in toDelete) {
      delete msgHistoryDb[i]
   } 
}
*/

function checkFilter(cond, message) {
   var _cond;
   for(var j in cond) {
      _cond=false;
      var regex=false;
      try {
         regex=new RegExp(cond[j])
      }
      catch(e) {
         return [true, 400, {status:"error", errorcode: 4, message: "Regex error: "+cond[j]}];
      }

      if(j=="source") {
         if(message.header.source.match(regex)) {
            _cond=true;
         }
      }
      else if(j=="schema") {
         if(message.bodyName.match(regex)) {
            _cond=true;
         }
      }
      else if(j=="type") {
         if(message.headerName.match(regex)) {
            _cond=true;
         }
      }
      else if(j in message.body) {
         if(message.body[j].match(regex)) {
            _cond=true;
         }
      }

      if(_cond==false)
         break;
   }
   if(_cond==true) {
      return [true, 200];
   }
   else {
      return [false];
   }
}


function httpCall(callback, message) {
   var _http;
   var options = url.parse(callback.url);
   options.method="POST";
   switch(options.protocol) {
      case 'http:': _http=http; break;
      case 'https:': _http=https; break;
      default: callback.errorCounter=999; return;
   }

   var req=_http.request(options, (response) => {
      var str='';
      response.on('data', function(chunk) {
         str+=chunk;
      });
      response.on('end', function() {
         callback.errorCounter=0;
         callback.lastAccess=Date.now();
         // console.log("ret:", str);
      });
      response.on('error', function(e) {
         console.log("error:", e.message);
      });
   });
   req.on('error', function(e) {
      console.log("error:", e.message);
      callback.errorCounter++;
   });

   callback.counter--;
   req.write(JSON.stringify({message: message, counter:callback.counter, callbackid: callback.id }));
   req.end();
}


function msgToCallback(message) {
   for(var i in callbacks_queue) {

      if(callbacks_queue[i].errorCounter>3) {
         delete callbacks_queue[i];
         continue;
      }

      if(callbacks_queue[i].counter==0) {
         delete callbacks_queue[i];
         continue;
      }

      var rep=checkFilter(callbacks_queue[i].filter, message)
      if(rep[0]==true) {
         if(rep[1]==200) {
            httpCall(callbacks_queue[i], message);
         }
         else if(rep[1]==400) {
            delete callbacks_queue[i];
         }
      }
   }
}


function msgToResponse(message) {
   var i = responses_queue.length
   while (i--) {
      if(responses_queue[i].xplRequestData.result!=undefined) {
         var _result=responses_queue[i].xplRequestData.result;

         var cond=_result.conditions;
         if(cond==undefined) {
            responses_queue.splice(i,1)
            continue;
         }

         var values=_result.values;
         if(values==undefined) {
            responses_queue.splice(i,1)
            continue;
         }

         var _values=true;
         for(var j in values) {
            if(message.body[values[j]]==undefined) {
               _values=false
            }
         }
         if(_values==false)
            continue;

         var rep=checkFilter(cond, message);
         if(rep[0]==true) {
            responses_queue[i].response.writeHead(JSON.stringify(rep[1]), contentType);
            if(rep[1]==200)
               responses_queue[i].response.end(JSON.stringify(message.body));
            else
               responses_queue[i].response.end(rep[2]);
            responses_queue.splice(i,1);
         }
      }
      else {
         responses_queue.splice(i,1)
      }
   }
}


function xplMessage(message) {

   var source=message.header.source;
   
   if(source == xpl._configuration.xplSource)
      return;

   var tsp=Date.now()

/*
   if(msgHistoryDb[source]==undefined)
      msgHistoryDb[source]=[]
   msgHistoryDb[source].unshift({timestamp: tsp, message: message})
*/
   console.log(message);
   msgToResponse(message);
   msgToCallback(message);
}


function cleanExit() {
  xpl.close();
}
