%{
#include <string.h>
#include <stdlib.h>

#include "aidl_language.h"
#include "aidl_language_y.hpp"

#define YY_USER_ACTION yylloc->columns(yyleng);
%}

%option yylineno
%option noyywrap
%option reentrant
%option bison-bridge
%option bison-locations

%x COPYING LONG_COMMENT

identifier  [_a-zA-Z][_a-zA-Z0-9]*
whitespace  ([ \t\r]+)
idvalue     (0|[1-9][0-9]*)

%%
%{
  /* This happens at every call to yylex (every time we receive one token) */
  std::string extra_text;
  yylloc->step();
%}


\%\%\{                { extra_text += "/**"; BEGIN(COPYING); }
<COPYING>\}\%\%       { extra_text += "**/"; yylloc->step(); BEGIN(INITIAL); }
<COPYING>.*           { extra_text += yytext; }
<COPYING>\n+          { extra_text += yytext; yylloc->lines(yyleng); }

\/\*                  { extra_text += yytext; BEGIN(LONG_COMMENT); }
<LONG_COMMENT>\n+     { extra_text += yytext; yylloc->lines(yyleng); }
<LONG_COMMENT>[^*\n]* { extra_text += yytext; }
<LONG_COMMENT>\*+[^/] { extra_text += yytext; }
<LONG_COMMENT>\*+\/   { extra_text += yytext; yylloc->step(); BEGIN(INITIAL);  }

\"[^\"]*\"            { yylval->token = new AidlToken(yytext, extra_text);
                        return yy::parser::token::C_STR; }

\/\/.*\n              { extra_text += yytext; yylloc->lines(1); yylloc->step(); }

\n+                   { yylloc->lines(yyleng); yylloc->step(); }
{whitespace}          {}
<<EOF>>               { yyterminate(); }

    /* symbols */
;                     { return ';'; }
\{                    { return '{'; }
\}                    { return '}'; }
=                     { return '='; }
,                     { return ','; }
\.                    { return '.'; }
\(                    { return '('; }
\)                    { return ')'; }
\[                    { return '['; }
\]                    { return ']'; }
\<                    { return '<'; }
\>                    { return '>'; }

    /* keywords */
parcelable            { return yy::parser::token::PARCELABLE; }
import                { return yy::parser::token::IMPORT; }
package               { return yy::parser::token::PACKAGE; }
in                    { return yy::parser::token::IN; }
out                   { return yy::parser::token::OUT; }
inout                 { return yy::parser::token::INOUT; }
from                  { return yy::parser::token::FROM; }

interface             { yylval->token = new AidlToken("interface", extra_text);
                        return yy::parser::token::INTERFACE;
                      }
oneway                { yylval->token = new AidlToken("oneway", extra_text);
                        return yy::parser::token::ONEWAY;
                      }

    /* scalars */
{identifier}          { yylval->token = new AidlToken(yytext, extra_text);
                        return yy::parser::token::IDENTIFIER;
                      }
{idvalue}             { yylval->integer = std::stoi(yytext);
                        return yy::parser::token::IDVALUE; }

    /* syntax error! */
.                     { printf("UNKNOWN(%s)", yytext);
                        yylval->token = new AidlToken(yytext, extra_text);
                        return yy::parser::token::IDENTIFIER;
                      }

%%

// comment and whitespace handling
// ================================================
