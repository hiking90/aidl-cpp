%{
#include "aidl_language.h"
#include "aidl_language_y.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int yylex(yy::parser::semantic_type *, yy::parser::location_type *, void *);

static int count_brackets(const char*);

#define lex_scanner ps->Scanner()

%}

%parse-param { ParseState* ps }
%lex-param { void *lex_scanner }

%pure-parser
%skeleton "glr.cc"

%union {
    buffer_type buffer;
    type_type type;
    arg_type *arg;
    method_type* method;
    interface_item_type* interface_item;
    interface_type* interface_obj;
    user_data_type* user_data;
    document_item_type* document_item;
}

%token<buffer> IMPORT PACKAGE IDENTIFIER IDVALUE GENERIC ARRAY PARCELABLE
%token<buffer> ONEWAY INTERFACE IN OUT INOUT ';' '{' '}' '(' ')' ',' '='

%type<document_item> document_items declaration
%type<user_data> parcelable_decl
%type<interface_item> interface_items
%type<interface_obj> interface_decl interface_header
%type<method> method_decl
%type<type> type
%type<arg> arg_list arg
%type<buffer> direction

%type<buffer> error
%%
document:
        document_items                          { ps->ProcessDocument(*$1); }
    |   headers document_items                  { ps->ProcessDocument(*$2); }
    ;

headers:
        package                                 { }
    |   imports                                 { }
    |   package imports                         { }
    ;

package:
        PACKAGE                                 { }
    ;

imports:
        IMPORT                                  { ps->ProcessImport($1); }
    |   IMPORT imports                          { ps->ProcessImport($1); }
    ;

document_items:
                                                { $$ = NULL; }
    |   document_items declaration              {
                                                    if ($2 == NULL) {
                                                        // error cases only
                                                        $$ = $1;
                                                    } else {
                                                        document_item_type* p = $1;
                                                        while (p && p->next) {
                                                            p=p->next;
                                                        }
                                                        if (p) {
                                                            p->next = (document_item_type*)$2;
                                                            $$ = $1;
                                                        } else {
                                                            $$ = (document_item_type*)$2;
                                                        }
                                                    }
                                                }
    | document_items error                      {
                                                    fprintf(stderr, "%s:%d: syntax error don't know what to do with \"%s\"\n",
                                                            ps->FileName().c_str(),
                                                            $2.lineno, $2.data);
                                                    $$ = $1;
                                                }
    ;

declaration:
        parcelable_decl                            { $$ = (document_item_type*)$1; }
    |   interface_decl                             { $$ = (document_item_type*)$1; }
    ;

parcelable_decl:
        PARCELABLE IDENTIFIER ';'                   {
                                                        user_data_type* b = (user_data_type*)malloc(sizeof(user_data_type));
                                                        b->document_item.item_type = USER_DATA_TYPE;
                                                        b->document_item.next = NULL;
                                                        b->keyword_token = $1;
                                                        b->name = $2;
                                                        b->package =
                                                        strdup(ps->Package().c_str());
                                                        b->semicolon_token = $3;
                                                        b->parcelable = true;
                                                        $$ = b;
                                                    }
    |   PARCELABLE ';'                              {
                                                        fprintf(stderr, "%s:%d syntax error in parcelable declaration. Expected type name.\n",
                                                                     ps->FileName().c_str(), $1.lineno);
                                                        $$ = NULL;
                                                    }
    |   PARCELABLE error ';'                        {
                                                        fprintf(stderr, "%s:%d syntax error in parcelable declaration. Expected type name, saw \"%s\".\n",
                                                                     ps->FileName().c_str(), $2.lineno, $2.data);
                                                        $$ = NULL;
                                                    }
    ;

