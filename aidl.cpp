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

#include "aidl.h"

#include <fcntl.h>
#include <iostream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#include <sys/stat.h>
#endif


#include "aidl_language.h"
#include "generate_cpp.h"
#include "generate_java.h"
#include "logging.h"
#include "options.h"
#include "os.h"
#include "parse_helpers.h"
#include "search_path.h"
#include "type_java.h"
#include "type_namespace.h"

#ifndef O_BINARY
#  define O_BINARY  0
#endif

using std::cerr;
using std::endl;
using std::map;
using std::set;
using std::string;
using std::unique_ptr;
using std::vector;

namespace android {
namespace aidl {
namespace {

// The following are gotten as the offset from the allowable id's between
// android.os.IBinder.FIRST_CALL_TRANSACTION=1 and
// android.os.IBinder.LAST_CALL_TRANSACTION=16777215
const int kMinUserSetMethodId = 0;
const int kMaxUserSetMethodId = 16777214;

int check_filename(const char* filename,
                   const char* package,
                   buffer_type* name) {
    const char* p;
    string expected;
    string fn;
    size_t len;
    char cwd[MAXPATHLEN];
    bool valid = false;

#ifdef _WIN32
    if (isalpha(filename[0]) && filename[1] == ':'
        && filename[2] == OS_PATH_SEPARATOR) {
#else
    if (filename[0] == OS_PATH_SEPARATOR) {
#endif
        fn = filename;
    } else {
        fn = getcwd(cwd, sizeof(cwd));
        len = fn.length();
        if (fn[len-1] != OS_PATH_SEPARATOR) {
            fn += OS_PATH_SEPARATOR;
        }
        fn += filename;
    }

    if (package) {
        expected = package;
        expected += '.';
    }

    len = expected.length();
    for (size_t i=0; i<len; i++) {
        if (expected[i] == '.') {
            expected[i] = OS_PATH_SEPARATOR;
        }
    }

    p = strchr(name->data, '.');
    len = p ? p-name->data : strlen(name->data);
    expected.append(name->data, len);

    expected += ".aidl";

    len = fn.length();
    valid = (len >= expected.length());

    if (valid) {
        p = fn.c_str() + (len - expected.length());

#ifdef _WIN32
        if (OS_PATH_SEPARATOR != '/') {
            // Input filename under cygwin most likely has / separators
            // whereas the expected string uses \\ separators. Adjust
            // them accordingly.
          for (char *c = const_cast<char *>(p); *c; ++c) {
                if (*c == '/') *c = OS_PATH_SEPARATOR;
            }
        }
#endif

        // aidl assumes case-insensitivity on Mac Os and Windows.
#if defined(__linux__)
        valid = (expected == p);
#else
        valid = !strcasecmp(expected.c_str(), p);
#endif
    }

    if (!valid) {
        fprintf(stderr, "%s:%d interface %s should be declared in a file"
                " called %s.\n",
                filename, name->lineno, name->data, expected.c_str());
        return 1;
    }

    return 0;
}

int check_filenames(const char* filename, document_item_type* items) {
    int err = 0;
    while (items) {
        if (items->item_type == USER_DATA_TYPE) {
            user_data_type* p = (user_data_type*)items;
            err |= check_filename(filename, p->package, &p->name);
        }
        else if (items->item_type == INTERFACE_TYPE_BINDER) {
            interface_type* c = (interface_type*)items;
            err |= check_filename(filename, c->package, &c->name);
        }
        else {
            fprintf(stderr, "aidl: internal error unkown document type %d.\n",
                        items->item_type);
            return 1;
        }
        items = items->next;
    }
    return err;
}

char* rfind(char* str, char c) {
    char* p = str + strlen(str) - 1;
    while (p >= str) {
        if (*p == c) {
            return p;
        }
        p--;
    }
    return NULL;
}

bool gather_types(const char* raw_filename,
                  document_item_type* all_items,
                  TypeNamespace* types) {
  bool success = true;
  if (raw_filename == nullptr)
    raw_filename = "";
  const std::string filename{raw_filename};

  for (document_item_type* item = all_items; item; item = item->next) {
    if (item->item_type == USER_DATA_TYPE) {
      user_data_type* p = (user_data_type*)item;
      success &= types->AddParcelableType(p, filename);
    } else if (item->item_type == INTERFACE_TYPE_BINDER) {
      interface_type* c = (interface_type*)item;
      success &= types->AddBinderType(c, filename);
    } else {
      LOG(FATAL) << "internal error";
    }
  }
  return success;
}

int check_method(const char* filename, method_type* m, TypeNamespace* types) {
    int err = 0;

    // return type
    const Type* returnType = types->Search(m->type.type.data);
    if (returnType == NULL) {
        fprintf(stderr, "%s:%d unknown return type %s\n", filename,
                    m->type.type.lineno, m->type.type.data);
        err = 1;
        return err;
    }

    if (!returnType->CanWriteToParcel()) {
        fprintf(stderr, "%s:%d return type %s can't be marshalled.\n", filename,
                        m->type.type.lineno, m->type.type.data);
        err = 1;
    }

    if (m->type.dimension > 0 && !returnType->CanBeArray()) {
        fprintf(stderr, "%s:%d return type %s%s can't be an array.\n", filename,
                m->type.array_token.lineno, m->type.type.data,
                m->type.array_token.data);
        err = 1;
    }

    if (m->type.dimension > 1) {
        fprintf(stderr, "%s:%d return type %s%s only one"
                " dimensional arrays are supported\n", filename,
                m->type.array_token.lineno, m->type.type.data,
                m->type.array_token.data);
        err = 1;
    }

    int index = 1;

    arg_type* arg = m->args;
    while (arg) {
        const Type* t = types->Search(arg->type.type.data);

        // check the arg type
        if (t == NULL) {
            fprintf(stderr, "%s:%d parameter %s (%d) unknown type %s\n",
                    filename, m->type.type.lineno, arg->name.data, index,
                    arg->type.type.data);
            err = 1;
            goto next;
        }

        if (!t->CanWriteToParcel()) {
            fprintf(stderr, "%s:%d parameter %d: '%s %s' can't be marshalled.\n",
                        filename, m->type.type.lineno, index,
                        arg->type.type.data, arg->name.data);
            err = 1;
        }

        if (arg->direction.data == NULL
                && (arg->type.dimension != 0 || t->CanBeOutParameter())) {
            fprintf(stderr, "%s:%d parameter %d: '%s %s' can be an out"
                                " parameter, so you must declare it as in,"
                                " out or inout.\n",
                        filename, m->type.type.lineno, index,
                        arg->type.type.data, arg->name.data);
            err = 1;
        }

        if (convert_direction(arg->direction.data) != IN_PARAMETER
                && !t->CanBeOutParameter()
                && arg->type.dimension == 0) {
            fprintf(stderr, "%s:%d parameter %d: '%s %s %s' can only be an in"
                            " parameter.\n",
                        filename, m->type.type.lineno, index,
                        arg->direction.data, arg->type.type.data,
                        arg->name.data);
            err = 1;
        }

        if (arg->type.dimension > 0 && !t->CanBeArray()) {
            fprintf(stderr, "%s:%d parameter %d: '%s %s%s %s' can't be an"
                    " array.\n", filename,
                    m->type.array_token.lineno, index, arg->direction.data,
                    arg->type.type.data, arg->type.array_token.data,
                    arg->name.data);
            err = 1;
        }

        if (arg->type.dimension > 1) {
            fprintf(stderr, "%s:%d parameter %d: '%s %s%s %s' only one"
                    " dimensional arrays are supported\n", filename,
                    m->type.array_token.lineno, index, arg->direction.data,
                    arg->type.type.data, arg->type.array_token.data,
                    arg->name.data);
            err = 1;
        }

        // check that the name doesn't match a keyword
        if (is_java_keyword(arg->name.data)) {
            fprintf(stderr, "%s:%d parameter %d %s is named the same as a"
                    " Java or aidl keyword\n",
                    filename, m->name.lineno, index, arg->name.data);
            err = 1;
        }

        // Reserve a namespace for internal use
        if (!strncmp(arg->name.data, "_aidl", 5)) {
            fprintf(stderr, "%s:%d parameter %d %s cannot begin with"
                    " '_aidl'\n",
                    filename, m->name.lineno, index, arg->name.data);
            err = 1;
        }

next:
        index++;
        arg = arg->next;
    }

    return err;
}

int check_types(const char* filename,
                document_item_type* items,
                TypeNamespace* types) {
    int err = 0;
    while (items) {
        // (nothing to check for USER_DATA_TYPE)
        if (items->item_type == INTERFACE_TYPE_BINDER) {
            map<string,method_type*> methodNames;
            interface_type* c = (interface_type*)items;

            interface_item_type* member = c->interface_items;
            while (member) {
                if (member->item_type == METHOD_TYPE) {
                    method_type* m = (method_type*)member;

                    err |= check_method(filename, m, types);

                    // prevent duplicate methods
                    if (methodNames.find(m->name.data) == methodNames.end()) {
                        methodNames[m->name.data] = m;
                    } else {
                        fprintf(stderr,"%s:%d attempt to redefine method %s,\n",
                                filename, m->name.lineno, m->name.data);
                        method_type* old = methodNames[m->name.data];
                        fprintf(stderr, "%s:%d    previously defined here.\n",
                                filename, old->name.lineno);
                        err = 1;
                    }
                }
                member = member->next;
            }
        }

        items = items->next;
    }
    return err;
}

void generate_dep_file(const JavaOptions& options,
                       const document_item_type* items,
                       import_info* import_head) {
    /* we open the file in binary mode to ensure that the same output is
     * generated on all platforms !!
     */
    FILE* to = NULL;
    if (options.auto_dep_file_) {
        string fileName = options.output_file_name_ + ".d";
        to = fopen(fileName.c_str(), "wb");
    } else {
        to = fopen(options.dep_file_name_.c_str(), "wb");
    }

    if (to == NULL) {
        return;
    }

    const char* slash = "\\";
    import_info* import = import_head;
    if (import == NULL) {
        slash = "";
    }

    if (items->item_type == INTERFACE_TYPE_BINDER) {
        fprintf(to, "%s: \\\n", options.output_file_name_.c_str());
    } else {
        // parcelable: there's no output file.
        fprintf(to, " : \\\n");
    }
    fprintf(to, "  %s %s\n", options.input_file_name_.c_str(), slash);

    while (import) {
        if (import->next == NULL) {
            slash = "";
        }
        if (import->filename) {
            fprintf(to, "  %s %s\n", import->filename, slash);
        }
        import = import->next;
    }

    fprintf(to, "\n");

    // Output "<input_aidl_file>: " so make won't fail if the input .aidl file
    // has been deleted, moved or renamed in incremental build.
    fprintf(to, "%s :\n", options.input_file_name_.c_str());

    // Output "<imported_file>: " so make won't fail if the imported file has
    // been deleted, moved or renamed in incremental build.
    import = import_head;
    while (import) {
        if (import->filename) {
            fprintf(to, "%s :\n", import->filename);
        }
        import = import->next;
    }

    fclose(to);
}

string generate_outputFileName2(const JavaOptions& options,
                                const buffer_type& name,
                                const char* package) {
    string result;

    // create the path to the destination folder based on the
    // interface package name
    result = options.output_base_folder_;
    result += OS_PATH_SEPARATOR;

    string packageStr = package;
    size_t len = packageStr.length();
    for (size_t i=0; i<len; i++) {
        if (packageStr[i] == '.') {
            packageStr[i] = OS_PATH_SEPARATOR;
        }
    }

    result += packageStr;

    // add the filename by replacing the .aidl extension to .java
    const char* p = strchr(name.data, '.');
    len = p ? p-name.data : strlen(name.data);

    result += OS_PATH_SEPARATOR;
    result.append(name.data, len);
    result += ".java";

    return result;
}

string generate_outputFileName(const JavaOptions& options,
                               const document_item_type* items) {
    // items has already been checked to have only one interface.
    if (items->item_type == INTERFACE_TYPE_BINDER) {
        interface_type* type = (interface_type*)items;

        return generate_outputFileName2(options, type->name, type->package);
    } else if (items->item_type == USER_DATA_TYPE) {
        user_data_type* type = (user_data_type*)items;
        return generate_outputFileName2(options, type->name, type->package);
    }

    // I don't think we can come here, but safer than returning NULL.
    string result;
    return result;
}


void check_outputFilePath(const string& path) {
    size_t len = path.length();
    for (size_t i=0; i<len ; i++) {
        if (path[i] == OS_PATH_SEPARATOR) {
            string p = path.substr(0, i);
            if (access(path.data(), F_OK) != 0) {
#ifdef _WIN32
                _mkdir(p.data());
#else
                mkdir(p.data(), S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP);
#endif
            }
        }
    }
}


int parse_preprocessed_file(const string& filename, TypeNamespace* types) {
    FILE* f = fopen(filename.c_str(), "rb");
    if (f == NULL) {
        fprintf(stderr, "aidl: can't open preprocessed file: %s\n",
                filename.c_str());
        return 1;
    }

    int lineno = 1;
    char line[1024];
    char type[1024];
    char fullname[1024];
    while (fgets(line, sizeof(line), f)) {
        // skip comments and empty lines
        if (!line[0] || strncmp(line, "//", 2) == 0) {
          continue;
        }

        sscanf(line, "%s %[^; \r\n\t];", type, fullname);

        char* packagename;
        char* classname = rfind(fullname, '.');
        if (classname != NULL) {
            *classname = '\0';
            classname++;
            packagename = fullname;
        } else {
            classname = fullname;
            packagename = NULL;
        }

        //printf("%s:%d:...%s...%s...%s...\n", filename.c_str(), lineno,
        //        type, packagename, classname);
        document_item_type* doc;

        if (0 == strcmp("parcelable", type)) {
            user_data_type* parcl = (user_data_type*)malloc(
                    sizeof(user_data_type));
            memset(parcl, 0, sizeof(user_data_type));
            parcl->document_item.item_type = USER_DATA_TYPE;
            parcl->keyword_token.lineno = lineno;
            parcl->keyword_token.data = strdup(type);
            parcl->package = packagename ? strdup(packagename) : NULL;
            parcl->name.lineno = lineno;
            parcl->name.data = strdup(classname);
            parcl->semicolon_token.lineno = lineno;
            parcl->semicolon_token.data = strdup(";");
            parcl->parcelable = true;
            doc = (document_item_type*)parcl;
        }
        else if (0 == strcmp("interface", type)) {
            interface_type* iface = (interface_type*)malloc(
                    sizeof(interface_type));
            memset(iface, 0, sizeof(interface_type));
            iface->document_item.item_type = INTERFACE_TYPE_BINDER;
            iface->interface_token.lineno = lineno;
            iface->interface_token.data = strdup(type);
            iface->package = packagename ? strdup(packagename) : NULL;
            iface->name.lineno = lineno;
            iface->name.data = strdup(classname);
            iface->open_brace_token.lineno = lineno;
            iface->open_brace_token.data = strdup("{");
            iface->close_brace_token.lineno = lineno;
            iface->close_brace_token.data = strdup("}");
            doc = (document_item_type*)iface;
        }
        else {
            fprintf(stderr, "%s:%d: bad type in line: %s\n",
                    filename.c_str(), lineno, line);
            fclose(f);
            return 1;
        }
        if (!gather_types(filename.c_str(), doc, types)) {
            fprintf(stderr, "Failed to gather types for preprocessed aidl.\n");
            fclose(f);
            return 1;
        }
        lineno++;
    }

    if (!feof(f)) {
        fprintf(stderr, "%s:%d: error reading file, line to long.\n",
                filename.c_str(), lineno);
        return 1;
    }

    fclose(f);
    return 0;
}

int check_and_assign_method_ids(const char * filename,
                                interface_item_type* first_item) {
    // Check whether there are any methods with manually assigned id's and any that are not.
    // Either all method id's must be manually assigned or all of them must not.
    // Also, check for duplicates of user set id's and that the id's are within the proper bounds.
    set<int> usedIds;
    interface_item_type* item = first_item;
    bool hasUnassignedIds = false;
    bool hasAssignedIds = false;
    while (item != NULL) {
        if (item->item_type == METHOD_TYPE) {
            method_type* method_item = (method_type*)item;
            if (method_item->hasId) {
                hasAssignedIds = true;
                method_item->assigned_id = atoi(method_item->id.data);
                // Ensure that the user set id is not duplicated.
                if (usedIds.find(method_item->assigned_id) != usedIds.end()) {
                    // We found a duplicate id, so throw an error.
                    fprintf(stderr,
                            "%s:%d Found duplicate method id (%d) for method: %s\n",
                            filename, method_item->id.lineno,
                            method_item->assigned_id, method_item->name.data);
                    return 1;
                }
                // Ensure that the user set id is within the appropriate limits
                if (method_item->assigned_id < kMinUserSetMethodId ||
                        method_item->assigned_id > kMaxUserSetMethodId) {
                    fprintf(stderr, "%s:%d Found out of bounds id (%d) for method: %s\n",
                            filename, method_item->id.lineno,
                            method_item->assigned_id, method_item->name.data);
                    fprintf(stderr, "    Value for id must be between %d and %d inclusive.\n",
                            kMinUserSetMethodId, kMaxUserSetMethodId);
                    return 1;
                }
                usedIds.insert(method_item->assigned_id);
            } else {
                hasUnassignedIds = true;
            }
            if (hasAssignedIds && hasUnassignedIds) {
                fprintf(stderr,
                        "%s: You must either assign id's to all methods or to none of them.\n",
                        filename);
                return 1;
            }
        }
        item = item->next;
    }

    // In the case that all methods have unassigned id's, set a unique id for them.
    if (hasUnassignedIds) {
        int newId = 0;
        item = first_item;
        while (item != NULL) {
            if (item->item_type == METHOD_TYPE) {
                method_type* method_item = (method_type*)item;
                method_item->assigned_id = newId++;
            }
            item = item->next;
        }
    }

    // success
    return 0;
}

int load_and_validate_aidl(const std::vector<std::string> preprocessed_files,
                           const std::vector<std::string> import_paths,
                           const std::string& input_file_name,
                           TypeNamespace* types,
                           interface_type** returned_interface,
                           import_info** returned_imports) {
  int err = 0;

  set_import_paths(import_paths);

  // import the preprocessed file
  for (const string& s : preprocessed_files) {
    err |= parse_preprocessed_file(s, types);
  }
  if (err != 0) {
    return err;
  }

  // parse the input file
  Parser p{input_file_name};
  if (!p.OpenFileFromDisk() || !p.RunParser()) {
    return 1;
  }
  document_item_type* parsed_doc = p.GetDocument();
  // We could in theory declare parcelables in the same file as the interface.
  // In practice, those parcelables would have to have the same name as
  // the interface, since this was originally written to support Java, with its
  // packages and names that correspond to file system structure.
  // Since we can't have two distinct classes with the same name and package,
  // we can't actually declare parcelables in the same file.
  if (parsed_doc == nullptr ||
      parsed_doc->item_type != INTERFACE_TYPE_BINDER ||
      parsed_doc->next != nullptr) {
    cerr << "aidl expects exactly one interface per input file";
    err |= 1;
  }
  interface_type* interface = (interface_type*)parsed_doc;
  err |= check_filename(input_file_name.c_str(),
                        interface->package, &interface->name);

  // parse the imports of the input file
  for (import_info* import = p.GetImports(); import; import = import->next) {
    if (types->Find(import->neededClass) != NULL) {
      continue;
    }
    import->filename = find_import_file(import->neededClass);
    if (!import->filename) {
      cerr << import->from << ":" << import->statement.lineno
           << ": couldn't find import for class "
           << import->neededClass << endl;
      err |= 1;
      continue;
    }
    Parser p{import->filename};
    if (!p.OpenFileFromDisk() || !p.RunParser() || p.GetDocument() == nullptr) {
      cerr << "error while parsing import for class "
           << import->neededClass << endl;
      err |= 1;
      continue;
    }
    import->doc = p.GetDocument();
    err |= check_filenames(import->filename, import->doc);
  }
  if (err != 0) {
    return err;
  }

  // gather the types that have been declared
  if (!gather_types(input_file_name.c_str(), parsed_doc, types)) {
    err |= 1;
  }
  for (import_info* import = p.GetImports(); import; import = import->next) {
    if (!gather_types(import->filename, import->doc, types)) {
      err |= 1;
    }
  }

  // check the referenced types in parsed_doc to make sure we've imported them
  err |= check_types(input_file_name.c_str(), parsed_doc, types);


  // assign method ids and validate.
  err |= check_and_assign_method_ids(input_file_name.c_str(),
                                     interface->interface_items);

  // after this, there shouldn't be any more errors because of the
  // input.
  if (err != 0) {
    return err;
  }

  *returned_interface = interface;
  *returned_imports = p.GetImports();
  return 0;
}

}  // namespace

int compile_aidl_to_cpp(const CppOptions& options) {
  interface_type* interface = nullptr;
  import_info* imports = nullptr;
  register_base_types();
  JavaTypeNamespace* types = &NAMES;
  int err = load_and_validate_aidl(std::vector<std::string>{},
                                   options.ImportPaths(),
                                   options.InputFileName(),
                                   types,
                                   &interface,
                                   &imports);
  if (err != 0) {
    return err;
  }

  // TODO(wiley) b/23600457 generate a dependency file if requested with -b

  return (GenerateCpp(options, interface)) ? 0 : 1;
}

int compile_aidl_to_java(const JavaOptions& options) {
  interface_type* interface = nullptr;
  import_info* imports = nullptr;
  register_base_types();
  JavaTypeNamespace* types = &NAMES;
  int err = load_and_validate_aidl(options.preprocessed_files_,
                                   options.import_paths_,
                                   options.input_file_name_,
                                   types,
                                   &interface,
                                   &imports);
  if (err != 0) {
    return err;
  }
  document_item_type* parsed_doc = (document_item_type*)interface;

  string output_file_name = options.output_file_name_;
  // if needed, generate the output file name from the base folder
  if (output_file_name.length() == 0 &&
      options.output_base_folder_.length() > 0) {
    output_file_name = generate_outputFileName(options, parsed_doc);
  }

  // if we were asked to, generate a make dependency file
  // unless it's a parcelable *and* it's supposed to fail on parcelable
  if (options.auto_dep_file_ || options.dep_file_name_ != "") {
    // make sure the folders of the output file all exists
    check_outputFilePath(output_file_name);
    generate_dep_file(options, parsed_doc, imports);
  }

  // make sure the folders of the output file all exists
  check_outputFilePath(output_file_name);

  err = generate_java(output_file_name, options.input_file_name_.c_str(),
                      interface, types);

  return err;
}

int preprocess_aidl(const JavaOptions& options) {
    vector<string> lines;

    // read files
    int N = options.files_to_preprocess_.size();
    for (int i=0; i<N; i++) {
        Parser p{options.files_to_preprocess_[i]};
        if (!p.OpenFileFromDisk())
            return 1;
        if (!p.RunParser())
            return 1;
        document_item_type* doc = p.GetDocument();
        string line;
        if (doc->item_type == USER_DATA_TYPE) {
            user_data_type* parcelable = (user_data_type*)doc;
            if (parcelable->parcelable) {
                line = "parcelable ";
            }
            if (parcelable->package) {
                line += parcelable->package;
                line += '.';
            }
            line += parcelable->name.data;
        } else {
            line = "interface ";
            interface_type* iface = (interface_type*)doc;
            if (iface->package) {
                line += iface->package;
                line += '.';
            }
            line += iface->name.data;
        }
        line += ";\n";
        lines.push_back(line);
    }

    // write preprocessed file
    int fd = open( options.output_file_name_.c_str(),
                   O_RDWR|O_CREAT|O_TRUNC|O_BINARY,
#ifdef _WIN32
                   _S_IREAD|_S_IWRITE);
#else
                   S_IRUSR|S_IWUSR|S_IRGRP);
#endif
    if (fd == -1) {
        fprintf(stderr, "aidl: could not open file for write: %s\n",
                options.output_file_name_.c_str());
        return 1;
    }

    N = lines.size();
    for (int i=0; i<N; i++) {
        const string& s = lines[i];
        int len = s.length();
        if (len != write(fd, s.c_str(), len)) {
            fprintf(stderr, "aidl: error writing to file %s\n",
                options.output_file_name_.c_str());
            close(fd);
            unlink(options.output_file_name_.c_str());
            return 1;
        }
    }

    close(fd);
    return 0;
}

}  // namespace android
}  // namespace aidl
