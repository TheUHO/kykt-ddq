%option	reentrant bison-bridge bison-locations
%option	yylineno
%option noyywrap

%{

#include <string.h>
#include "ast.h"
#include "ast.tab.h"

%}

single_comment  \/\/.*
%x MULTI_COMMENT
load_op         load_op
load_type       load_type
load_data       load_data
load_so         load_so
load_builtin    load_builtin
load_python     load_python
boolean         TRUE|FALSE
defcall         :=
string          [a-zA-Z_][a-zA-Z0-9_]*
path            \([a-zA-Z_/\.][a-zA-Z0-9_/\.]*\)
integer         [-+]?[0-9]+
real            [-+]?[0-9]+\.?[0-9]+([eE][-+]?[0-9]+)?
quote_string    \"[^\"]*\"
delim           [ \n\t]
whitespace      {delim}+ 
ascii           .

%%

"/*"            { BEGIN(MULTI_COMMENT); }  
<MULTI_COMMENT>"\n"     { /* 忽略注释中的换行符 */ }  
<MULTI_COMMENT>"*/"      {  
    BEGIN(INITIAL);  
}  
<MULTI_COMMENT>.     { /* 忽略注释中的除换行符的所有字符 */ }  

{single_comment}			{ /* eat comments */ }
{load_op}			{ yylval->num_i = ast_type_load_op; return LOAD_OP; }
{load_type}			{ yylval->num_i = ast_type_load_type; return LOAD; }
{load_data}			{ yylval->num_i = ast_type_load_data; return LOAD; }
{load_so}			{ yylval->num_i = ast_type_load_so; return LOAD; }
{load_builtin}			{ yylval->num_i = ast_type_load_builtin; return LOAD; }
{load_python}			{ yylval->num_i = ast_type_load_python; return LOAD; }
{integer}			{ yylval->num_i = atoi(yytext); return INTEGER; }
{real}			{ yylval->num_r = atof(yytext); return REAL; }
{defcall}       {return DEFCALL;}
{boolean}			{ yylval->num_i = (strcmp(yytext, "TRUE") == 0); return BOOLEAN; }
{string}			{ yylval->str = strdup(yytext); return STRING; }
{path}				{ yylval->str = strdup(yytext); return PATH_STRING; }
{quote_string}			{ yylval->str = strdup(yytext); return QUOTE_STRING; }
{whitespace}			{ /* eat whitespace */ }
{ascii}				{ return yytext[0]; }


%%



