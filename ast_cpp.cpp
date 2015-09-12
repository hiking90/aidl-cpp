#include "ast_cpp.h"

#include "code_writer.h"

namespace android {
namespace aidl {

namespace {

void WriteDocumentBody(CodeWriter* to,
                       const std::vector<std::string>& include_list,
                       const std::vector<std::string>& namespaces) {
  for (const auto& include : include_list) {
    to->Write("#include <%s>\n", include.c_str());
  }
  to->Write("\n");

  for (const auto& space : namespaces) {
    to->Write("namespace %s {\n", space.c_str());
  }
  to->Write("\n");

  // TODO(wiley) When we have classes to generate, put them here.
  for (auto it = namespaces.rbegin(); it != namespaces.rend(); ++it) {
    to->Write("}  // namespace %s\n", it->c_str());
  }
}

}  // namespace

CppHeader::CppHeader(const std::string& include_guard,
                     const std::vector<std::string>& include_list,
                     const std::vector<std::string>& namespaces)
    : include_guard_(include_guard),
      include_list_(include_list),
      namespaces_(namespaces) {}

void CppHeader::Write(CodeWriter* to) const {
  to->Write("#ifndef %s\n", include_guard_.c_str());
  to->Write("#define %s\n\n", include_guard_.c_str());

  WriteDocumentBody(to, include_list_, namespaces_);
  to->Write("\n");

  to->Write("#endif  // %s", include_guard_.c_str());
}

CppDocument::CppDocument(const std::vector<std::string>& include_list,
            const std::vector<std::string>& namespaces)
    : include_list_(include_list),
      namespaces_(namespaces) {}

void CppDocument::Write(CodeWriter* to) const {
  WriteDocumentBody(to, include_list_, namespaces_);
}

}  // namespace aidl
}  // namespace android
