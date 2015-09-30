#ifndef AIDL_AIDL_LANGUAGE_H_
#define AIDL_AIDL_LANGUAGE_H_

#include <memory>
#include <string>
#include <vector>

#include <base/macros.h>

struct yy_buffer_state;
typedef yy_buffer_state* YY_BUFFER_STATE;

enum which_extra_text {
    NO_EXTRA_TEXT = 0,
    SHORT_COMMENT,
    LONG_COMMENT,
    COPY_TEXT,
    WHITESPACE
};

struct extra_text_type {
    unsigned lineno;
    which_extra_text which;
    char* data; 
    unsigned len;
    struct extra_text_type* next;
};

struct buffer_type {
  unsigned lineno;
  unsigned token;
  char *data;
  extra_text_type* extra;

  std::string Literal() const {
    return data ? std::string(data) : "";
  }
};

struct type_type {
  buffer_type type;
  buffer_type array_token;
  int dimension;

  std::string Brackets() const;
};

class AidlNode {
 public:
  AidlNode() = default;
  virtual ~AidlNode() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(AidlNode);
};

class AidlArgument : public AidlNode {
 public:
  enum Direction { IN_DIR = 1, OUT_DIR = 2, INOUT_DIR = 3 };

  AidlArgument(AidlArgument::Direction direction, type_type type, buffer_type name);
  AidlArgument(type_type type, buffer_type name);
  virtual ~AidlArgument() = default;

  Direction GetDirection() const { return direction_; }
  bool DirectionWasSpecified() const { return direction_specified_; }
  std::string GetName() const { return name_; }
  int GetLine() const { return line_; }

  std::string ToString() const;

  type_type type;

 private:
  Direction direction_;
  bool direction_specified_;
  std::string name_;
  int line_;

  DISALLOW_COPY_AND_ASSIGN(AidlArgument);
};

struct method_type {
    struct method_type *next;
    type_type type;
    bool oneway;
    buffer_type oneway_token;
    buffer_type name;
    buffer_type open_paren_token;
    std::vector<std::unique_ptr<AidlArgument>>* args;
    buffer_type close_paren_token;
    bool hasId;
    buffer_type equals_token;
    buffer_type id;
    // XXX missing comments/copy text here
    buffer_type semicolon_token;
    buffer_type* comments_token; // points into this structure, DO NOT DELETE
    int assigned_id;
};

enum {
    USER_DATA_TYPE = 12,
    INTERFACE_TYPE_BINDER
};

struct document_item_type {
    unsigned item_type;
    struct document_item_type* next;
};


struct user_data_type {
    document_item_type document_item;
    buffer_type keyword_token; // only the first one
    char* package;
    buffer_type name;
    buffer_type semicolon_token;
    bool parcelable;
};

struct interface_type {
    document_item_type document_item;
    buffer_type interface_token;
    bool oneway;
    buffer_type oneway_token;
    char* package;
    buffer_type name;
    buffer_type open_brace_token;
    method_type* interface_items;
    buffer_type close_brace_token;
    buffer_type* comments_token; // points into this structure, DO NOT DELETE
};


#if __cplusplus
extern "C" {
#endif

// callbacks from within the parser
// these functions all take ownership of the strings
struct ParserCallbacks {
    void (*document)(document_item_type* items);
    void (*import)(buffer_type* statement);
};

extern ParserCallbacks* g_callbacks;

// the package name for our current file
extern char const* g_currentPackage;

enum error_type {
    STATEMENT_INSIDE_INTERFACE
};

void init_buffer_type(buffer_type* buf, int lineno);

struct import_info {
    const char* from;
    const char* filename;
    buffer_type statement;
    const char* neededClass;
    document_item_type* doc;
    struct import_info* next;
};

class Parser {
 public:
  Parser(const std::string& filename);
  ~Parser();

  bool OpenFileFromDisk();

  // Call this instead of OpenFileFromDisk to provide the text of the file
  // directly.
  void SetFileContents(const std::string& contents);

  bool RunParser();
  void ReportError(const std::string& err);

  bool FoundNoErrors();
  std::string FileName();
  std::string Package();
  void *Scanner();

  void SetDocument(document_item_type *items);
  void AddImport(const buffer_type& statement);

  document_item_type *GetDocument() const;
  import_info *GetImports() const;

 private:
  int error_ = 0;
  std::string filename_;
  std::string package_;
  void *scanner_ = nullptr;
  document_item_type* document_ = nullptr;
  import_info* imports_ = nullptr;
  bool buffer_is_valid_ = false;
  YY_BUFFER_STATE buffer_;

  DISALLOW_COPY_AND_ASSIGN(Parser);
};

#if __cplusplus
}
#endif

#endif // AIDL_AIDL_LANGUAGE_H_
