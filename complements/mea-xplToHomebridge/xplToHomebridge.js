var http = require('http');
var https = require('https');
var Xpl = require("xpl-api");
var commander = require('commander');
var crypto = require('crypto');
var url = require("url");
var fs = require("fs");
 
const contentType={'content-type':'application/json'}
const defaultTimeOut=5000
const HTTP_PORT=7104

var defaults={
   "networkInterface":"eth0",
   "notifHost":"localhost",
   "notifPort":7103,
   "xplSource":"mea-xpl2hb.home",
};

var notifServer=false
var notifPort=false


var devices={
   "o02a" : {
      "id":"device",
      "value":"current",
      "on": {"device":"o02a","current":"high","type":"output"},
      "off": {"device":"o02a","current":"low","type":"output"},
      "filters":  [
         { "device": "o02a", "schema": "^sensor\\..*$", "current":"^.*$", "source":"^.*$", "msgtype":["^.*-stat$","^.*-trig$"] }
      ]
   },
   "o02b" : {
      "id":"device",
      "value":"current",
      "on": {"device":"o02b","current":"high","type":"output"},
      "off": {"device":"o02b","current":"low","type":"output"},
      "filters":  [
         { "device": "o02b", "schema": "^sensor\\..*$", "current":"^.*$", "source":"^.*$", "msgtype":["^.*-stat$","^.*-trig$"] }
      ]
   }
};


commander.version('0.0.1');
commander.option("-c --cfgFile <file>", "Specify a configuration file");
commander.option("-n --networkInterface <name>", "Specify the network interface name");
commander.option("-d --hubPingDelaySecond <sec>", "");
commander.option("-s --xplSource <name>", "Specify the source in XPL message");
commander.option("-t --xplTarget <name>", "Specify the target in XPL message");
commander.option("-t --notifHost <name>", "notify server name or ip");
commander.option("-t --notifPort <n>", "notif server port");
commander.parse(process.argv);


var config={}

if(commander.cfgFile) {
   var f = fs.readFileSync(commander.cfgFile);
   config = JSON.parse(f);
   if(config.devices) {
      devices=config.devices;
   }
}
console.log(devices)

for(var i in defaults) {
   if(!commander[i]) {
      if(config[i]) {
         commander[i]=config[i]
      }
      else {
         commander[i]=defaults[i]
      }
   }
}


notifServer=commander.notifHost
notifPort=commander.notifPort

var responses_queue=[]
setInterval(manageResponsesQueue, 100);
 
 
var xpl = new Xpl(commander);
var server = http.createServer(requestHandler);
server.listen(HTTP_PORT, function() {
      console.log("Server listening on port #"+HTTP_PORT);
   }
);
 
 
function isArray(a) {
   return (!!a) && (a.constructor === Array);
}
 

function callHttp(serverUrl, method, data, onsuccess, onerror) {
   var options = url.parse(serverUrl);
   options.method=method;

   var req=http.request(options, (response) => {
      var str='';
      response.on('data', function(chunk) {
         str+=chunk;
      });
      response.on('end', function() {
         if(typeof(onsuccess)=="function") {
            var _str=""
            try {
               _str=JSON.parse(str)
            }
            catch(e) {
               _str=str
            }
            onsuccess(_str);
         }
      });
      response.on('error', function(e) {
         console.log("error:", e.message);
         if(typeof(onerror)=="function")
            onerror(e);
      });
   });
   req.on('error', function(e) {
      console.log("error:", e.message);
      if(typeof(onerror)=="function")
         onerror(e);
   });
   if(method!='GET' && method!='DELETE') {
      req.write(JSON.stringify(data));
   }
   req.end();
}

 
function manageResponsesQueue() {
   var i = responses_queue.length
   var now=Date.now()
   while (i--) {
      if((now - responses_queue[i].timestamp) > responses_queue[i].timeout) {
         responses_queue[i].response.writeHead(404, contentType);
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
/*
         for(var i in xplRequestData.result.conditions) {
            if(typeof(i)!="string" || typeof(xplRequestData.result.conditions[i])!="string") {
               return "result.conditions data error";
            }
         }
*/
      }
 
      if(xplRequestData.result.timeout && isNaN(xplRequestData.result.timeout)) {
         return "result.timeout is not a number";
      }
 
      if(xplRequestData.xplmsgtype) {
         if(typeof(xplRequestData.xplmsgtype) != "string") {
            return "xplmsgtype is not a string";
         }

         if(xplRequestData.xplmsgtype !== "cmnd" &&
            xplRequestData.xplmsgtype !== "stat" &&
            xplRequestData.xplmsgtype !== "trig") {
            return "xplmsgtype not supported";
         }
      } 
   }
 
   return true;
}
 

