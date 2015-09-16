#include "ast_cpp.h"

#include "code_writer.h"

namespace android {
namespace aidl {

CppNamespace::CppNamespace(const std::string& name,
                           std::vector<CppDeclaration *> declarations)
    : declarations_(declarations),
      name_(name) {}

CppNamespace::~CppNamespace() {
  for (auto dec : declarations_)
    delete dec;
}

void CppNamespace::Write(CodeWriter* to) const {
  to->Write("namespace %s {\n\n", name_.c_str());

  for (const auto& dec : declarations_)
    dec->Write(to);

  to->Write("\n}  // namespace %s\n", name_.c_str());
}

CppDocument::CppDocument(const std::vector<std::string>& include_list,
                         CppNamespace *a_namespace)
    : include_list_(include_list),
      namespace_(a_namespace) {}

void CppDocument::Write(CodeWriter* to) const {
  for (const auto& include : include_list_) {
    to->Write("#include <%s>\n", include.c_str());
  }
  to->Write("\n");

  namespace_->Write(to);
}

CppHeader::CppHeader(const std::string& include_guard,
                     const std::vector<std::string>& include_list,
                     CppNamespace *a_namespace)
    : CppDocument(include_list, a_namespace),
      include_guard_(include_guard) {}

void CppHeader::Write(CodeWriter* to) const {
  to->Write("#ifndef %s\n", include_guard_.c_str());
  to->Write("#define %s\n\n", include_guard_.c_str());

  CppDocument::Write(to);
  to->Write("\n");

  to->Write("#endif  // %s", include_guard_.c_str());
}

CppSource::CppSource(const std::vector<std::string>& include_list,
                     CppNamespace *a_namespace)
    : CppDocument(include_list, a_namespace) {}

}  // namespace aidl
}  // namespace android
