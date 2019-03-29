cmake_minimum_required(VERSION 3.12)
project(lemond)

set(CMAKE_CXX_STANDARD 17)

find_package(Boost COMPONENTS system filesystem iostreams REQUIRED)
add_executable(lemond main.cpp main.h json.h spawn.h ws.h timer.h gzip.h systemd.cpp systemd.h user.cpp user.h ws.cpp)
target_link_libraries(lemond stdc++fs pthread systemd)
target_link_libraries(lemond
#        ${Boost_FILESYSTEM_LIBRARY}
#        ${Boost_SYSTEM_LIBRARY}
        )
