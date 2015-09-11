#include "aidl.h"
#include "options.h"

#include <stdio.h>

int
main(int argc, const char **argv)
{
    Options options;
    int result = parse_options(argc, argv, &options);
    if (result) {
        return result;
    }

    switch (options.task) {
        case COMPILE_AIDL:
            return android::aidl::compile_aidl(options);
        case PREPROCESS_AIDL:
            return android::aidl::preprocess_aidl(options);
    }
    fprintf(stderr, "aidl: internal error\n");
    return 1;
}
