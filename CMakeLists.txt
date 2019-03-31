cmake_minimum_required(VERSION 3.12)
project(lemond)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

find_package(Boost COMPONENTS system filesystem iostreams REQUIRED)
add_executable(lemond main.cpp main.h json.h spawn.h ws.h timer.h gzip.h systemd.cpp systemd.h user.cpp user.h ws.cpp)
target_link_libraries(lemond stdc++fs pthread systemd)
target_link_libraries(lemond)
