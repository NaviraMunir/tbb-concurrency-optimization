cmake_minimum_required(VERSION 3.10)
project(tbb_performance_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

find_package(TBB REQUIRED)

add_executable(tbb_performance_demo src/main.cpp)
target_link_libraries(tbb_performance_demo PRIVATE TBB::tbb TBB::tbbmalloc)
