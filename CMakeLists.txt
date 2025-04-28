cmake_minimum_required(VERSION 3.20)

project(alx-home_json LANGUAGES CXX)

win32_library(TARGET_NAME alx-home_json
    FILES 
        src/json.cpp
)

target_link_libraries(alx-home_json PUBLIC alx-home::cpp_utils)

add_library(alx-home::json ALIAS alx-home_json)

target_include_directories(alx-home_json INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(alx-home_json PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/json")
