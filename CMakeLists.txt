cmake_minimum_required(VERSION 3.10)
project(server)

set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Threads REQUIRED)
find_package(Boost 1.69 REQUIRED COMPONENTS system filesystem program_options)
set(Boost_USE_MULTITHREADED ON)
include_directories(${Boost_INCLUDE_DIRS})

set(ADDITIONAL_LIBRARIES "")
if(MINGW)
    set(ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES} -lws2_32 -lwsock32)
elseif(MSVC)
    set(ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES} ws2_32.lib wsock32.lib)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_WIN32_WINNT=0x0601 /D_SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING")
endif()

include_directories(./inc)
add_library(http SHARED inc/http.h src/http.cpp)

add_executable(async src/async.cpp)
target_link_libraries(async Threads::Threads ${Boost_LIBRARIES} ${ADDITIONAL_LIBRARIES})
