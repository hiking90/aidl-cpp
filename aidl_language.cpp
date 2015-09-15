#include "aidl_language.h"
#include "aidl_language_y.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>

#ifdef _WIN32
int isatty(int  fd)
{
    return (fd == 0);
}
#endif

using std::string;
using std::cerr;
using std::endl;

Parser *psGlobal = NULL;
ParserCallbacks* g_callbacks = NULL; // &k_parserCallbacks;
char const* g_currentPackage = NULL;


void yylex_init(void **);
void yylex_destroy(void *);
void yyset_in(FILE *f, void *);
int yyparse(Parser*);

Parser::Parser(const string& filename)
    : filename_(filename) {
  yylex_init(&scanner_);
}

Parser::~Parser() {
  yylex_destroy(scanner_);
}

string Parser::FileName() {
  return filename_;
}

string Parser::Package() {
  return g_currentPackage;
}

void Parser::ReportError(const string& err) {
  /* FIXME: We're printing out the line number as -1. We used to use yylineno
   * (which was NEVER correct even before reentrant parsing). Now we'll need
   * another way.
   */
  cerr << filename_ << ":" << -1 << ": " << err << endl;
  error_ = 1;
}

bool Parser::FoundNoErrors() {
  return error_ == 0;
}

void *Parser::Scanner() {
  return scanner_;
}

bool Parser::OpenFileFromDisk() {
  FILE *in = fopen(FileName().c_str(), "r");

  if (! in)
    return false;

  yyset_in(in, Scanner());
  return true;
}

bool Parser::RunParser() {
  int ret = yy::parser(this).parse();

  free((void *)g_currentPackage);
  g_currentPackage = NULL;

  if (error_)
    return true;

  return ret;
}

void Parser::SetDocument(document_item_type *d)
{
  document_ = d;
}

void Parser::AddImport(const buffer_type& statement)
{
  import_info* import = (import_info*)malloc(sizeof(import_info));
  memset(import, 0, sizeof(import_info));
  import->from = strdup(this->FileName().c_str());
  import->statement.lineno = statement.lineno;
  import->statement.data = strdup(statement.data);
  import->statement.extra = NULL;
  import->next = imports_;
  import->neededClass = android::aidl::parse_import_statement(statement.data);
  imports_ = import;
}

document_item_type *Parser::GetDocument() const
{
  return document_;
}

import_info *Parser::GetImports() const
{
  return imports_;
}
