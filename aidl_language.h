#ifndef AIDL_AIDL_LANGUAGE_H_
#define AIDL_AIDL_LANGUAGE_H_

#include <memory>
#include <string>
#include <vector>

#include <base/macros.h>

#include <io_delegate.h>

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

class AidlNode {
 public:
  AidlNode() = default;
  virtual ~AidlNode() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(AidlNode);
};

class AidlType : public AidlNode {
 public:
  AidlType(const std::string& name, unsigned line,
           const std::string& comments, bool is_array);
  virtual ~AidlType() = default;

  const std::string& GetName() const { return name_; }
  unsigned GetLine() const { return line_; }
  bool IsArray() const { return is_array_; }
  const std::string& GetComments() const { return comments_; }

  std::string ToString() const;

 private:
  std::string name_;
  unsigned line_;
  bool is_array_;
  std::string comments_;

  DISALLOW_COPY_AND_ASSIGN(AidlType);
};

class AidlArgument : public AidlNode {
 public:
  enum Direction { IN_DIR = 1, OUT_DIR = 2, INOUT_DIR = 3 };

  AidlArgument(AidlArgument::Direction direction, AidlType* type,
               std::string name, unsigned line);
  AidlArgument(AidlType *type, std::string name, unsigned line);
  virtual ~AidlArgument() = default;

  Direction GetDirection() const { return direction_; }
  bool DirectionWasSpecified() const { return direction_specified_; }
  std::string GetName() const { return name_; }
  int GetLine() const { return line_; }
  const AidlType& GetType() const { return *type_; }

  std::string ToString() const;

 private:
  std::unique_ptr<AidlType> type_;
  Direction direction_;
  bool direction_specified_;
  std::string name_;
  unsigned line_;

  DISALLOW_COPY_AND_ASSIGN(AidlArgument);
};

class AidlMethod {
 public:
  AidlMethod(bool oneway, AidlType* type, std::string name,
             std::vector<std::unique_ptr<AidlArgument>>* args,
             unsigned line, std::string comments);
  AidlMethod(bool oneway, AidlType* type, std::string name,
             std::vector<std::unique_ptr<AidlArgument>>* args,
             unsigned line, std::string comments, int id);
  virtual ~AidlMethod() = default;

  const std::string& GetComments() const { return comments_; }
  const AidlType& GetType() const { return *type_; }
  bool IsOneway() const { return oneway_; }
  const std::string& GetName() const { return name_; }
  unsigned GetLine() const { return line_; }
  bool HasId() const { return has_id_; }
  int GetId() { return id_; }
  void SetId(unsigned id) { id_ = id; }

  const std::vector<std::unique_ptr<AidlArgument>>& GetArguments() const {
      return arguments_;
  }

 private:
  bool oneway_;
  std::string comments_;
  std::unique_ptr<AidlType> type_;
  std::string name_;
  unsigned line_;
  std::vector<std::unique_ptr<AidlArgument>> arguments_;
  bool has_id_;
  int id_;

  DISALLOW_COPY_AND_ASSIGN(AidlMethod);
};

enum {
  USER_DATA_TYPE = 12,
  INTERFACE_TYPE_BINDER
};

class AidlDocumentItem : public AidlNode {
 public:
  AidlDocumentItem() = default;
  virtual ~AidlDocumentItem() = default;

  AidlDocumentItem* next;
  unsigned item_type;

 private:
  DISALLOW_COPY_AND_ASSIGN(AidlDocumentItem);
};

class AidlParcelable : public AidlDocumentItem {
 public:
  AidlParcelable() = default;
  virtual ~AidlParcelable() = default;

  buffer_type keyword_token; // only the first one
  char* package;
  buffer_type name;
  buffer_type semicolon_token;
  bool parcelable;

 private:
  DISALLOW_COPY_AND_ASSIGN(AidlParcelable);
};

class AidlInterface : public AidlDocumentItem {
 public:
  explicit AidlInterface() = default;
  virtual ~AidlInterface() = default;

  buffer_type interface_token;
  bool oneway;
  buffer_type oneway_token;
  char* package;
  buffer_type name;
  buffer_type open_brace_token;
  std::vector<std::unique_ptr<AidlMethod>>* methods;
  buffer_type close_brace_token;
  buffer_type* comments_token; // points into this structure, DO NOT DELETE

 private:
  DISALLOW_COPY_AND_ASSIGN(AidlInterface);
};


void init_buffer_type(buffer_type* buf, int lineno);

class AidlImport : public AidlNode {
 public:
  AidlImport(const std::string& from, const std::string& needed_class,
             unsigned line);
  virtual ~AidlImport() = default;

  const std::string& GetFileFrom() const { return from_; }
  const std::string& GetFilename() const { return filename_; }
  const std::string& GetNeededClass() const { return needed_class_; }
  unsigned GetLine() const { return line_; }
  const AidlDocumentItem* GetDocument() { return document_.get(); };
  void SetDocument(AidlDocumentItem* doc) {
    document_ = std::unique_ptr<AidlDocumentItem>(doc);
  }

  void SetFilename(const std::string& filename) { filename_ = filename; }

 private:
  std::unique_ptr<AidlDocumentItem> document_;
  std::string from_;
  std::string filename_;
  std::string needed_class_;
  unsigned line_;

  DISALLOW_COPY_AND_ASSIGN(AidlImport);
};

class Parser {
 public:
  explicit Parser(const android::aidl::IoDelegate& io_delegate);
  ~Parser();

  // Parse contents of file |filename|.
  bool ParseFile(const std::string& filename);

  void ReportError(const std::string& err);

  bool FoundNoErrors() const { return error_ == 0; }
  const std::string& FileName() const { return filename_; }
  const std::string& Package() const { return package_; }
  void *Scanner() const { return scanner_; }

  void SetDocument(AidlDocumentItem *items) { document_ = items; };

  void AddImport(std::vector<std::string>* terms, unsigned line);
  void SetPackage(std::vector<std::string>* terms);

  AidlDocumentItem *GetDocument() const { return document_; }
  const std::vector<std::unique_ptr<AidlImport>>& GetImports() { return imports_; }

  void ReleaseImports(std::vector<std::unique_ptr<AidlImport>>* ret) {
      *ret = std::move(imports_);
      imports_.clear();
  }

 private:
  const android::aidl::IoDelegate& io_delegate_;
  int error_ = 0;
  std::string filename_;
  std::string package_;
  void *scanner_ = nullptr;
  AidlDocumentItem* document_ = nullptr;
  std::vector<std::unique_ptr<AidlImport>> imports_;
  std::unique_ptr<std::string> raw_buffer_;
  YY_BUFFER_STATE buffer_;

  DISALLOW_COPY_AND_ASSIGN(Parser);
};

#endif // AIDL_AIDL_LANGUAGE_H_
