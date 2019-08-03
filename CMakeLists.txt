cmake_minimum_required(VERSION 3.10)
project(server)

set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Threads REQUIRED)
find_package(Qt5Core REQUIRED COMPONENTS QString QVector)
find_package(Boost REQUIRED COMPONENTS system filesystem)
set(Boost_USE_MULTITHREADED ON)
include_directories(${Boost_INCLUDE_DIRS})

add_executable(async src/async.cpp)
target_link_libraries(async Threads::Threads ${Boost_LIBRARIES} Qt5::Core)
