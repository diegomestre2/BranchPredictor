cmake_minimum_required(VERSION 3.6)
project(BranchPredictor)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES perceptronBranchPredictor.c)
add_executable(BranchPredictor ${SOURCE_FILES})