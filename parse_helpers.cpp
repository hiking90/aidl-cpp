/*
 * Copyright (C) 2015, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "parse_helpers.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <stdlib.h>
#include <string>
#include <vector>

#include "aidl_language.h"

namespace android {
namespace aidl {

char* parse_import_statement(const char* text) {
  const char* end;
  int len;

  while (isspace(*text)) {
    text++;
  }
  while (!isspace(*text)) {
    text++;
  }
  while (isspace(*text)) {
    text++;
  }
  end = text;
  while (!isspace(*end) && *end != ';') {
    end++;
  }
  len = end - text;

  char* rv = new char[len + 1];
  memcpy(rv, text, len);
  rv[len] = '\0';

  return rv;
}

bool is_java_keyword(const char* str) {
  static const std::vector<std::string> kJavaKeywords{
      "abstract",   "assert",       "boolean",   "break",      "byte",
      "case",       "catch",        "char",      "class",      "const",
      "continue",   "default",      "do",        "double",     "else",
      "enum",       "extends",      "final",     "finally",    "float",
      "for",        "goto",         "if",        "implements", "import",
      "instanceof", "int",          "interface", "long",       "native",
      "new",        "package",      "private",   "protected",  "public",
      "return",     "short",        "static",    "strictfp",   "super",
      "switch",     "synchronized", "this",      "throw",      "throws",
      "transient",  "try",          "void",      "volatile",   "while",
      "true",       "false",        "null",
  };
  return std::find(kJavaKeywords.begin(), kJavaKeywords.end(), str) !=
      kJavaKeywords.end();
}

char* cpp_strdup(const char* in)
{
  char *out = new char[std::strlen(in) + 1];
  strcpy(out, in);
  return out;
}

}  // namespace android
}  // namespace aidl
