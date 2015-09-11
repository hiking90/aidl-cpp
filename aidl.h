#ifndef AIDL_AIDL_H_
#define AIDL_AIDL_H_

#include "options.h"

namespace android {
namespace aidl {

// strips off the leading whitespace, the "import" text
// also returns whether it's a local or system import
// we rely on the input matching the import regex from below
char* parse_import_statement(const char* text);

int convert_direction(const char* direction);

int compile_aidl(Options& options);
int preprocess_aidl(const Options& options);

}  // namespace android
}  // namespace aidl

#endif  // AIDL_AIDL_H_
