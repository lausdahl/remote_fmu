
SET(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "Build rapidjson examples" FORCE)
SET(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "Build rapidjson perftests and unittests" FORCE)
FetchContent_Declare(rapidjson
        GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
        GIT_TAG 7c73dd7de7c4f14379b781418c6e947ad464c818 # this hash seems to make it work on windows (nix works with v1.1.0)
        #   GIT_SHALLOW ON
        SOURCE_DIR "${CMAKE_BINARY_DIR}/rapidjson"
        BUILD_COMMAND ""
        CONFIGURE_COMMAND ""
        INSTALL_COMMAND ""
        TEST_COMMAND ""
        EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(rapidjson )
# Add client executable
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFMI_COSIMULATION")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFMI_COSIMULATION")
add_definitions( -DCUSTOM_Instantiate
        -DCUSTOM_GetString
        -DCUSTOM_GetFMUstate
        -DCUSTOM_FreeFMUstate
        -DCUSTOM_SerializedFMUstateSize
        -DCUSTOM_SerializeFMUstate
        -DCUSTOM_DeSerializeFMUstate
        -DCUSTOM_NewDiscreteStates
        -DCUSTOM_CompletedIntegratorStep
        -DCUSTOM_SetRealInputDerivatives
        -DCUSTOM_GetRealOutputDerivatives
        -DCUSTOM_GetStatus
        -DCUSTOM_GetRealStatus
        -DCUSTOM_GetIntegerStatus
        -DCUSTOM_GetBooleanStatus
        -DCUSTOM_GetStringStatus
        -DCUSTOM_FreeInstance
        -DFMI_COSIMULATION)
#add_executable(client  client.cc custom_client.cpp client_fmi.cxx execution_statistics.cpp establish_remote_connection.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../grpc_fmi_enums.cxx ${PROTO_SRCS} ${GRPC_SRCS})
#target_link_libraries(client fmi2_proto fmi zmq_client_transport)
#target_precompile_headers(client PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR}/fmi_metrics.h)
#target_include_directories(client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../ ${rapidjson_SOURCE_DIR}/include)
#target_compile_definitions(client PRIVATE -DCOMMUNICATION_STUB_TYPE=Fmi2Service::Stub)

add_executable(client_zmq  client.cc custom_client.cpp client_fmi.cxx execution_statistics.cpp establish_remote_connection_zmq.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../grpc_fmi_enums.cxx ${PROTO_SRCS} ${GRPC_SRCS})
target_link_libraries(client_zmq fmi2_proto fmi zmq_client_transport)
target_precompile_headers(client_zmq PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR}/fmi_metrics.h)
target_include_directories(client_zmq PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../ ${rapidjson_SOURCE_DIR}/include)
target_compile_definitions(client_zmq PRIVATE -DCOMMUNICATION_STUB_TYPE=Fmi2ZmqClientTransport)


add_library(remote_fmu SHARED custom_client.cpp client_fmi.cxx execution_statistics.cpp establish_remote_connection_zmq.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../grpc_fmi_enums.cxx ${PROTO_SRCS} ${GRPC_SRCS})
#add_library(remote_fmu SHARED custom_client.cpp client_fmi.cxx execution_statistics.cpp establish_remote_connection.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../grpc_fmi_enums.cxx ${PROTO_SRCS} ${GRPC_SRCS})
#target_link_libraries(remote_fmu fmi2_proto fmi)
target_link_libraries(remote_fmu fmi2_proto fmi zmq_client_transport)
target_precompile_headers(remote_fmu PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR}/fmi_metrics.h)
target_include_directories(remote_fmu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../ ${rapidjson_SOURCE_DIR}/include)
#target_compile_definitions(remote_fmu PRIVATE -DCOMMUNICATION_STUB_TYPE=Fmi2Service::Stub)
target_compile_definitions(remote_fmu PRIVATE -DCOMMUNICATION_STUB_TYPE=Fmi2ZmqClientTransport)



foreach(LETTER ${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES})

    if(EXISTS "${LETTER}/libstdc++.a")
        #   message (STATUS "${LETTER}")
        set(STDCPP_LIBRARY "${LETTER}/libstdc++.a")
    else()
        #  message (STATUS "${LETTER} ---- not these")
    endif()
endforeach()

message(STATUS "Static linking with libstdc++.a in ${STDCPP_LIBRARY}")
target_link_libraries(remote_fmu ${STDCPP_LIBRARY})


#IF(NOT(DEFINED XOSX))

if (UNIX)
    if(APPLE)


        SET(PTHREAD_LIBRARY libpthread.dylib)
    ELSE()
        find_library(PTHREAD_LIBRARY NAMES libpthread.a libpthread.dylib)


    ENDIF()
ENDIF()

#target_link_libraries(${PROJECT_NAME} ${PTHREAD_LIBRARY})

# I need this: -static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic

if (WIN32)
    #windows
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
endif()


if (UNIX)
    if(APPLE)

    else()


        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
        target_link_libraries(remote_fmu m)

    endif()
endif()

set_target_properties(remote_fmu
        PROPERTIES POSITION_INDEPENDENT_CODE ON, PREFIX "")


include(FmiBinaryFolder.cmake)


get_fmi_binary_folder(output_folder)

set(FOLDER_TO_ZIP "${CMAKE_CURRENT_SOURCE_DIR}/remote_fmu/")
set(ZIP_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/remote_fmu.zip")


add_custom_command(TARGET remote_fmu POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${FOLDER_TO_ZIP}"
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/create_remote_fmu.py ${FOLDER_TO_ZIP}
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:remote_fmu> "${FOLDER_TO_ZIP}/${output_folder}/$<TARGET_FILE_NAME:remote_fmu>"
        COMMENT "Creating binary library for fmu"
        COMMAND ${CMAKE_COMMAND} -E chdir ${FOLDER_TO_ZIP} ${CMAKE_COMMAND} -E tar "cf" ${ZIP_OUTPUT} --format=zip -- binaries "create_remote_fmu.py"
        COMMENT "Zipping folder using zip command"
        DEPENDS remote_fmu
        BYPRODUCTS   ${ZIP_OUTPUT}
        VERBATIM
)


