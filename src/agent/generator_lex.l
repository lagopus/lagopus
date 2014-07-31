%{
#include <stdio.h>
#include "generator_parser.h"

int
yywrap(void)
{
  return 1;
}
%}
%%
^"#".*$    ;
"/*".*"*/" ;
"//".*$    ;
"struct" return STRUCT_T;
"enum"   return ENUM_T;
"{"      return LC_T;
"}"      return RC_T;
"["      return LB_T;
"]"      return RB_T;
"="      return EQUAL_T;
";"      return SEMICOLON_T;
","      return COMMA_T;
"<<"     return LEFTSHIFT_T;
" "      ;
"\n" {
  /* We can put line number count at here.  */
}
0|([1-9][0-9]*) {
  int temp;
  sscanf(yytext, "%d", &temp);
  yylval.int_value = temp;
  return INTEGER;
}
[a-zA-Z0-9_][a-zA-Z0-9_]* {
  yylval.identifier = strdup(yytext);
  return IDENTIFIER;
}
%%
