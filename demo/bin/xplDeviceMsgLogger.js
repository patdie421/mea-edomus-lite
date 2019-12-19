var Xpl = require("xpl-api");
var commander = require('commander');

commander.option("--networkInterface <name>", "Specify the network interface name");
commander.option("--hubPingDelaySecond <sec>", "");
commander.option("--xplSource <name>", "Specify the source in XPL message");
commander.option("--xplTarget <name>", "Specify the target in XPL message");

commander.parse(process.argv);

var xpl = new Xpl(commander);


xpl.on("close", () => {
  cleanExit();
});


xpl.on("message", xplMessage);


xpl.bind( xplIsInit );


function xplIsInit() {
   console.log("xpl is initialized");
}


function xplMessage(message) {

   var source=message.header.source;
   
   if(source == xpl._configuration.xplSource)
      return;

   var tsp=Date.now()

   if ( message.headerName=='xpl-trig' ||
        message.headerName=='xpl-stat' )  {
      if("device" in message.body) {
         date=new Date(message.timestamp);
         message.body["date"]=date.toISOString();
         console.log(JSON.stringify(message.body));
      }
   }
}


function cleanExit() {
  xpl.close();
  process.exit(0);
}
