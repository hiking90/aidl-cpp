%{
#include <string.h>
#include <stdlib.h>

#include "aidl_language.h"
#include "aidl_language_y.hpp"
#include "parse_helpers.h"

// comment and whitespace handling
// these functions save a copy of the buffer
static void begin_extra_text(which_extra_text which);
static void append_extra_text(char* text);
static extra_text_type* get_extra_text(void);   // you now own the object
                                                // this returns
static void drop_extra_text(void);

using android::aidl::cpp_strdup;

#define RETURN_BUFFER(t) \
    do { \
        yylval->buffer.token = (t); \
        yylval->buffer.data = cpp_strdup(yytext); \
        yylval->buffer.extra = get_extra_text(); \
        return (t); \
    } while(0)

#define YY_USER_ACTION yylloc->columns(yyleng);
%}

%option yylineno
%option noyywrap
%option reentrant
%option bison-bridge
%option bison-locations

%x COPYING LONG_COMMENT

identifier  [_a-zA-Z][_a-zA-Z0-9\.]*
whitespace  ([ \t\r]+)
idvalue     (0|[1-9][0-9]*)

%%
%{
  /* This happens at every call to yylex (every time we receive one token) */
  yylloc->step();
%}


\%\%\{                { begin_extra_text(COPY_TEXT);
                        BEGIN(COPYING); }
<COPYING>\}\%\%       { yylloc->step(); BEGIN(INITIAL); }
<COPYING>.*           { append_extra_text(yytext); }
<COPYING>\n+          { yylloc->lines(yyleng); append_extra_text(yytext); }

\/\*                  { begin_extra_text((which_extra_text)LONG_COMMENT);
                        BEGIN(LONG_COMMENT); }
<LONG_COMMENT>\n+     { yylloc->lines(yyleng); append_extra_text(yytext); }
<LONG_COMMENT>[^*]*   { append_extra_text(yytext); }
<LONG_COMMENT>\*+[^/] { append_extra_text(yytext); }
<LONG_COMMENT>\*+\/   { yylloc->step(); BEGIN(INITIAL);  }

\/\/.*\n              { begin_extra_text(SHORT_COMMENT);
                        append_extra_text(yytext);
                        yylloc->lines(1);
                        yylloc->step(); }

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
interface             { RETURN_BUFFER(yy::parser::token::INTERFACE); }
oneway                { RETURN_BUFFER(yy::parser::token::ONEWAY); }

parcelable            { return yy::parser::token::PARCELABLE; }
import                { return yy::parser::token::IMPORT; }
package               { return yy::parser::token::PACKAGE; }
in                    { return yy::parser::token::IN; }
out                   { return yy::parser::token::OUT; }
inout                 { return yy::parser::token::INOUT; }

    /* scalars */
{idvalue}             { RETURN_BUFFER(yy::parser::token::IDVALUE); }
{identifier}          { RETURN_BUFFER(yy::parser::token::IDENTIFIER); }

    /* syntax error! */
.                     { printf("UNKNOWN(%s)", yytext);
                        yylval->buffer.token = yy::parser::token::IDENTIFIER;
                        yylval->buffer.data = cpp_strdup(yytext);
                        return yy::parser::token::IDENTIFIER;
                      }

%%

// comment and whitespace handling
// ================================================
extra_text_type* g_extraText = NULL;
extra_text_type* g_nextExtraText = NULL;

void begin_extra_text(which_extra_text which)
{
    extra_text_type* text = new extra_text_type();
    text->which = which;
    text->data = NULL;
    text->len = 0;
    text->next = NULL;
    if (g_nextExtraText == NULL) {
        g_extraText = text;
    } else {
        g_nextExtraText->next = text;
    }
    g_nextExtraText = text;
}

void append_extra_text(char* text)
{
    if (g_nextExtraText->data == NULL) {
        g_nextExtraText->data = cpp_strdup(text);
        g_nextExtraText->len = strlen(text);
    } else {
        char* orig = g_nextExtraText->data;
        unsigned oldLen = g_nextExtraText->len;
        unsigned len = strlen(text);
        g_nextExtraText->len += len;
        g_nextExtraText->data = new char[g_nextExtraText->len+1];
        memcpy(g_nextExtraText->data, orig, oldLen);
        memcpy(g_nextExtraText->data+oldLen, text, len);
        g_nextExtraText->data[g_nextExtraText->len] = '\0';
        delete[] orig;
    }
}

extra_text_type*
get_extra_text(void)
{
    extra_text_type* result = g_extraText;
    g_extraText = NULL;
    g_nextExtraText = NULL;
    return result;
}

void drop_extra_text(void)
{
    extra_text_type* p = g_extraText;
    while (p) {
        extra_text_type* next = p->next;
        delete[] p->data;
        delete p;
        p = next;
    }
    g_extraText = NULL;
    g_nextExtraText = NULL;
}
