cmake_minimum_required(VERSION 3.14)
project(client)

set(CMAKE_CXX_STANDARD 17)

add_executable(client main.cpp)
find_library(WIN_THREAD libwinpthread-1.dll)
target_link_libraries(client ws2_32 -static-libgcc -static-libstdc++ -static ${WIN_THREAD})
