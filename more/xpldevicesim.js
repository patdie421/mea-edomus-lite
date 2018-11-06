var http = require('http');
var Xpl = require("xpl-api");
var commander = require('commander');


commander.option("--networkInterface <name>", "Specify the network interface name");
commander.option("--hubPingDelaySecond <sec>", "");
commander.option("--xplSource <name>", "Specify the source in XPL message");
commander.option("--xplTarget <name>", "Specify the target in XPL message");
commander.parse(process.argv);


var xpl = new Xpl(commander);


xpl.on("close", () => {
  process.exit(0);
});


xpl.on("message", xplMessage);


xpl.bind( xplIsInit );


function xplIsInit() {
}


function sendTrig() {
   xpl.sendXplTrig({ device: "XX01", type: "temp", current: Math.random()*20+10 });
}

setInterval(sendTrig, 60*1000);

function xplMessage(message) {
   if(message.header.source == xpl._configuration.xplSource)
      return;

   console.log(message);

   if(message.headerName == "xpl-cmnd") {
      if(message.body.request=="current" && message.body.device!=undefined) {
         
         xpl.sendXplStat({ device: message.body.device, type: "temp", current: Math.random()*20+10 });
      }
   }
}


function cleanExit() {
  xpl.close();
}
