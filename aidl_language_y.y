%{
#include "aidl_language.h"
#include "aidl_language_y.hpp"
#include "parse_helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int yylex(yy::parser::semantic_type *, yy::parser::location_type *, void *);

#define lex_scanner ps->Scanner()

using android::aidl::cpp_strdup;

%}

%parse-param { Parser* ps }
%lex-param { void *lex_scanner }

%pure-parser
%skeleton "glr.cc"

%union {
    buffer_type buffer;
    std::string *str;
    AidlType* type;
    AidlArgument* arg;
    AidlArgument::Direction direction;
    std::vector<std::unique_ptr<AidlArgument>>* arg_list;
    AidlMethod* method;
    std::vector<std::unique_ptr<AidlMethod>>* methods;
    std::vector<std::string>* strvec;
    AidlInterface* interface_obj;
    AidlParcelable* user_data;
    AidlDocumentItem* document_item;
}

%token<buffer> IDENTIFIER IDVALUE GENERIC PARCELABLE INTERFACE ONEWAY ';'
%token '(' ')' ',' '=' '[' ']' '<' '>' '.' '{' '}' PACKAGE IMPORT
%token IN OUT INOUT

%type<document_item> document_items declaration
%type<user_data> parcelable_decl
%type<methods> methods
%type<interface_obj> interface_decl
%type<method> method_decl
%type<type> type
%type<arg_list> arg_list
%type<arg> arg
%type<direction> direction
%type<str> generic_list
%type<strvec> package_name

%type<buffer> error
%%
document
 : package imports document_items
  { ps->SetDocument($3); };

package
 : {}
 | PACKAGE package_name ';'
  { ps->SetPackage($2); };

imports
 : {}
 | import imports {};

import
 : IMPORT package_name ';'
  { ps->AddImport($2, @1.begin.line); };

package_name
 : IDENTIFIER {
    $$ = new std::vector<std::string>();
    $$->push_back($1.data);
  }
 | package_name '.' IDENTIFIER
  { $$->push_back($3.data); };

document_items
 : { $$ = NULL; }
 | document_items declaration {
   $$ = $1;
   AidlDocumentItem **pos = &$$;
   while (*pos)
     pos = &(*pos)->next;
   if ($2)
     *pos = $2;
  }
 | document_items error {
    fprintf(stderr, "%s:%d: syntax error don't know what to do with \"%s\"\n",
            ps->FileName().c_str(),
            $2.lineno, $2.data);
    $$ = $1;
  };

declaration
 : parcelable_decl
  { $$ = $1; }
 | interface_decl
  { $$ = $1; };

parcelable_decl
 : PARCELABLE IDENTIFIER ';' {
    AidlParcelable* b = new AidlParcelable();
    b->item_type = USER_DATA_TYPE;
    b->keyword_token = $1;
    b->name = $2;
    b->package = cpp_strdup(ps->Package().c_str());
    b->semicolon_token = $3;
    b->parcelable = true;
    $$ = b;
  }
 | PARCELABLE ';' {
    fprintf(stderr, "%s:%d syntax error in parcelable declaration. Expected type name.\n",
            ps->FileName().c_str(), $1.lineno);
    $$ = NULL;
  }
 | PARCELABLE error ';' {
    fprintf(stderr, "%s:%d syntax error in parcelable declaration. Expected type name, saw \"%s\".\n",
            ps->FileName().c_str(), $2.lineno, $2.data);
    $$ = NULL;
  };

interface_decl
 : INTERFACE IDENTIFIER '{' methods '}' {
    $$ = new AidlInterface($2.Literal(), @2.begin.line,
        android::aidl::gather_comments($1.extra), false, $4, ps->Package());
  }
 | ONEWAY INTERFACE IDENTIFIER '{' methods '}' {
    $$ = new AidlInterface($3.Literal(), @3.begin.line,
        android::aidl::gather_comments($1.extra), true, $5, ps->Package());
  }
 | INTERFACE error '{' methods '}' {
    fprintf(stderr, "%s:%d: syntax error in interface declaration.  Expected type name, saw \"%s\"\n",
            ps->FileName().c_str(), $2.lineno, $2.data);
    $$ = NULL;
  }
 | INTERFACE error '}' {
    fprintf(stderr, "%s:%d: syntax error in interface declaration.  Expected type name, saw \"%s\"\n",
            ps->FileName().c_str(), $2.lineno, $2.data);
    $$ = NULL;
  };

methods
 :
  { $$ = new std::vector<std::unique_ptr<AidlMethod>>(); }
 | methods method_decl
  { $1->push_back(std::unique_ptr<AidlMethod>($2)); }
 | methods error ';' {
    fprintf(stderr, "%s:%d: syntax error before ';' "
                    "(expected method declaration)\n",
            ps->FileName().c_str(), $3.lineno);
    $$ = $1;
  };

method_decl
 : type IDENTIFIER '(' arg_list ')' ';' {
    $$ = new AidlMethod(false, $1, $2.Literal(), $4, @2.begin.line,
                        $1->GetComments());
  }
 | ONEWAY type IDENTIFIER '(' arg_list ')' ';' {
    $$ = new AidlMethod(true, $2, $3.Literal(), $5, @3.begin.line,
                        android::aidl::gather_comments($1.extra));
  }
 | type IDENTIFIER '(' arg_list ')' '=' IDVALUE ';' {
    $$ = new AidlMethod(false, $1, $2.Literal(), $4, @2.begin.line,
                        $1->GetComments(), std::stoi($7.data));
  }
 | ONEWAY type IDENTIFIER '(' arg_list ')' '=' IDVALUE ';' {
    $$ = new AidlMethod(true, $2, $3.Literal(), $5, @3.begin.line,
                        android::aidl::gather_comments($1.extra),
                        std::stoi($8.data));
  };

arg_list
 :
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

arg
 : direction type IDENTIFIER
  { $$ = new AidlArgument($1, $2, $3.data, @3.begin.line); };
 | type IDENTIFIER
  { $$ = new AidlArgument($1, $2.data, @2.begin.line); };

type
 : IDENTIFIER {
    $$ = new AidlType($1.data, @1.begin.line,
                      android::aidl::gather_comments($1.extra), false);
  }
 | IDENTIFIER '[' ']' {
    $$ = new AidlType($1.data,
                      @1.begin.line, android::aidl::gather_comments($1.extra),
                      true);
  }
 | IDENTIFIER '<' generic_list '>' {
    $$ = new AidlType(std::string($1.data) + "<" + *$3 + ">", @1.begin.line,
                      android::aidl::gather_comments($1.extra), false);
    delete $3;
  };

generic_list
 : IDENTIFIER
  { $$ = new std::string($1.data); }
 | generic_list ',' IDENTIFIER {
    $$ = new std::string(*$1 + "," + std::string($3.data));
    delete $1;
  };

direction
 : IN
  { $$ = AidlArgument::IN_DIR; }
 | OUT
  { $$ = AidlArgument::OUT_DIR; }
 | INOUT
  { $$ = AidlArgument::INOUT_DIR; };

%%

#include <ctype.h>
#include <stdio.h>

void yy::parser::error(const yy::parser::location_type& l, const std::string& errstr)
{
  ps->ReportError(errstr);
}
