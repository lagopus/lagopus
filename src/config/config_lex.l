%{
#include <stdio.h>
#include "config_parser.h"

int
yywrap(void)
{
  return 1;
}
%}
%%
"{"    return LC_T;
"}"    return RC_T;
";"    return SEMICOLON_T;
" "    ;
^[ \t]*#.*$  ;
"\n"   {
  /* We can put line number count at here. */
}
[a-zA-Z0-9/][a-zA-Z0-9Z:\.\-/]* {
  yylval.identifier = strdup(yytext);
  return IDENTIFIER;
}
%%
