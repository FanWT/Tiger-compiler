%{
#include <string.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "y.tab.h"
#include "errormsg.h"

int charPos=1;

int yywrap(void)
{
 charPos=1;
 return 1;
}

void adjust(void)
{
 EM_tokPos=charPos;
 charPos+=yyleng;
}
/*
* Please don't modify the lines above.
* You can add C declarations of your own below.
*/

%}
  /* You can add lex definitions here. */
%Start COMMENT
%%
  /*
  * Below are some examples, which you can wipe out
  * and write reguler expressions and actions of your own.
  */

" "  {adjust(); continue;}
\t  {adjust(); continue;}
\n	 {adjust(); EM_newline(); continue;}

<INITIAL>"/*" {adjust();BEGIN COMMENT;}
<COMMENT>[^*]    {adjust();}
<COMMENT>"*/" {adjust();BEGIN INITIAL;}

<INITIAL>for     {adjust(); return FOR;}
<INITIAL>while   {adjust(); return WHILE;}
<INITIAL>to      {adjust(); return TO;}
<INITIAL>break   {adjust(); return BREAK;}
<INITIAL>let     {adjust(); return LET;}
<INITIAL>in      {adjust(); return IN;}
<INITIAL>end     {adjust(); return END;}
<INITIAL>var     {adjust(); return VAR;}
<INITIAL>type    {adjust(); return TYPE;}
<INITIAL>array   {adjust(); return ARRAY;}
<INITIAL>if      {adjust(); return IF;}
<INITIAL>then    {adjust(); return THEN;}
<INITIAL>else    {adjust(); return ELSE;}
<INITIAL>do      {adjust(); return DO;}
<INITIAL>of      {adjust(); return OF;}
<INITIAL>nil     {adjust(); return NIL;}
<INITIAL>function  {adjust(); return FUNCTION;}

<INITIAL>","	 {adjust(); return COMMA;}
<INITIAL>":"	 {adjust(); return COLON;}
<INITIAL>";"	 {adjust(); return SEMICOLON;}
<INITIAL>"("	 {adjust(); return LPAREN;}
<INITIAL>")"	 {adjust(); return RPAREN;}
<INITIAL>"["	 {adjust(); return LBRACK;}
<INITIAL>"]"	 {adjust(); return RBRACK;}
<INITIAL>"{"	 {adjust(); return LBRACE;}
<INITIAL>"}"	 {adjust(); return RBRACE;}
<INITIAL>"."	 {adjust(); return DOT;}
<INITIAL>"+"	 {adjust(); return PLUS;}
<INITIAL>"-"	 {adjust(); return MINUS;}
<INITIAL>"*"	 {adjust(); return TIMES;}
<INITIAL>"/"	 {adjust(); return DIVIDE;}
<INITIAL>"="	 {adjust(); return EQ;}
<INITIAL>"<"	 {adjust(); return LT;}
<INITIAL>">"	 {adjust(); return GT;}
<INITIAL>"&"	 {adjust(); return AND;}
<INITIAL>"|"	 {adjust(); return OR;}
<INITIAL>"<>"	 {adjust(); return NEQ;}
<INITIAL>"<="	 {adjust(); return LE;}
<INITIAL>">="	 {adjust(); return GE;}
<INITIAL>":="	 {adjust(); return ASSIGN;}


<INITIAL>[0-9]+	 {adjust(); yylval.ival=atoi(yytext); return INT;}
<INITIAL>[a-zA-Z][a-zA-Z0-9_]* {adjust(); yylval.sval = String(yytext); return ID;}
<INITIAL>\"[^\"]*\" {adjust();
char temp[strlen(yytext)];
int index1 = 1, index2 = 0;
while (yytext[index1] != 0)
{
  temp[index2] = yytext[index1];
  index1++;
  index2++;
}
temp[index2-1]=0;

yylval.sval = String(temp);
return STRING;}

<INITIAL>.	 {adjust(); EM_error(EM_tokPos,"illegal token");}
