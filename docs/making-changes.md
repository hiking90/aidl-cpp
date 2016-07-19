# Making changes

## Coding style

This project was originally written in C, in the Android platform style.
It has been substantially re-written in C++, in the Google C++ style.

This style
[is summarized here](https://google.github.io/styleguide/cppguide.html).

When in doubt, clang-format -style=google is a good reference.

## Testing

This codebase has both integration and unittests, all of which are expected to
consistently pass against a device/emulator:

```
$ mmma system/tools/aidl && \
    out/host/linux-x86/nativetest64/aidl_unittests/aidl_unittests && \
    adb remount && adb sync && \
    adb install -r `find out/ -name aidl_test_services.apk` && \
    (pushd system/tools/aidl/ && tests/integration-test.py) && \
    echo "All tests pass"

```
