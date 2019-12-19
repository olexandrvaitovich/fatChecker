cmake_minimum_required(VERSION 3.15)
project(fatChecker)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -lboost_filesystem -lboost_system")
find_package(Boost COMPONENTS system filesystem REQUIRED)
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

set(CMAKE_CXX_STANDARD 17)
include_directories(headers sources)
add_executable(fatChecker main.cpp sources/reader.cpp headers/reader.h sources/parsers.cpp headers/parsers.h)
target_link_libraries(fatChecker  boost_system boost_filesystem)
