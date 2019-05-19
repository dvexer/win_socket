cmake_minimum_required(VERSION 3.14)
project(server)

set(CMAKE_CXX_STANDARD 17)

add_executable(server main.cpp)
find_library(WIN_THREAD libwinpthread-1.dll)
target_link_libraries(server ws2_32 -static-libgcc -static-libstdc++ -static ${WIN_THREAD})