interface_header:
        INTERFACE                                  {
                                                        interface_type* c = (interface_type*)malloc(sizeof(interface_type));
                                                        c->document_item.item_type = INTERFACE_TYPE_BINDER;
                                                        c->document_item.next = NULL;
                                                        c->interface_token = $1;
                                                        c->oneway = false;
                                                        memset(&c->oneway_token, 0, sizeof(buffer_type));
                                                        c->comments_token = &c->interface_token;
                                                        $$ = c;
                                                   }
    |   ONEWAY INTERFACE                           {
                                                        interface_type* c = (interface_type*)malloc(sizeof(interface_type));
                                                        c->document_item.item_type = INTERFACE_TYPE_BINDER;
                                                        c->document_item.next = NULL;
                                                        c->interface_token = $2;
                                                        c->oneway = true;
                                                        c->oneway_token = $1;
                                                        c->comments_token = &c->oneway_token;
                                                        $$ = c;
                                                   }
    ;

interface_decl:
        interface_header IDENTIFIER '{' interface_items '}' { 
                                                        interface_type* c = $1;
                                                        c->name = $2;
                                                        c->package =
                                                        strdup(ps->Package().c_str());
                                                        c->open_brace_token = $3;
                                                        c->interface_items = $4;
                                                        c->close_brace_token = $5;
                                                        $$ = c;
                                                    }
    |   INTERFACE error '{' interface_items '}'     {
                                                        fprintf(stderr, "%s:%d: syntax error in interface declaration.  Expected type name, saw \"%s\"\n",
                                                                    ps->FileName().c_str(), $2.lineno, $2.data);
                                                        $$ = NULL;
                                                    }
    |   INTERFACE error '}'                {
                                                        fprintf(stderr, "%s:%d: syntax error in interface declaration.  Expected type name, saw \"%s\"\n",
                                                                    ps->FileName().c_str(), $2.lineno, $2.data);
                                                        $$ = NULL;
                                                    }

    ;

interface_items:
                                                    { $$ = NULL; }
    |   interface_items method_decl                 {
                                                        interface_item_type* p=$1;
                                                        while (p && p->next) {
                                                            p=p->next;
                                                        }
                                                        if (p) {
                                                            p->next = (interface_item_type*)$2;
                                                            $$ = $1;
                                                        } else {
                                                            $$ = (interface_item_type*)$2;
                                                        }
                                                    }
    |   interface_items error ';'                   {
                                                        fprintf(stderr, "%s:%d: syntax error before ';' (expected method declaration)\n",
                                                                    ps->FileName().c_str(), $3.lineno);
                                                        $$ = $1;
                                                    }
    ;

