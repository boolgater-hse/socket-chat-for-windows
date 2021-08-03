# system
cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 17)

# project
project(server CXX)

# sources
set(source_files src/server.cpp)

# build
SET(CMAKE_CXX_FLAGS -pthread)
add_executable(${PROJECT_NAME} src/server.cpp src/funcs.cpp src/funcs.h)
target_link_libraries(${PROJECT_NAME} ws2_32)
