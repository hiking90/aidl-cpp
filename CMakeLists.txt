find_package(BISON)
find_package(FLEX)

set(CMAKE_CXX_FLAGS "-std=gnu++11 -fPIC -DPIC -Wno-unused-parameter -Wno-unused-function")

bison_target(LangParser aidl_language_y.yy ${CMAKE_CURRENT_BINARY_DIR}/aidl_language_y.cpp)
flex_target(LangScanner aidl_language_l.ll  ${CMAKE_CURRENT_BINARY_DIR}/aidl_language_l.cpp)
add_flex_bison_dependency(LangScanner LangParser)

include_directories(../include ../android base/include . ${CMAKE_CURRENT_BINARY_DIR})

set(SRCS
    aidl.cpp
    aidl_language.cpp
    ${BISON_LangParser_OUTPUTS}
    ${FLEX_LangScanner_OUTPUTS}
    ast_cpp.cpp
    ast_java.cpp
    code_writer.cpp
    generate_cpp.cpp
    generate_java.cpp
    generate_java_binder.cpp
    import_resolver.cpp
    line_reader.cpp
    io_delegate.cpp
    options.cpp
    type_cpp.cpp
    type_java.cpp
    type_namespace.cpp
    main_cpp.cpp
    base/logging.cpp
    base/stringprintf.cpp
    base/strings.cpp
)

add_executable(aidl-cpp ${SRCS})
target_link_libraries(aidl-cpp binder)
