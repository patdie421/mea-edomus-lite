%{
#include <stdio.h>
#include <stdlib.h>
#include "y.tab.h"

#define YY_USER_ACTION file_offset += yyleng; line_offset +=yyleng;

char *str;
int file_offset=0;
int line_offset=0;

%}

%option yylineno

blancs          [\t ]+
newline         [\n\r]
lettre          [A-Za-z]
chiffre         [0-9]
nombre          {chiffre}+
virgule         ,
accolade_o      \{
accolade_f      \}
crochet_o       \[
crochet_f       \]
parenthese_o    \(
parenthese_f    \)
commentaire     \*\*.*
identifiant     (_|{lettre})(_|{lettre}|{chiffre})*
boolean         &([01]|{identifiant})
reel            #[+-]?{nombre}("."{nombre})?([eE][+-]?{nombre})?
chaine          '[^']*'
fonction        ${identifiant}
special         <{identifiant}>

%%
"=="            { return OPERATOR_EQ; }
"!="            { return OPERATOR_NE; }
">="            { return OPERATOR_GE; }
"<="            { return OPERATOR_LE; }
">"             { return OPERATOR_GT; }
"<"             { return OPERATOR_LW; }
"="             { return AFFECTATION; }
"is:"           { return INSTRUCTION_IS; }
"do:"           { return INSTRUCTION_DO; }
"if:"           { return INSTRUCTION_IF; }
"with:"         { return INSTRUCTION_WITH; }
"elseis:"       { return INSTRUCTION_ELSEIS; }
"onmatch:"      { return INSTRUCTION_ONMATCH; }
"onnotmatch:"   { return INSTRUCTION_ONNOTMATCH; }
"when:"         { return INSTRUCTION_WHEN; }
"rise"          { return RISE; }
"fall"          { return FALL; }
"change"        { return CHANGE; }
"break"         { return ACTION_BREAK; }
"continue"      { return ACTION_CONTINUE; }
"moveforward"   { return ACTION_MOVEFORWARD; }
{newline}       { line_offset = 0; /*yylineno++;*/ }

{blancs}        { }
{commentaire}   { }
{reel}          { yylval.str = strdup(yytext); return NOMBRE; }
{chaine}        { yylval.str = strdup(yytext); return CHAINE; }
{boolean}       { yylval.str = strdup(yytext); return BOOL; }
{identifiant}   { yylval.str = strdup(yytext); return IDENTIFIANT; }
{fonction}      { yylval.str = strdup(yytext); return FONCTION; }
{special}       { yylval.str = strdup(yytext); return SPECIAL; }
{crochet_o}     { return CROCHET_O; }
{crochet_f}     { return CROCHET_F; }
{accolade_o}    { return ACCOLADE_O; }
{accolade_f}    { return ACCOLADE_F; }
{parenthese_o}  { return PARENTHESE_O; }
{parenthese_f}  { return PARENTHESE_F; }
{virgule}       { return VIRGULE; }

.               { /* printf("Error line %d [%s]\n", yylineno, yytext); */ return ERROR; }
