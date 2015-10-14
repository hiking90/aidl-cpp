%{
#include "aidl_language.h"
#include "aidl_language_y.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int yylex(yy::parser::semantic_type *, yy::parser::location_type *, void *);

#define lex_scanner ps->Scanner()

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
    AidlQualifiedName* qname;
    AidlInterface* interface_obj;
    AidlParcelable* user_data;
}

%token<buffer> IDENTIFIER IDVALUE INTERFACE ONEWAY

%token '(' ')' ',' '=' '[' ']' '<' '>' '.' '{' '}' ';'
%token IN OUT INOUT PACKAGE IMPORT PARCELABLE

%type<user_data> parcelable_decl parcelable_decls
%type<methods> methods
%type<interface_obj> interface_decl
%type<method> method_decl
%type<type> type
%type<arg_list> arg_list
%type<arg> arg
%type<direction> direction
%type<str> generic_list
%type<qname> qualified_name

%type<buffer> error
%%
document
 : package imports parcelable_decls
  { ps->SetDocument($3); }
 | package imports interface_decl
  { ps->SetDocument($3); };

package
 : {}
 | PACKAGE qualified_name ';'
  { ps->SetPackage($2); };

imports
 : {}
 | import imports {};

import
 : IMPORT qualified_name ';'
  { ps->AddImport($2, @1.begin.line); };

qualified_name
 : IDENTIFIER {
    $$ = new AidlQualifiedName($1.data, $1.Comments());
  }
 | qualified_name '.' IDENTIFIER
  { $$ = $1;
    $$->AddTerm($3.data);
  };

parcelable_decls
 :
  { $$ = NULL; }
 | parcelable_decls parcelable_decl {
   $$ = $1;
   AidlParcelable **pos = &$$;
   while (*pos)
     pos = &(*pos)->next;
   if ($2)
     *pos = $2;
  }
 | parcelable_decls error {
    fprintf(stderr, "%s:%d: syntax error don't know what to do with \"%s\"\n",
            ps->FileName().c_str(),
            @2.begin.line, $2.data);
    $$ = $1;
  };

parcelable_decl
 : PARCELABLE qualified_name ';' {
    $$ = new AidlParcelable($2, @2.begin.line, ps->Package());
  }
 | PARCELABLE ';' {
    fprintf(stderr, "%s:%d syntax error in parcelable declaration. Expected type name.\n",
            ps->FileName().c_str(), @1.begin.line);
    $$ = NULL;
  }
 | PARCELABLE error ';' {
    fprintf(stderr, "%s:%d syntax error in parcelable declaration. Expected type name, saw \"%s\".\n",
            ps->FileName().c_str(), @2.begin.line, $2.data);
    $$ = NULL;
  };

interface_decl
 : INTERFACE IDENTIFIER '{' methods '}' {
    $$ = new AidlInterface($2.Literal(), @2.begin.line, $1.Comments(),
                           false, $4, ps->Package());
  }
 | ONEWAY INTERFACE IDENTIFIER '{' methods '}' {
    $$ = new AidlInterface($3.Literal(), @3.begin.line, $1.Comments(),
                           true, $5, ps->Package());
  }
 | INTERFACE error '{' methods '}' {
    fprintf(stderr, "%s:%d: syntax error in interface declaration.  Expected type name, saw \"%s\"\n",
            ps->FileName().c_str(), @2.begin.line, $2.data);
    $$ = NULL;
  }
 | INTERFACE error '}' {
    fprintf(stderr, "%s:%d: syntax error in interface declaration.  Expected type name, saw \"%s\"\n",
            ps->FileName().c_str(), @2.begin.line, $2.data);
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
            ps->FileName().c_str(), @3.begin.line);
    $$ = $1;
  };

method_decl
 : type IDENTIFIER '(' arg_list ')' ';' {
    $$ = new AidlMethod(false, $1, $2.Literal(), $4, @2.begin.line,
                        $1->GetComments());
  }
 | ONEWAY type IDENTIFIER '(' arg_list ')' ';' {
    $$ = new AidlMethod(true, $2, $3.Literal(), $5, @3.begin.line,
                        $1.Comments());
  }
 | type IDENTIFIER '(' arg_list ')' '=' IDVALUE ';' {
    $$ = new AidlMethod(false, $1, $2.Literal(), $4, @2.begin.line,
                        $1->GetComments(), std::stoi($7.data));
  }
 | ONEWAY type IDENTIFIER '(' arg_list ')' '=' IDVALUE ';' {
    $$ = new AidlMethod(true, $2, $3.Literal(), $5, @3.begin.line,
                        $1.Comments(), std::stoi($8.data));
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
            ps->FileName().c_str(), @1.begin.line);
    $$ = new std::vector<std::unique_ptr<AidlArgument>>();
  };

arg
 : direction type IDENTIFIER
  { $$ = new AidlArgument($1, $2, $3.data, @3.begin.line); };
 | type IDENTIFIER
  { $$ = new AidlArgument($1, $2.data, @2.begin.line); };

type
 : qualified_name {
    $$ = new AidlType($1->GetDotName(), @1.begin.line, $1->GetComments(),
                      false);
    delete $1;
  }
 | qualified_name '[' ']' {
    $$ = new AidlType($1->GetDotName(), @1.begin.line, $1->GetComments(),
                      true);
    delete $1;
  }
 | qualified_name '<' generic_list '>' {
    $$ = new AidlType($1->GetDotName() + "<" + *$3 + ">", @1.begin.line,
                      $1->GetComments(), false);
    delete $1;
    delete $3;
  };

generic_list
 : qualified_name {
    $$ = new std::string($1->GetDotName());
    delete $1;
  }
 | generic_list ',' qualified_name {
    $$ = new std::string(*$1 + "," + $3->GetDotName());
    delete $1;
    delete $3;
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

void yy::parser::error(const yy::parser::location_type& l,
                       const std::string& errstr) {
  ps->ReportError(errstr, l.begin.line);
}
