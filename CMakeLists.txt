cmake_minimum_required(VERSION 3.10)
project(CoreKit VERSION "0.1.0" LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

option(COREKIT_BUILD_EXAMPLES "Builds the example programs" ON)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE Library)

find_package(ctre REQUIRED)
find_package(OpenSSL REQUIRED)
target_link_libraries(${PROJECT_NAME} INTERFACE ctre::ctre openssl::openssl)

if (COREKIT_BUILD_EXAMPLES)
    add_subdirectory(Sample)
endif()

install(
    DIRECTORY Library/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
)

install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    INCLUDES DESTINATION include
)

install(
    EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    DESTINATION lib/cmake/${PROJECT_NAME}
)