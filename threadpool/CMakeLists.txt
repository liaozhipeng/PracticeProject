project(Ctt)
cmake_minimum_required(VERSION 3.5)
add_compile_options(-std=c++11)

find_package(Threads)
FILE(GLOB_RECURSE CURRENT_SOURCES src/*.cpp)   
#add_executable(gdb_test test.cpp Worker.cpp Mutex_Cond.cpp MyThread.cpp)
add_executable(gdb_test ${CURRENT_SOURCES})

target_link_libraries(gdb_test ${CMAKE_THREAD_LIBS_INIT})