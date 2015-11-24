#include "generate_java.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "code_writer.h"
#include "type_java.h"

using ::android::aidl::java::Variable;

namespace android {
namespace aidl {

// =================================================
VariableFactory::VariableFactory(const string& base)
    :m_base(base),
     m_index(0)
{
}

Variable*
VariableFactory::Get(const Type* type)
{
    char name[100];
    sprintf(name, "%s%d", m_base.c_str(), m_index);
    m_index++;
    Variable* v = new Variable(type, name);
    m_vars.push_back(v);
    return v;
}

Variable*
VariableFactory::Get(int index)
{
    return m_vars[index];
}

// =================================================
string
append(const char* a, const char* b)
{
    string s = a;
    s += b;
    return s;
}

namespace java {

int
generate_java(const string& filename, const string& originalSrc,
                AidlInterface* iface, JavaTypeNamespace* types,
                const IoDelegate& io_delegate)
{
    Class* cl;

    if (iface->item_type == INTERFACE_TYPE_BINDER) {
        cl = generate_binder_interface_class(iface, types);
    }

    Document* document = new Document;
        document->comment = "";
        if (!iface->GetPackage().empty())
            document->package = iface->GetPackage();
        document->originalSrc = originalSrc;
        document->classes.push_back(cl);

    CodeWriterPtr code_writer = io_delegate.GetCodeWriter(filename);
    document->Write(code_writer.get());

    return 0;
}

}  // namespace java
}  // namespace android
}  // namespace aidl
