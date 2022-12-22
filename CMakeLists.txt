cmake_minimum_required(VERSION 2.8.12)

project(final)

add_compile_options(-std=c++11)
add_executable(${PROJECT_NAME} main.cpp)

find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
  target_compile_options(${PROJECT_NAME} PUBLIC "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(${PROJECT_NAME} "-pthread")
endif()