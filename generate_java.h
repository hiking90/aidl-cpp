#ifndef AIDL_GENERATE_JAVA_H_
#define AIDL_GENERATE_JAVA_H_

#include <string>

#include "aidl_language.h"
#include "ast_java.h"
#include "io_delegate.h"

namespace android {
namespace aidl {

using std::string;
using std::vector;

namespace java {

class JavaTypeNamespace;

int generate_java(const string& filename, const string& originalSrc,
                  AidlInterface* iface, java::JavaTypeNamespace* types,
                  const IoDelegate& io_delegate);

android::aidl::java::Class* generate_binder_interface_class(
    const AidlInterface* iface, java::JavaTypeNamespace* types);

}  // namespace java

string append(const char* a, const char* b);

class VariableFactory
{
public:
    using Variable = ::android::aidl::java::Variable;
    using Type = ::android::aidl::java::Type;

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
