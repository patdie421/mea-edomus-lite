ace.define("ace/mode/mearules_highlight_rules",["require","exports","module","ace/lib/oop","ace/mode/text_highlight_rules"], function(require, exports, module) {
"use strict";

var oop = require("../lib/oop");
var TextHighlightRules = require("./text_highlight_rules").TextHighlightRules;

var MearulesHighlightRules = function() {

    var specials = (
        "<NOP>|<LABEL>"
    );

    var keywords1 = (
        "is:|if:|onmatch:|do:|when:|with:|elseis:"
    );

    var keywords2 = (
        "onmatch|moveforward|continue|break|change|rise|new|fall"
    );

    var builtinConstants = (
        "&0|&1|&true|&false|&high|&low"
    );

    var builtinFunctions = (
        "$startup|$exist|$rise|$fall|$stay|$change|$timerstatus|$timer|$date|$time|$calcn|$xplmsgdata|$now|$sunrise|$sunset|$twilightstart|$twilightend|$tohlstr|$totfstr"
    );

    var builtinActions = (
//        "xPLSend|timerCtrl|setInput|resetState"
        "xPLSend|timerCtrl|clearState"
    );
    var specialsMapper = this.createKeywordMapper({
        "keyword": specials
    }, "identifier", true);

// createKeywordMapper = function(map, defaultToken, ignoreCase, splitChar)
    var keywordMapper1 = this.createKeywordMapper({
        "keyword.control": keywords1
    }, "identifier", true);

    var keywordMapper2 = this.createKeywordMapper({
        "keyword.other": keywords2,
        "support.function": builtinActions
    }, "indentifier", true);

    var booleanMapper = this.createKeywordMapper({
       "constant.numeric": builtinConstants
    }, "identifier", true);

    var functionMapper = this.createKeywordMapper({
       "support.function": builtinFunctions
    }, "identifier", true);

    var actionMapper = this.createKeywordMapper({
       "support.function": builtinActions
    }, "identifier", true);

    this.$rules = {
        "start" : [
        {
           token : "keyword.operator",
           regex : ";",
           next  : "start"
        },
        {
            token : "comment",
            regex : "\\*\\*.*$"
        },  {
            token : "string",           // ' string
            regex : "'.*?'"
        },  {
            token : "constant.numeric", // variable
            regex : "{[a-zA-Z][a-zA-Z0-9_]*?}"
        }, {
            token : specialsMapper,     // variable
            regex : "<[A-Z]+?>"
        }, {
            token : "constant.numeric", // float
            regex : "#[+-]?\\d+(?:(?:\\.\\d*)?(?:[eE][+-]?\\d+)?)?\\b"
        }, {
            token : booleanMapper,      // boolean
            regex : "&[a-z01][a-z]*"
        }, {
            token : functionMapper,     // function
            regex : "\\$[a-z]+\\b"
        }, {
            token : keywordMapper1,
            regex : "[a-z]+\\:"
        }, {
            token : keywordMapper2,
            regex : "[a-zA-Z][a-zA-Z0-9_]*?\\b"
        }, {
            token : "keyword.operator",
            regex : "<|>|<=|=>|==|!="
        }, {
            token : "keyword.operator",
            regex : "="
        }, {
            token : "paren.lparen",
            regex : "[\\(]"
        }, {
            token : "paren.rparen",
            regex : "[\\)]"
        }, {
            token : "paren.lparen",
            regex : "[\\[]"
        }, {
            token : "paren.rparen",
            regex : "[\\]]"
        }, {
            token : "text",
            regex : "\\s+"
        } ]
    };
    this.normalizeRules();
};

oop.inherits(MearulesHighlightRules, TextHighlightRules);

exports.MearulesHighlightRules = MearulesHighlightRules;
});

ace.define("ace/mode/mearules",["require","exports","module","ace/lib/oop","ace/mode/text","ace/mode/mearules_highlight_rules","ace/range"], function(require, exports, module) {
"use strict";

var oop = require("../lib/oop");
var TextMode = require("./text").Mode;
var MearulesHighlightRules = require("./mearules_highlight_rules").MearulesHighlightRules;
var Range = require("../range").Range;

var Mode = function() {
    this.HighlightRules = MearulesHighlightRules;
};
oop.inherits(Mode, TextMode);

(function() {

    this.lineCommentStart = "--";

    this.$id = "ace/mode/mearules";
}).call(Mode.prototype);

exports.Mode = Mode;

});