function doSendMessage(op, surl, request, response, data) {

   var xplRequestData=data;

   if(surl.length != 0) {
      response.writeHead(400);
      response.end();
      return -1;
   }

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

 
function doRequest(surl, request, response, data) {
   var method = request.method;
 
   if(surl.length != 0) {
      response.writeHead(400);
      response.end();
      return -1;
   }
 
   var xplRequestData=data;
   var xplmsgtype=0;
   var ret=checkXplRequestData(xplRequestData);
   if(ret!==true) {
      response.writeHead(400, contentType);
      response.end('{"status":"error","errorcode":6, "message":"'+ret+'"}');
      return -1;
   }
   var timeout=defaultTimeOut;

   var tsp=Date.now()
   if(!xplRequestData.schema || typeof(xplRequestData.schema) !== "string") {
      xplRequestData["schema"]="sensor.request";
   }

   if(!xplRequestData.target || typeof(xplRequestData.target) !== "string") {
      xplRequestData["target"]="*";
   }
 
   if(!xplRequestData.xplmsgtype)
      xplmsgtype="cmnd";
   else
      xplmsgtype=xplRequestData.xplmsgtype;
 
   if(!isNaN(xplRequestData.result.timeout)) {
      timeout=xplRequestData.result.timeout;
   }

   responses_queue.push({xplRequestData: xplRequestData, timestamp: tsp, response: response, timeout: timeout});

   switch(xplmsgtype) {
      case 'cmnd':
         xpl.sendXplCmnd(xplRequestData.body, xplRequestData["schema"], xplRequestData["target"]);
         break;
      case 'stat':
         xpl.sendXplStat(xplRequestData.body, xplRequestData["schema"], xplRequestData["target"]);
         break;
      case 'trig':
         xpl.sendXplTrig(xplRequestData.body, xplRequestData["schema"], xplRequestData["target"]);
         break;
  }
  return 0;
}
 

function doDevice(surl, request, response, data)
{
   console.log(surl.length, surl)
   if(request.method!="GET")
   {
      response.writeHead(400);
      response.end("{}");
      return;
   }

   if(surl.length>2) {
      response.writeHead(400);
      response.end("{}");
      return;
   }

   if(surl.length==0) {
      response.writeHead(200);
      response.end('{"msg":"todo"}');
      return;
   }

   var id=surl[0]
   var d=false
   if(devices[id])
      d=devices[id]
   else {
      response.writeHead(400);
      response.end(JSON.stringify({msg: "device not defined"}));
      return;
   }

   var statusData={
      "body": {
         "device": id,
         "request": d.value },
      "target":"*",
      "schema":"sensor.request",
      "result": {
         "values": [ d.value ],
         "conditions" : d.filters[0],
         "timeout":5000
      }
   }

   if(surl.length==1) {
      try {
         doRequest([], request, response, statusData)
      }
      catch(e) {
         console.log(e);
         response.writeHead(400);
         response.end(JSON.stringify({msg: "error: "+e}));
         return;
      }
   }
   else {
      var action=surl[1].toLowerCase()

      switch(action) {
         case "on":
         case "off":
            try {
               xpl.sendXplCmnd(d[action], "control.basic", "*");
               doRequest([], request, response, statusData);
            }
            catch(e) {
               console.log(e);
               response.writeHead(400);
               response.end(JSON.stringify({msg: "error: "+e}));
               return;
            }
            break;
         default:
            response.writeHead(400);
            response.end("{}");
            break;
      }
   }
}

 
function doXpl(surl, request, response, data)
{
   if(surl.length==0) {
      response.writeHead(200);
      response.end("{}");
   }
   else {
      var op=surl[0];
      surl.splice(0,1)

      if(request.method != "POST") {
         response.writeHead(400);
         response.end();
         return 0;
      }

      switch(op) {
         case "request":
            doRequest(surl, request, response, data);
            break;
         case "cmnd":
         case "stat":
         case "trig":
            doSendMessage(op, surl, request, response, data);
            break;
         default:
            response.writeHead(400);
            response.end();
            break;
      }
   }
   return 1;
}
 
 
function requestHandler(request, response)
{
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
            data=undefined;
         }
         surl.splice(0,2)
         switch(selector) {
            case "device":
               doDevice(surl, request, response, data);
               break;
            case "xpl":
               doXpl(surl, request, response, data);
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
   console.log("XPLISINIT")
}
 
 
function checkFilter(cond, message) {
   var _cond;
   for(var j in cond) {
      _cond=false;
      var regex=false;
      var c=[]
      if(typeof(cond[j]) == "string") {
         c=[ cond[j] ]
      }
      else {
         c=cond[j]
      }

      for(i in c) {
         try {
            regex=new RegExp(c[i])
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
         else if(j=="msgtype") {
            if(message.headerName.match(regex)) {
               _cond=true;
            }
         }
         else if(j in message.body) {
            if(message.body[j].match(regex)) {
               _cond=true;
            }
         }
         if(_cond==true)
            break;
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
 
 
function msgToNotify(message) {

   for (var i in message.body) {
      message.body[i]=message.body[i].toLowerCase()
   }

   for (var i in devices) { 
      for (var j in devices[i].filters) {
         var rep=checkFilter(devices[i].filters[j], message);
         if (rep[0]==true) {
            var notificationID=message.body[devices[i].id];
            var onoff=message.body[devices[i].value];
            var flag=false
            switch(onoff)
            {
               case "low":
               case "off":
                  onoff=false;
                  flag=true;
                  break;
               case "high":
               case "on":
                  onoff=true;
                  flag=true;
                  break;
               default:
                  break;
            }
            if(flag===true) {
               callHttp("http://"+notifServer+":"+notifPort+"/"+notificationID, "POST", {characteristic: "On", value: onoff}, undefined, undefined );
            }
            break;
         }
      }
   }
}
 
 
function xplMessage(message) {
   var source=message.header.source;
 
   if(source == xpl._configuration.xplSource) {
      return;
   }
   var tsp=Date.now()
 
   msgToResponse(message);
   msgToNotify(message);
}
 
 
function cleanExit() {
  xpl.close();
}
