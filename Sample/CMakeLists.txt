cmake_minimum_required(VERSION 3.10)

add_executable(HTTPServer HTTPServer.cpp)
target_link_libraries(HTTPServer PRIVATE CoreKit)

add_executable(DHTRunner DHTRunner.cpp)
target_link_libraries(DHTRunner PRIVATE CoreKit)