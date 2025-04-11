cmake_minimum_required(VERSION 3.20)

project(json LANGUAGES CXX)

add_library(json src/json.cpp)
add_library(alx-home::json ALIAS json)

set_target_properties(json PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(json PROPERTIES CXX_STANDARD 26)
set_target_properties(json PROPERTIES CMAKE_CXX_STANDARD_REQUIRED ON)
set_target_properties(json PROPERTIES CMAKE_CXX_EXTENSIONS ON)

target_include_directories(json INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(json PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/json")


set(COMPILE_OPTIONS
    -std=c++2c
    "$<$<CONFIG:DEBUG>:-DDEBUG>"
    -Wall -Wextra -Wpedantic -Wcast-align -Waddress-of-packed-member
    -ftemplate-backtrace-limit=0
    "$<$<CONFIG:Release>:-O3>"
    "$<$<CONFIG:Debug>:-O0>"
)

# set(SANITIZE "address")

if(DEFINED SANITIZE)
    list(APPEND COMPILE_OPTIONS
        -fsanitize=${SANITIZE}
    )
endif(DEFINED SANITIZE)

# if(DEFINED ADDRESS_SANITIZER)
#     list(APPEND COMPILE_OPTIONS 
#         "-DADDRESS_SANITIZER"
#         -fsanitize-recover=address
#     )
# endif(DEFINED ADDRESS_SANITIZER)

if(MSVC)
    list(TRANSFORM COMPILE_OPTIONS PREPEND "-clang:")
    target_compile_options(json PUBLIC /W4 ${COMPILE_OPTIONS})
else()
    target_compile_options(json PUBLIC 
        -export-dynamic
        -ggdb3 -pg -g
        ${COMPILE_OPTIONS}
        -D_GNU_SOURCE
        -Wno-psabi
    )
endif()


# set(MAJOR_VERSION 1)
# set(MINOR_VERSION 0)
# set(REVISION 0)

# set(VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.${REVISION})

# set_target_properties(vfrnav PROPERTIES VERSION ${MAJOR_VERSION})
# set_target_properties(vfrnav PROPERTIES SOVERSION ${MAJOR_VERSION}.${MINOR_VERSION})