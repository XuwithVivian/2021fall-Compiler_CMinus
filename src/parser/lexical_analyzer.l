%option noyywrap
%{
/*****************声明和选项设置  begin*****************/
#include <stdio.h>
#include <stdlib.h>

#include "syntax_tree.h"
#include "syntax_analyzer.h"

int lines;
int pos_start;
int pos_end;

void pass_node(char *text){
     yylval.node = new_syntax_tree_node(text);
}

/*****************声明和选项设置  end*****************/

%}


%%

 /*去除空格*/
[ \t] {pos_start = pos_end; pos_end += 1;}

 /* to do for students */
 /* two cases for you, pass_node will send flex's token to bison */

 /*专用符号*/
\+ {pos_start = pos_end; pos_end += 1; pass_node(yytext); return ADD;}
\- {pos_start = pos_end; pos_end += 1; pass_node(yytext); return SUBTRACT;}
\* {pos_start = pos_end; pos_end += 1; pass_node(yytext); return MULTIPLY;}
\/ {pos_start = pos_end; pos_end += 1; pass_node(yytext); return DIVIDE;}
\< {pos_start = pos_end; pos_end += 1; pass_node(yytext); return LESS;}
\<= {pos_start = pos_end; pos_end += 2; pass_node(yytext); return LESSEQUAL;}
\> {pos_start = pos_end; pos_end += 1; pass_node(yytext); return GREATER;}
\>= {pos_start = pos_end; pos_end += 2; pass_node(yytext); return GREATEREQUAL;}
\== {pos_start = pos_end; pos_end += 2; pass_node(yytext); return EQUAL;}
\!= {pos_start = pos_end; pos_end += 2; pass_node(yytext); return NEQUAL;}
\= {pos_start = pos_end; pos_end += 1; pass_node(yytext); return ASSIGNMENT;}
; {pos_start = pos_end; pos_end += 1; pass_node(yytext); return SEMICOLON;}
, {pos_start = pos_end; pos_end += 1; pass_node(yytext); return COMMA;}
\( {pos_start = pos_end; pos_end += 1; pass_node(yytext); return LPAREN;}
\) {pos_start = pos_end; pos_end += 1; pass_node(yytext); return RPAREN;}
\[ {pos_start = pos_end; pos_end += 1; pass_node(yytext); return LBRACKET;}
\] {pos_start = pos_end; pos_end += 1; pass_node(yytext); return RBRACKET;}
\{ {pos_start = pos_end; pos_end += 1; pass_node(yytext); return LBRACE;}
\} {pos_start = pos_end; pos_end += 1; pass_node(yytext); return RBRACE;}

 /*关键字*/
else {pos_start = pos_end; pos_end += 4; pass_node(yytext); return ELSE;}
if {pos_start = pos_end; pos_end += 2; pass_node(yytext); return IF;}
int {pos_start = pos_end; pos_end += 3; pass_node(yytext); return INT;}
return {pos_start = pos_end; pos_end += 6; pass_node(yytext); return RETURN;}
void {pos_start = pos_end; pos_end += 4; pass_node(yytext); return VOID;}
while {pos_start = pos_end; pos_end += 5; pass_node(yytext); return WHILE;}
float {pos_start = pos_end; pos_end += 5; pass_node(yytext); return FLOAT;}

 /*标识符ID和整数NUM，通过下列正则表达式定义：*/
[a-zA-Z]+ {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return ID;}
[0-9]+ {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return INTEGER;}
[0-9]+\.|[0-9]*\.[0-9]+ {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return FLOATING;}
\n {pos_start = pos_end = 0; lines++;}

 /*正确的注释语法*/
"/*"([^\*]|(\*)*[^\*/])*(\*)*"*/" {
     pos_start = pos_end;
     for (int i = 0; i < strlen(yytext); i++) {
          if(yytext[i] == '\n') {
               pos_start = pos_end = 0;
               lines++;
          }
          else
            pos_end++;
     }
}

. { pos_start = pos_end; pos_end++; return ERROR; }



 /****请在此补全所有flex的模式与动作  end******/
%%
