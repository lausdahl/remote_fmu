# Minimum required CMake version


cmake_minimum_required(VERSION 3.16)
project(my_project C CXX)

set(CMAKE_CXX_STANDARD 17)

# Add the directory containing CMake package configurations
set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../protobuf-install/lib/cmake" ${CMAKE_PREFIX_PATH})



add_subdirectory(proto)
add_subdirectory(fmi)
add_subdirectory(transport)

add_subdirectory(server)
add_subdirectory(client)
