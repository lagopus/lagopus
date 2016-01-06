/*
 * Copyright 2014-2016 Nippon Telegraph and Telephone Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* OpenFlow encoder/decoder generation parser */
%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "generator.h"

FILE *fp_source = NULL;
FILE *fp_header = NULL;

int yylex(void);
int yyerror (char const *str);
%}
%union {
  char *identifier;
  int int_value;
}
%token <identifier> IDENTIFIER
%token <int_value> INTEGER
%token STRUCT_T ENUM_T LC_T RC_T LB_T RB_T EQUAL_T COMMA_T SEMICOLON_T
%token LEFTSHIFT_T
%%
statement_list
: statement
| statement_list statement
;

statement
: struct_expression SEMICOLON_T
{
  create_statement();
}
;

struct_expression
: STRUCT_T IDENTIFIER block
{
  create_struct_expression($2);
}
| ENUM_T IDENTIFIER block
{
  create_enum_expression($2);
}
;

expression_list
: expression
| expression_list expression
;

expression
: IDENTIFIER IDENTIFIER SEMICOLON_T
{
  create_decl_expression($1, $2);
}
| IDENTIFIER IDENTIFIER LB_T INTEGER RB_T SEMICOLON_T
{
  create_decl_array_expression($1, $2, $4);
}
| IDENTIFIER IDENTIFIER LB_T IDENTIFIER RB_T SEMICOLON_T
{
  create_decl_array_id_expression($1, $2, $4);
}
| STRUCT_T IDENTIFIER IDENTIFIER SEMICOLON_T
{
  create_decl_struct_expression($2, $3);
}
| STRUCT_T IDENTIFIER IDENTIFIER LB_T INTEGER RB_T SEMICOLON_T
{
  create_decl_array_struct_expression($2, $3, $5);
}
| IDENTIFIER EQUAL_T INTEGER COMMA_T
{
  create_decl_enum_expression($1);
}
| IDENTIFIER EQUAL_T INTEGER
{
  create_decl_enum_expression($1);
}
| IDENTIFIER EQUAL_T IDENTIFIER COMMA_T
{
  create_decl_enum_expression($1);
}
| IDENTIFIER EQUAL_T IDENTIFIER
{
  create_decl_enum_expression($1);
}
| IDENTIFIER EQUAL_T num_shift COMMA_T
{
  create_decl_enum_expression($1);
}
| IDENTIFIER EQUAL_T num_shift
{
  create_decl_enum_expression($1);
}
;

num_shift
: INTEGER LEFTSHIFT_T INTEGER
;

block
: LC_T expression_list RC_T
| LC_T RC_T
;
%%
int
yyerror(char const *str)
{
  extern char *yytext;
  fprintf(stderr, "Parser error near [%s][%s]\n", yytext, str);
  return 0;
}

static void
usage(char *progname)
{
  fprintf(stderr, "Usage: %s <header file> <output file name>\n", progname);
  exit(1);
}

int
main(int argc, char **argv)
{
  extern FILE *yyin;
  char *p;
  char *progname;
  char hname[256];
  char cname[256];
  int ret;

  /* Get programe name from argv[0].  */
  progname = ((p = strchr(argv[0], '/')) ? ++p : argv[0]);

  /* When header file name is not specified. */
  if (argc != 3) {
    usage(progname);
  }

  /* Find out ".h" pointer.  */
  p = strrchr(argv[1], '.');
  if (! p || *(p + 1) != 'h')  {
    fprintf(stderr, "Need to specify header file.\n");
    usage(progname);
  }

  /* Open header file.  */
  yyin = fopen(argv[1], "r");
  if (yyin == NULL) {
    fprintf(stderr, "%s: Can't open header file %s\n", progname, argv[1]);
    exit(2);
  }

  /* Create open output files. */
  sprintf(hname, "%s.h", argv[2]);
  sprintf(cname, "%s.c", argv[2]);

  fp_source = fopen(cname, "w");
  fp_header = fopen(hname, "w");

  /* Need to generate C source file and header file.  */
  output_top(hname);

  /* Parse.  */
  ret = yyparse();

  output_bottom();

  /* Close files.  */
  fclose(yyin);
  fclose(fp_source);
  fclose(fp_header);

  /* Parse result check.  */
  if (ret != 0) {
    fprintf(stderr, "Error\n");
    exit(1);
  }

  return 0;
}