method_decl:
        type IDENTIFIER '(' arg_list ')' ';'  {
                                                        method_type *method = (method_type*)malloc(sizeof(method_type));
                                                        method->interface_item.item_type = METHOD_TYPE;
                                                        method->interface_item.next = NULL;
                                                        method->oneway = false;
                                                        method->type = $1;
                                                        memset(&method->oneway_token, 0, sizeof(buffer_type));
                                                        method->name = $2;
                                                        method->open_paren_token = $3;
                                                        method->args = $4;
                                                        method->close_paren_token = $5;
                                                        method->hasId = false;
                                                        memset(&method->equals_token, 0, sizeof(buffer_type));
                                                        memset(&method->id, 0, sizeof(buffer_type));
                                                        method->semicolon_token = $6;
                                                        method->comments_token = &method->type.type;
                                                        $$ = method;
                                                    }
    |   ONEWAY type IDENTIFIER '(' arg_list ')' ';'  {
                                                        method_type *method = (method_type*)malloc(sizeof(method_type));
                                                        method->interface_item.item_type = METHOD_TYPE;
                                                        method->interface_item.next = NULL;
                                                        method->oneway = true;
                                                        method->oneway_token = $1;
                                                        method->type = $2;
                                                        method->name = $3;
                                                        method->open_paren_token = $4;
                                                        method->args = $5;
                                                        method->close_paren_token = $6;
                                                        method->hasId = false;
                                                        memset(&method->equals_token, 0, sizeof(buffer_type));
                                                        memset(&method->id, 0, sizeof(buffer_type));
                                                        method->semicolon_token = $7;
                                                        method->comments_token = &method->oneway_token;
                                                        $$ = method;
                                                    }
    |    type IDENTIFIER '(' arg_list ')' '=' IDVALUE ';'  {
                                                        method_type *method = (method_type*)malloc(sizeof(method_type));
                                                        method->interface_item.item_type = METHOD_TYPE;
                                                        method->interface_item.next = NULL;
                                                        method->oneway = false;
                                                        memset(&method->oneway_token, 0, sizeof(buffer_type));
                                                        method->type = $1;
                                                        method->name = $2;
                                                        method->open_paren_token = $3;
                                                        method->args = $4;
                                                        method->close_paren_token = $5;
                                                        method->hasId = true;
                                                        method->equals_token = $6;
                                                        method->id = $7;
                                                        method->semicolon_token = $8;
                                                        method->comments_token = &method->type.type;
                                                        $$ = method;
                                                    }
    |   ONEWAY type IDENTIFIER '(' arg_list ')' '=' IDVALUE ';'  {
                                                        method_type *method = (method_type*)malloc(sizeof(method_type));
                                                        method->interface_item.item_type = METHOD_TYPE;
                                                        method->interface_item.next = NULL;
                                                        method->oneway = true;
                                                        method->oneway_token = $1;
                                                        method->type = $2;
                                                        method->name = $3;
                                                        method->open_paren_token = $4;
                                                        method->args = $5;
                                                        method->close_paren_token = $6;
                                                        method->hasId = true;
                                                        method->equals_token = $7;
                                                        method->id = $8;
                                                        method->semicolon_token = $9;
                                                        method->comments_token = &method->oneway_token;
                                                        $$ = method;
                                                    }
    ;

arg_list:
                                { $$ = NULL; }
    |   arg                     { $$ = $1; }
    |   arg_list ',' arg        {
                                    if ($$ != NULL) {
                                        // only NULL on error
                                        $$ = $1;
                                        arg_type *p = $1;
                                        while (p && p->next) {
                                            p=p->next;
                                        }
                                        $3->comma_token = $2;
                                        p->next = $3;
                                    }
                                }
    |   error                   {
                                    fprintf(stderr, "%s:%d: syntax error in parameter list\n",
                                            ps->FileName().c_str(), $1.lineno);
                                    $$ = NULL;
                                }
    ;

arg:
        direction type IDENTIFIER     {
                                                arg_type* arg = (arg_type*)malloc(sizeof(arg_type));
                                                memset(&arg->comma_token, 0, sizeof(buffer_type));
                                                arg->direction = $1;
                                                arg->type = $2;
                                                arg->name = $3;
                                                arg->next = NULL;
                                                $$ = arg;
                                      }
    ;

type:
        IDENTIFIER              {
                                    $$.type = $1;
                                    init_buffer_type(&$$.array_token, $1.lineno);
                                    $$.dimension = 0;
                                }
    |   IDENTIFIER ARRAY        {
                                    $$.type = $1;
                                    $$.array_token = $2;
                                    $$.dimension = count_brackets($2.data);
                                }
    |   GENERIC                 {
                                    $$.type = $1;
                                    init_buffer_type(&$$.array_token, $1.lineno);
                                    $$.dimension = 0;
                                }
    ;

direction:
                    { init_buffer_type(&$$, $$.lineno); }
    |   IN          { $$ = $1; }
    |   OUT         { $$ = $1; }
    |   INOUT       { $$ = $1; }
    ;

%%

#include <ctype.h>
#include <stdio.h>

void init_buffer_type(buffer_type* buf, int lineno)
{
    buf->lineno = lineno;
    buf->token = 0;
    buf->data = NULL;
    buf->extra = NULL;
}

static int count_brackets(const char* s)
{
    int n=0;
    while (*s) {
        if (*s == '[') n++;
        s++;
    }
    return n;
}

void yy::parser::error(const yy::parser::location_type& l, const std::string& errstr)
{
  ps->ReportError(errstr);
}
