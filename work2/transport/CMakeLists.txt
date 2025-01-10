

# Find ZeroMQ
find_package(PkgConfig REQUIRED)
#pkg_check_modules(ZeroMQ REQUIRED libzmq)

# Fetch cppzmq from GitHub
include(FetchContent)
#FetchContent_Declare(
#        cppzmq
#        GIT_REPOSITORY https://github.com/zeromq/cppzmq.git
#        GIT_TAG master
##        FIND_PACKAGE_ARGS
#)
#
#FetchContent_MakeAvailable(cppzmq)

# Fetch ZeroMQ (for cppzmq)
FetchContent_Declare(
        zeromq
        GIT_REPOSITORY https://github.com/zeromq/libzmq.git
        GIT_TAG v4.3.5  # You can choose the version you need

)
set(ZeroMQ_BUILD_STATIC ON CACHE INTERNAL "")
set(ZeroMQ_BUILD_SHARED OFF CACHE INTERNAL "")
set(ZeroMQ_BUILD_TESTS OFF CACHE INTERNAL "")
set(CPPZMQ_BUILD_TESTS OFF CACHE INTERNAL "")
set(ZMQ_BUILD_TESTS OFF CACHE INTERNAL "")



# Fetch cppzmq
FetchContent_Declare(
        cppzmq
        GIT_REPOSITORY https://github.com/zeromq/cppzmq.git
        GIT_TAG v4.10.0  # Or specify a version

)

# Make sure dependencies are downloaded
FetchContent_MakeAvailable(zeromq cppzmq)


# Add include directories for ZeroMQ
#include_directories(${ZMQ_INCLUDE_DIRS})
#include_directories(${cppzmq_SOURCE_DIR})
# Create server executable
#add_executable(server src/server.cpp)
#target_link_libraries(server ${ZMQ_LIBRARIES})
#message(${ZMQ_LIBRARIES})
# Create client executable
add_library(zmq_client_transport INTERFACE)
target_link_libraries(zmq_client_transport INTERFACE fmi2_proto cppzmq)
target_include_directories(zmq_client_transport INTERFACE  ${ZMQ_INCLUDE_DIRS} ${cppzmq_SOURCE_DIR})
target_include_directories(zmq_client_transport INTERFACE  ${CMAKE_CURRENT_SOURCE_DIR})

add_library(zmq_server_transport STATIC Fmi2ZmqServerTransport.cpp)
target_link_libraries(zmq_server_transport fmi2_proto  cppzmq libzmq-static fmi)
target_include_directories(zmq_server_transport PUBLIC ${ZMQ_INCLUDE_DIRS} ${cppzmq_SOURCE_DIR})
target_include_directories(zmq_server_transport PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../)

set_target_properties(zmq_server_transport PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(zmq_client_transport PROPERTIES POSITION_INDEPENDENT_CODE ON)
# Optional: Compiler settings
#set_target_properties(server client PROPERTIES
#        CXX_STANDARD 17
#        CXX_STANDARD_REQUIRED ON
#)
