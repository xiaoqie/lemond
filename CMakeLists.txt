cmake_minimum_required(VERSION 3.12)
project(lemond)

set(CMAKE_CXX_STANDARD 17)

find_package(Boost COMPONENTS system filesystem REQUIRED)
add_executable(lemond main.cpp)
target_link_libraries(lemond stdc++fs pthread)
target_link_libraries(lemond
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        )