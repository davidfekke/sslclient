
cmake_minimum_required(VERSION 3.10.0)

project(SSLClient)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")

find_package(OpenSSL REQUIRED)

add_executable (SSLClient.a SSLClient.c)
target_link_libraries(SSLClient.a OpenSSL::SSL)
target_link_libraries(SSLClient.a OpenSSL::Crypto)

