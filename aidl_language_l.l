%{
#include <string.h>
#include <stdlib.h>

#include "aidl_language.h"
#include "aidl_language_y.hpp"
#include "parse_helpers.h"
#include "search_path.h"

// comment and whitespace handling
// these functions save a copy of the buffer
static void begin_extra_text(unsigned lineno, which_extra_text which);
static void append_extra_text(char* text);
static extra_text_type* get_extra_text(void);   // you now own the object
                                                // this returns
static void drop_extra_text(void);

// package handling
static void do_package_statement(const char* importText);

using android::aidl::cpp_strdup;

#define SET_BUFFER(t) \
    do { \
        yylval->buffer.lineno = yyget_lineno(yyscanner); \
        yylval->buffer.token = (t); \
        yylval->buffer.data = cpp_strdup(yytext); \
        yylval->buffer.extra = get_extra_text(); \
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
brackets    \[{whitespace}?\]
idvalue     (0|[1-9][0-9]*)

%%
%{
  /* This happens at every call to yylex (every time we receive one token) */
  yylloc->step();
%}


\%\%\{              { begin_extra_text(yylineno, COPY_TEXT); BEGIN(COPYING); }
<COPYING>\}\%\%     { yylloc->step(); BEGIN(INITIAL); }
<COPYING>.*         { append_extra_text(yytext); }
<COPYING>\n+        { yylloc->lines(yyleng); append_extra_text(yytext); }


\/\*                            { begin_extra_text(yylineno, (which_extra_text)LONG_COMMENT);
                                  BEGIN(LONG_COMMENT); }
<LONG_COMMENT>[^*]*             { append_extra_text(yytext); }
<LONG_COMMENT>\*+[^/]           { append_extra_text(yytext); }
<LONG_COMMENT>\n+               { yylloc->lines(yyleng); append_extra_text(yytext); }
<LONG_COMMENT>\**\/             { yylloc->step(); BEGIN(INITIAL); }

^{whitespace}?import{whitespace}[^ \t\r\n]+{whitespace}?;  {
                                                SET_BUFFER(yy::parser::token::IMPORT);
                                                return yy::parser::token::IMPORT;
                                            }
^{whitespace}?package{whitespace}[^ \t\r\n]+{whitespace}?;  {
                                                do_package_statement(yytext);
                                                SET_BUFFER(yy::parser::token::PACKAGE);
                                                return yy::parser::token::PACKAGE;
                                            }
<<EOF>>             { yyterminate(); }

\/\/.*\n            { begin_extra_text(yylineno, SHORT_COMMENT);
                        append_extra_text(yytext); yylloc->lines(1);
                        yylloc->step(); }

\n+ { yylloc->lines(yyleng); yylloc->step(); }

{whitespace}    { /* begin_extra_text(yylineno, WHITESPACE);
                    append_extra_text(yytext); */ }

;               { SET_BUFFER(';'); return ';'; }
\{              { SET_BUFFER('{'); return '{'; }
\}              { SET_BUFFER('}'); return '}'; }
\(              { SET_BUFFER('('); return '('; }
\)              { SET_BUFFER(')'); return ')'; }
,               { SET_BUFFER(','); return ','; }
=               { SET_BUFFER('='); return '='; }

    /* keywords */
parcelable      { SET_BUFFER(yy::parser::token::PARCELABLE); return yy::parser::token::PARCELABLE; }
interface       { SET_BUFFER(yy::parser::token::INTERFACE); return yy::parser::token::INTERFACE; }
oneway          { SET_BUFFER(yy::parser::token::ONEWAY); return yy::parser::token::ONEWAY; }

in              { return yy::parser::token::IN; }
out             { return yy::parser::token::OUT; }
inout           { return yy::parser::token::INOUT; }

{brackets}+     { SET_BUFFER(yy::parser::token::ARRAY); return yy::parser::token::ARRAY; }
{idvalue}       { SET_BUFFER(yy::parser::token::IDVALUE); return yy::parser::token::IDVALUE; }
{identifier}                                        { SET_BUFFER(yy::parser::token::IDENTIFIER); return yy::parser::token::IDENTIFIER; }
{identifier}\<{whitespace}*{identifier}({whitespace}*,{whitespace}*{identifier})*{whitespace}*\>    {
                                                      SET_BUFFER(yy::parser::token::GENERIC); return yy::parser::token::GENERIC; }

    /* syntax error! */
.               { printf("UNKNOWN(%s)", yytext);
                  yylval->buffer.lineno = yylineno;
                  yylval->buffer.token = yy::parser::token::IDENTIFIER;
                  yylval->buffer.data = cpp_strdup(yytext);
                  return yy::parser::token::IDENTIFIER;
                }

%%

// comment and whitespace handling
// ================================================
extra_text_type* g_extraText = NULL;
extra_text_type* g_nextExtraText = NULL;

void begin_extra_text(unsigned lineno, which_extra_text which)
{
    extra_text_type* text = new extra_text_type();
    text->lineno = lineno;
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


// package handling
// ================================================
void do_package_statement(const char* importText)
{
    if (g_currentPackage) delete[] g_currentPackage;
    g_currentPackage = android::aidl::parse_import_statement(importText);
}
