#include "aidl_language.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "aidl_language_y.hpp"
#include "parse_helpers.h"

#ifdef _WIN32
int isatty(int  fd)
{
    return (fd == 0);
}
#endif

using std::string;
using std::cerr;
using std::endl;
using android::aidl::cpp_strdup;

Parser *psGlobal = NULL;
ParserCallbacks* g_callbacks = NULL; // &k_parserCallbacks;
char const* g_currentPackage = NULL;


void yylex_init(void **);
void yylex_destroy(void *);
void yyset_in(FILE *f, void *);
int yyparse(Parser*);
YY_BUFFER_STATE yy_scan_string(const char *, void *);
void yy_delete_buffer(YY_BUFFER_STATE, void *);

Parser::Parser(const string& filename)
    : filename_(filename) {
  yylex_init(&scanner_);
}

Parser::~Parser() {
  if (buffer_is_valid_)
    yy_delete_buffer(buffer_, scanner_);
  yylex_destroy(scanner_);
}

AidlArgument::AidlArgument(buffer_type direction, type_type type, buffer_type name)
    : direction(direction),
      name(name),
      type(type) {}

string Parser::FileName() {
  return filename_;
}

string Parser::Package() {
  return g_currentPackage ? g_currentPackage : "";
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

void Parser::SetFileContents(const std::string& contents) {
  if (buffer_is_valid_)
    yy_delete_buffer(buffer_, scanner_);

  buffer_ = yy_scan_string(contents.c_str(), scanner_);
  buffer_is_valid_ = true;
}

bool Parser::RunParser() {
  int ret = yy::parser(this).parse();

  delete[] g_currentPackage;
  g_currentPackage = NULL;

  return ret == 0 && error_ == 0;
}

void Parser::SetDocument(document_item_type *d)
{
  document_ = d;
}

void Parser::AddImport(const buffer_type& statement)
{
  import_info* import = new import_info();
  memset(import, 0, sizeof(import_info));
  import->from = cpp_strdup(this->FileName().c_str());
  import->statement.lineno = statement.lineno;
  import->statement.data = cpp_strdup(statement.data);
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

std::string type_type::Brackets() const {
  std::string result;

  for (int i = 0; i < dimension; i++)
    result += "[]";

  return result;
}
