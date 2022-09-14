cmake_minimum_required(VERSION 3.1)

project(SimpleMuduo)

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -fPIC")

include_directories(${PROJECT_SOURCE_DIR}/include)

aux_source_directory(. SOURCE_LIST)

add_library(SimpleMuduo SHARED ${SOURCE_LIST})



