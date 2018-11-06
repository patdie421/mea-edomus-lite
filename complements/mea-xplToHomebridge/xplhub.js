/**
 * New node file
 */

var Xpl = require("xpl-api");
var commander = require('commander');

commander.option("--networkInterface <name>", "Specify the network interface name");

commander.parse(process.argv);

commander["hubSupport"]=true
// commander["debugOn"]=true

var xpl = new Xpl(commander);

xpl.on("message", (message) => {
   console.log("Receive message ", message);
});

xpl.on("close", () => {
   console.log("Receive close even");
});

xpl.bind( xplIsInit );

function xplIsInit() {
}
