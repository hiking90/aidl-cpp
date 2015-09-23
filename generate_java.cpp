#include "generate_java.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "code_writer.h"
#include "type_java.h"

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
gather_comments(extra_text_type* extra)
{
    string s;
    while (extra) {
        if (extra->which == SHORT_COMMENT) {
            s += extra->data;
        }
        else if (extra->which == LONG_COMMENT) {
            s += "/*";
            s += extra->data;
            s += "*/";
        }
        extra = extra->next;
    }
    return s;
}

string
append(const char* a, const char* b)
{
    string s = a;
    s += b;
    return s;
}

// =================================================
int
generate_java(const string& filename, const string& originalSrc,
                interface_type* iface)
{
    Class* cl;

    if (iface->document_item.item_type == INTERFACE_TYPE_BINDER) {
        cl = generate_binder_interface_class(iface);
    }

    Document* document = new Document;
        document->comment = "";
        if (iface->package) document->package = iface->package;
        document->originalSrc = originalSrc;
        document->classes.push_back(cl);

    CodeWriterPtr code_writer = GetFileWriter(filename);
    document->Write(code_writer.get());

    return 0;
}

}  // namespace android
}  // namespace aidl
