cmake_minimum_required(VERSION 3.15)
project(fatChecker)

set(CMAKE_CXX_STANDARD 17)
include_directories(headers sources)
add_executable(fatChecker main.cpp sources/reader.cpp headers/reader.h sources/parsers.cpp headers/parsers.h)