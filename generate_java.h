#ifndef AIDL_GENERATE_JAVA_H_
#define AIDL_GENERATE_JAVA_H_

#include <string>

#include "aidl_language.h"
#include "ast_java.h"

namespace android {
namespace aidl {

using std::string;
using std::vector;

class JavaTypeNamespace;

int generate_java(const string& filename, const string& originalSrc,
                  interface_type* iface, JavaTypeNamespace* types);

android::aidl::Class* generate_binder_interface_class(
    const interface_type* iface, JavaTypeNamespace* types);

string gather_comments(extra_text_type* extra);
string append(const char* a, const char* b);

class VariableFactory
{
public:
    using Variable = android::aidl::Variable;
    using Type = android::aidl::Type;

    VariableFactory(const string& base); // base must be short
    Variable* Get(const Type* type);
    Variable* Get(int index);
private:
    vector<Variable*> m_vars;
    string m_base;
    int m_index;
};

}  // namespace android
}  // namespace aidl

#endif // AIDL_GENERATE_JAVA_H_
