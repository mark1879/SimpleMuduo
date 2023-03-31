cmake_minimum_required(VERSION 3.1)

project(simple_muduo)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

include_directories(${CMAKE_SOURCE_DIR})

add_library(simple_muduo Timestamp.cc Logger.cc InetAddress.cc Channel.cc Poller.cc EPollPoller.cc 
                DefaultPoller.cc CurrentThread.cc EventLoop.cc Thread.cc EventLoopThread.cc EventLoopThreadPool.cc)

set(head_files noncopyable.h)
install(FILES ${head_files} DESTINATION  ${PROJECT_NAME}/include)
install(TARGETS simple_muduo ARCHIVE DESTINATION ${PROJECT_NAME}/lib)

add_subdirectory(examples/)