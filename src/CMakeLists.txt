cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(asynch_bellman_ford VERSION 1.0 LANGUAGES CXX C)

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)

add_executable(asynch_bellman_ford
  main.cc
  bellman_ford.cc
  bellman_ford.h
  utils/log_util.h
  utils/message.h
  utils/message_channel.h
  utils/connectivity_parser.h)

target_include_directories(asynch_bellman_ford PRIVATE utils)

if(CMAKE_COMPILER_IS_GNUCXX)
  target_compile_options(asynch_bellman_ford PRIVATE -std=c++0x)
  target_link_libraries(asynch_bellman_ford PRIVATE Threads::Threads)
endif()
