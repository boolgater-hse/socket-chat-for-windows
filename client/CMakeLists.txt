cmake_minimum_required(VERSION 3.20)
project(client)

set(CMAKE_CXX_STANDARD 17)

# sources
set(source_files src/client.cpp)

# build
SET(CMAKE_CXX_FLAGS -pthread)
add_executable(client src/client.cpp)
target_link_libraries(client ws2_32)
target_link_libraries(${PROJECT_NAME} pthread)
