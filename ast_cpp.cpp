#include "ast_cpp.h"

#include "code_writer.h"

using std::unique_ptr;

namespace android {
namespace aidl {

CppClassDeclaration::CppClassDeclaration(const std::string& name, const std::string& parent,
                                         std::vector<unique_ptr<CppDeclaration>> public_members,
                                         std::vector<unique_ptr<CppDeclaration>> private_members)
    : name_(name),
      parent_(parent),
      public_members_(std::move(public_members)),
      private_members_(std::move(private_members)) {}

void CppClassDeclaration::Write(CodeWriter* to) const {
  to->Write("class %s ", name_.c_str());

  if (parent_.length() > 0)
      to->Write(": public %s ", parent_.c_str());

  to->Write("{\n");

  if (!public_members_.empty())
      to->Write("public:\n");

  for (const auto& dec : public_members_)
    dec->Write(to);

  if (!private_members_.empty())
      to->Write("private:\n");

  for (const auto& dec : private_members_)
    dec->Write(to);

  to->Write("};  // class %s\n", name_.c_str());
}

CppMethodDeclaration::CppMethodDeclaration(const std::string& return_type,
                                           const std::string& name,
                                           std::vector<std::string> arguments,
                                           bool is_const,
                                           bool is_virtual)
    : return_type_(return_type),
      name_(name),
      arguments_(arguments),
      is_const_(is_const),
      is_virtual_(is_virtual) {}

void CppMethodDeclaration::Write(CodeWriter* to) const {
  if (is_virtual_)
    to->Write("virtual ");

  to->Write("%s %s(", return_type_.c_str(), name_.c_str());

  bool not_first = false;

  for (const auto& arg : arguments_) {
    if (not_first)
      to->Write(", ");
    not_first = true;
    to->Write("%s", arg.c_str());
  }

  to->Write(")");

  if (is_const_)
    to->Write(" const");

  to->Write(";\n");
}

CppNamespace::CppNamespace(const std::string& name,
                           std::vector<unique_ptr<CppDeclaration>> declarations)
    : declarations_(std::move(declarations)),
      name_(name) {}

void CppNamespace::Write(CodeWriter* to) const {
  to->Write("namespace %s {\n\n", name_.c_str());

  for (const auto& dec : declarations_) {
    dec->Write(to);
    to->Write("\n");
  }

  to->Write("}  // namespace %s\n", name_.c_str());
}

CppDocument::CppDocument(const std::vector<std::string>& include_list,
                         unique_ptr<CppNamespace> a_namespace)
    : include_list_(include_list),
      namespace_(std::move(a_namespace)) {}

void CppDocument::Write(CodeWriter* to) const {
  for (const auto& include : include_list_) {
    to->Write("#include <%s>\n", include.c_str());
  }
  to->Write("\n");

  namespace_->Write(to);
}

CppHeader::CppHeader(const std::string& include_guard,
                     const std::vector<std::string>& include_list,
                     unique_ptr<CppNamespace> a_namespace)
    : CppDocument(include_list, std::move(a_namespace)),
      include_guard_(include_guard) {}

void CppHeader::Write(CodeWriter* to) const {
  to->Write("#ifndef %s\n", include_guard_.c_str());
  to->Write("#define %s\n\n", include_guard_.c_str());

  CppDocument::Write(to);
  to->Write("\n");

  to->Write("#endif  // %s", include_guard_.c_str());
}

CppSource::CppSource(const std::vector<std::string>& include_list,
                     unique_ptr<CppNamespace> a_namespace)
    : CppDocument(include_list, std::move(a_namespace)) {}

}  // namespace aidl
}  // namespace android
