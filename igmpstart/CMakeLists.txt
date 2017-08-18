cmake_minimum_required(VERSION 2.6)

project(igmpstart)

add_executable(igmpstart main.cpp)

target_link_libraries(igmpstart pthread memcached)

install(TARGETS igmpstart RUNTIME DESTINATION bin)
