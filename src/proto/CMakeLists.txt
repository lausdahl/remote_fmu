


find_package(protobuf CONFIG REQUIRED)
#find_package(gRPC CONFIG REQUIRED)
find_package(Threads)

#
# Protobuf/Grpc source files
#
set(PROTO_FILES
    fmi2.proto

)

#
# Add Library target with protobuf sources
#
add_library(fmi2_proto STATIC ${PROTO_FILES} "${CMAKE_CURRENT_SOURCE_DIR}/Status.cpp")
target_link_libraries(fmi2_proto
    PUBLIC
        protobuf::libprotobuf
 #       gRPC::grpc
 #       gRPC::grpc++
)
target_include_directories(fmi2_proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(fmi2_proto PROPERTIES POSITION_INDEPENDENT_CODE ON)
#
# Compile protobuf and grpc files in fmi2 target to cpp
#
#get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)
protobuf_generate(TARGET fmi2_proto LANGUAGE cpp)
set_target_properties(fmi2_proto PROPERTIES POSITION_INDEPENDENT_CODE ON)
#protobuf_generate(TARGET fmi2_proto LANGUAGE grpc GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}")