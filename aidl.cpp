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
#include "import_resolver.h"
#include "logging.h"
#include "options.h"
#include "os.h"
#include "parse_helpers.h"
#include "type_cpp.h"
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

int check_filename(const std::string& filename,
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
                filename.c_str(), name->lineno, name->data, expected.c_str());
        return 1;
    }

    return 0;
}

int check_filenames(const std::string& filename, const document_item_type* items) {
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

bool gather_types(const std::string& filename,
                  const document_item_type* all_items,
                  TypeNamespace* types) {
  bool success = true;

  for (const document_item_type* item = all_items; item; item = item->next) {
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

int check_types(const string& filename,
                interface_type* c,
                TypeNamespace* types) {
  int err = 0;

  // Has to be a pointer due to deleting copy constructor. No idea why.
  map<string, const AidlMethod*> method_names;
  for (const auto& m : *c->methods) {
    if (!types->AddContainerType(m->GetType().GetName()) ||
        !types->IsValidReturnType(m->GetType(), filename)) {
      err = 1;  // return type is invalid
    }

    int index = 1;
    for (const auto& arg : m->GetArguments()) {
      if (!types->AddContainerType(arg->GetType().GetName()) ||
          !types->IsValidArg(*arg, index, filename)) {
        err = 1;
      }
    }

    auto it = method_names.find(m->GetName());
    // prevent duplicate methods
    if (it == method_names.end()) {
      method_names[m->GetName()] = m.get();
    } else {
      cerr << filename << ":" << m->GetLine()
           << " attempt to redefine method " << m->GetName() << "," << endl
           << filename << ":" << it->second->GetLine()
           << "    previously defined here." << endl;
      err = 1;
    }
  }
  return err;
}

void generate_dep_file(const JavaOptions& options,
                       const document_item_type* items,
                       const std::vector<std::unique_ptr<AidlImport>>& imports) {
    /* we open the file in binary mode to ensure that the same output is
     * generated on all platforms !!
     */
    FILE* to = NULL;
    string fileName;

    if (options.auto_dep_file_) {
            fileName = options.output_file_name_ + ".d";
    } else {
        fileName = options.dep_file_name_;
    }

    string output_file_name;

    // TODO: Mock IO and remove this weird stuff (b/24816077)
    if (!options.output_file_name_for_deps_test_.empty())
        output_file_name = options.output_file_name_for_deps_test_;
    else
        output_file_name = options.output_file_name_;

    to = fopen(fileName.c_str(), "wb");

    if (to == NULL) {
        cerr << "Could not open " << fileName << endl;
        printf("%m\n");
        return;
    }

    if (items->item_type == INTERFACE_TYPE_BINDER) {
        fprintf(to, "%s: \\\n", output_file_name.c_str());
    } else {
        // parcelable: there's no output file.
        fprintf(to, " : \\\n");
    }
    fprintf(to, "  %s %s\n", options.input_file_name_.c_str(), imports.empty() ? "" : "\\");

    bool first = true;
    for (const auto& import : imports) {
        if (! first) {
          fprintf(to, " \\\n");
        }
        first = false;

        if (! import->GetFilename().empty()) {
            fprintf(to, "  %s", import->GetFilename().c_str());
        }
    }

    fprintf(to, first ? "\n" : "\n\n");

    // Output "<input_aidl_file>: " so make won't fail if the input .aidl file
    // has been deleted, moved or renamed in incremental build.
    fprintf(to, "%s :\n", options.input_file_name_.c_str());

    // Output "<imported_file>: " so make won't fail if the imported file has
    // been deleted, moved or renamed in incremental build.
    for (const auto& import : imports) {
        if (! import->GetFilename().empty()) {
            fprintf(to, "%s :\n", import->GetFilename().c_str());
        }
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
            user_data_type* parcl = new user_data_type();
            memset(parcl, 0, sizeof(user_data_type));
            parcl->document_item.item_type = USER_DATA_TYPE;
            parcl->keyword_token.lineno = lineno;
            parcl->keyword_token.data = cpp_strdup(type);
            parcl->package = packagename ? cpp_strdup(packagename) : NULL;
            parcl->name.lineno = lineno;
            parcl->name.data = cpp_strdup(classname);
            parcl->semicolon_token.lineno = lineno;
            parcl->semicolon_token.data = cpp_strdup(";");
            parcl->parcelable = true;
            doc = (document_item_type*)parcl;
        }
        else if (0 == strcmp("interface", type)) {
            interface_type* iface = new interface_type();
            memset(iface, 0, sizeof(interface_type));
            iface->document_item.item_type = INTERFACE_TYPE_BINDER;
            iface->interface_token.lineno = lineno;
            iface->interface_token.data = cpp_strdup(type);
            iface->package = packagename ? cpp_strdup(packagename) : NULL;
            iface->name.lineno = lineno;
            iface->name.data = cpp_strdup(classname);
            iface->open_brace_token.lineno = lineno;
            iface->open_brace_token.data = cpp_strdup("{");
            iface->close_brace_token.lineno = lineno;
            iface->close_brace_token.data = cpp_strdup("}");
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
                                const std::vector<std::unique_ptr<AidlMethod>>& items) {
    // Check whether there are any methods with manually assigned id's and any that are not.
    // Either all method id's must be manually assigned or all of them must not.
    // Also, check for duplicates of user set id's and that the id's are within the proper bounds.
    set<int> usedIds;
    bool hasUnassignedIds = false;
    bool hasAssignedIds = false;
    for (const auto& item : items) {
        if (item->HasId()) {
            hasAssignedIds = true;
            // Ensure that the user set id is not duplicated.
            if (usedIds.find(item->GetId()) != usedIds.end()) {
                // We found a duplicate id, so throw an error.
                fprintf(stderr,
                        "%s:%d Found duplicate method id (%d) for method: %s\n",
                        filename, item->GetLine(),
                        item->GetId(), item->GetName().c_str());
                return 1;
            }
            // Ensure that the user set id is within the appropriate limits
            if (item->GetId() < kMinUserSetMethodId ||
                    item->GetId() > kMaxUserSetMethodId) {
                fprintf(stderr, "%s:%d Found out of bounds id (%d) for method: %s\n",
                        filename, item->GetLine(),
                        item->GetId(), item->GetName().c_str());
                fprintf(stderr, "    Value for id must be between %d and %d inclusive.\n",
                        kMinUserSetMethodId, kMaxUserSetMethodId);
                return 1;
            }
            usedIds.insert(item->GetId());
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

    // In the case that all methods have unassigned id's, set a unique id for them.
    if (hasUnassignedIds) {
        int newId = 0;
        for (const auto& item : items) {
            item->SetId(newId++);
        }
    }

    // success
    return 0;
}

}  // namespace

namespace internals {

int load_and_validate_aidl(const std::vector<std::string> preprocessed_files,
                           const std::vector<std::string> import_paths,
                           const std::string& input_file_name,
                           const IoDelegate& io_delegate,
                           TypeNamespace* types,
                           interface_type** returned_interface,
                           std::vector<std::unique_ptr<AidlImport>>* returned_imports) {
  int err = 0;

  // import the preprocessed file
  for (const string& s : preprocessed_files) {
    err |= parse_preprocessed_file(s, types);
  }
  if (err != 0) {
    return err;
  }

  // parse the input file
  Parser p{io_delegate};
  if (!p.ParseFile(input_file_name)) {
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
    return 1;
  }
  interface_type* interface = (interface_type*)parsed_doc;
  err |= check_filename(input_file_name.c_str(),
                        interface->package, &interface->name);

  // parse the imports of the input file
  ImportResolver import_resolver{io_delegate, import_paths};
  for (auto& import : p.GetImports()) {
    if (types->HasType(import->GetNeededClass())) {
      // There are places in the Android tree where an import doesn't resolve,
      // but we'll pick the type up through the preprocessed types.
      // This seems like an error, but legacy support demands we support it...
      continue;
    }
    string import_path = import_resolver.FindImportFile(import->GetNeededClass());
    if (import_path.empty()) {
      cerr << import->GetFileFrom() << ":" << import->GetLine()
           << ": couldn't find import for class "
           << import->GetNeededClass() << endl;
      err |= 1;
      continue;
    }
    import->SetFilename(import_path);

    Parser p{io_delegate};
    if (!p.ParseFile(import->GetFilename())) {
      cerr << "error while parsing import for class "
           << import->GetNeededClass() << endl;
      err |= 1;
      continue;
    }

    import->SetDocument(p.GetDocument());
    err |= check_filenames(import->GetFilename(), import->GetDocument());
  }
  if (err != 0) {
    return err;
  }

  // gather the types that have been declared
  if (!gather_types(input_file_name.c_str(), parsed_doc, types)) {
    err |= 1;
  }
  for (const auto& import : p.GetImports()) {
    if (!gather_types(import->GetFilename(), import->GetDocument(), types)) {
      err |= 1;
    }
  }

  // check the referenced types in parsed_doc to make sure we've imported them
  err |= check_types(input_file_name, interface, types);


  // assign method ids and validate.
  err |= check_and_assign_method_ids(input_file_name.c_str(),
                                     *interface->methods);

  // after this, there shouldn't be any more errors because of the
  // input.
  if (err != 0) {
    return err;
  }

  if (returned_interface)
      *returned_interface = interface;
  else
      delete interface;

  if (returned_imports)
    p.ReleaseImports(returned_imports);

  return 0;
}

} // namespace internals

int compile_aidl_to_cpp(const CppOptions& options,
                        const IoDelegate& io_delegate) {
  interface_type* interface = nullptr;
  std::vector<std::unique_ptr<AidlImport>> imports;
  unique_ptr<cpp::TypeNamespace> types(new cpp::TypeNamespace());
  int err = internals::load_and_validate_aidl(
      std::vector<std::string>{},  // no preprocessed files
      options.ImportPaths(),
      options.InputFileName(),
      io_delegate,
      types.get(),
      &interface,
      &imports);
  if (err != 0) {
    return err;
  }

  // TODO(wiley) b/23600457 generate a dependency file if requested with -b

  return (cpp::GenerateCpp(options, *types, *interface)) ? 0 : 1;
}

int compile_aidl_to_java(const JavaOptions& options,
                         const IoDelegate& io_delegate) {
  interface_type* interface = nullptr;
  std::vector<std::unique_ptr<AidlImport>> imports;
  unique_ptr<java::JavaTypeNamespace> types(new java::JavaTypeNamespace());
  int err = internals::load_and_validate_aidl(
      options.preprocessed_files_,
      options.import_paths_,
      options.input_file_name_,
      io_delegate,
      types.get(),
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
                      interface, types.get());

  return err;
}

int preprocess_aidl(const JavaOptions& options,
                    const IoDelegate& io_delegate) {
    vector<string> lines;

    // read files
    int N = options.files_to_preprocess_.size();
    for (int i=0; i<N; i++) {
        Parser p{io_delegate};
        if (!p.ParseFile(options.files_to_preprocess_[i]))
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
