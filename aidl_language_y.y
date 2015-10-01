%{
#include "aidl_language.h"
#include "aidl_language_y.hpp"
#include "parse_helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int yylex(yy::parser::semantic_type *, yy::parser::location_type *, void *);

static int count_brackets(const char*);

#define lex_scanner ps->Scanner()

using android::aidl::cpp_strdup;

%}

%parse-param { Parser* ps }
%lex-param { void *lex_scanner }

%pure-parser
%skeleton "glr.cc"

%union {
    buffer_type buffer;
    AidlType* type;
    AidlArgument* arg;
    AidlArgument::Direction direction;
    std::vector<std::unique_ptr<AidlArgument>>* arg_list;
    method_type* method;
    interface_type* interface_obj;
    user_data_type* user_data;
    document_item_type* document_item;
}

%token<buffer> IMPORT PACKAGE IDENTIFIER IDVALUE GENERIC ARRAY PARCELABLE
%token<buffer> ONEWAY INTERFACE ';' '{' '}' '(' ')' ',' '='
%token IN OUT INOUT 

%type<document_item> document_items declaration
%type<user_data> parcelable_decl
%type<method> interface_items
%type<interface_obj> interface_decl interface_header
%type<method> method_decl
%type<type> type
%type<arg_list> arg_list
%type<arg> arg
%type<direction> direction

%type<buffer> error
%%
document:
        document_items                          { ps->SetDocument($1); }
    |   headers document_items                  { ps->SetDocument($2); }
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
        IMPORT                                  { ps->AddImport($1); }
    |   IMPORT imports                          { ps->AddImport($1); }
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
                                                        user_data_type* b = new user_data_type();
                                                        b->document_item.item_type = USER_DATA_TYPE;
                                                        b->document_item.next = NULL;
                                                        b->keyword_token = $1;
                                                        b->name = $2;
                                                        b->package =
                                                        cpp_strdup(ps->Package().c_str());
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
                                                        interface_type* c = new interface_type();
                                                        c->document_item.item_type = INTERFACE_TYPE_BINDER;
                                                        c->document_item.next = NULL;
                                                        c->interface_token = $1;
                                                        c->oneway = false;
                                                        memset(&c->oneway_token, 0, sizeof(buffer_type));
                                                        c->comments_token = &c->interface_token;
                                                        $$ = c;
                                                   }
    |   ONEWAY INTERFACE                           {
                                                        interface_type* c = new interface_type();
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
                                                        cpp_strdup(ps->Package().c_str());
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
                                                        method_type* p=$1;
                                                        while (p && p->next) {
                                                            p=p->next;
                                                        }
                                                        if (p) {
                                                            p->next = $2;
                                                            $$ = $1;
                                                        } else {
                                                            $$ = $2;
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
                                                        method_type *method = new method_type();
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
                                                        method->comments_token = &method->type->type;
                                                        $$ = method;
                                                    }
    |   ONEWAY type IDENTIFIER '(' arg_list ')' ';'  {
                                                        method_type *method = new method_type();
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
                                                        method_type *method = new method_type();
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
                                                        method->comments_token = &method->type->type;
                                                        $$ = method;
                                                    }
    |   ONEWAY type IDENTIFIER '(' arg_list ')' '=' IDVALUE ';'  {
                                                        method_type *method = new method_type();
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
  { $$ = new std::vector<std::unique_ptr<AidlArgument>>(); }
 | arg {
    $$ = new std::vector<std::unique_ptr<AidlArgument>>();
    $$->push_back(std::unique_ptr<AidlArgument>($1));
  }
 | arg_list ',' arg {
    $$ = $1;
    $$->push_back(std::unique_ptr<AidlArgument>($3));
  }
 | error {
    fprintf(stderr, "%s:%d: syntax error in parameter list\n",
            ps->FileName().c_str(), $1.lineno);
    $$ = new std::vector<std::unique_ptr<AidlArgument>>();
  };

arg: direction type IDENTIFIER
  { $$ = new AidlArgument($1, $2, $3); };
 | type IDENTIFIER
  { $$ = new AidlArgument($1, $2); };

type:
        IDENTIFIER              {
				    $$ = new AidlType();
                                    $$->type = $1;
                                    $$->dimension = 0;
                                }
    |   IDENTIFIER ARRAY        {
				    $$ = new AidlType();
                                    $$->type = $1;
                                    $$->dimension = count_brackets($2.data);
                                }
    |   GENERIC                 {
				    $$ = new AidlType();
                                    $$->type = $1;
                                    $$->dimension = 0;
                                }
    ;

direction: IN
  { $$ = AidlArgument::IN_DIR; }
 | OUT
  { $$ = AidlArgument::OUT_DIR; }
 | INOUT
  { $$ = AidlArgument::INOUT_DIR; };

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
