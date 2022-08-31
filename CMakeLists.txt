cmake_minimum_required(VERSION 3.18.0)

project(peer-socket C CXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -ggdb")

set(srcs "peer-socket.cc") 
set(test-srcs "test-peer-socket.cc") 

add_library(${CMAKE_PROJECT_NAME} STATIC ${srcs})

add_executable(test-${CMAKE_PROJECT_NAME} ${test-srcs})
target_link_libraries(test-${CMAKE_PROJECT_NAME} ${CMAKE_PROJECT_NAME})
