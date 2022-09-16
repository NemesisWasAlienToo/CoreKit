cmake_minimum_required(VERSION 3.10)

project(CoreKit VERSION "0.1.0" LANGUAGES CXX)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(IS_TOPLEVEL_PROJECT TRUE)
else()
    set(IS_TOPLEVEL_PROJECT FALSE)
endif()

set(CMAKE_CXX_STANDARD 20)

add_library(${PROJECT_NAME} INTERFACE)
add_subdirectory(Library)
target_include_directories(${PROJECT_NAME} INTERFACE Library)

# Add openssl

find_package(OpenSSL REQUIRED)

if(NOT OPENSSL_FOUND)
    message(FATAL_ERROR "OpenSSL is needed")
endif()

target_link_libraries(${PROJECT_NAME} INTERFACE ${OPENSSL_LIBRARIES})

# Add pthread

target_link_libraries(${PROJECT_NAME} INTERFACE pthread)