# Minimum required CMake version


add_definitions(
        -DCUSTOM_Instantiate
        -DCUSTOM_SetString
        -DCUSTOM_FreeInstance
        -DCUSTOM_SetDebugLogging
        -DCUSTOM_GetFMUstate
        -DCUSTOM_SetFMUstate
        -DCUSTOM_FreeFMUstate
        -DCUSTOM_SerializedFMUstateSize
        -DCUSTOM_SerializeFMUstate
        -DCUSTOM_DeSerializeFMUstate
        -DCUSTOM_GetDirectionalDerivative
        -DCUSTOM_NewDiscreteStates
        -DCUSTOM_CompletedIntegratorStep
        -DCUSTOM_SetContinuousStates
        -DCUSTOM_GetDerivatives
        -DCUSTOM_GetEventIndicators
        -DCUSTOM_GetContinuousStates
        -DCUSTOM_GetNominalsOfContinuousStates
        -DCUSTOM_SetRealInputDerivatives
        -DCUSTOM_GetRealOutputDerivatives
        -DCUSTOM_GetStatus
        -DCUSTOM_GetRealStatus
        -DCUSTOM_GetIntegerStatus
        -DCUSTOM_GetBooleanStatus
        -DCUSTOM_GetStringStatus
        -DCUSTOM_GetString
       # -DFMI_REMOTE_RECORD_EXEC_START\(name\)=record_exec_start\(FmiFunctionNames::name\)\;
)

# Add server executable
#add_executable(server
#
#        ${CMAKE_CURRENT_SOURCE_DIR}/server_main.cpp
#        ${CMAKE_CURRENT_SOURCE_DIR}/Fmi2ServiceImpl.cpp
#        ${CMAKE_CURRENT_SOURCE_DIR}/../grpc_fmi_enums.cxx
#        ${CMAKE_CURRENT_SOURCE_DIR}/lib/SimFmi2.cpp
#        ${CMAKE_CURRENT_SOURCE_DIR}/Fmi2ServiceImplCustom.cpp
#)
#target_precompile_headers(server PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR}/fmi_metrics.h)
#target_link_libraries(server fmi2_proto fmi )
#target_include_directories(server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../ ${CMAKE_CURRENT_SOURCE_DIR}/lib)


add_executable(server_zmq

        ${CMAKE_CURRENT_SOURCE_DIR}/server_zmq_main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Fmi2ServiceImpl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../grpc_fmi_enums.cxx
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/SimFmi2.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Fmi2ServiceImplCustom.cpp
)
target_precompile_headers(server_zmq PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR}/fmi_metrics.h)
target_link_libraries(server_zmq  fmi zmq_server_transport)
target_include_directories(server_zmq PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../ ${CMAKE_CURRENT_SOURCE_DIR}/lib)

#add_custom_command(
#        TARGET server
#        POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E compare_files
#        ${CMAKE_SOURCE_DIR}/fmi2functiontest.fmu
#        $<TARGET_FILE_DIR:server>/fmi2functiontest.fmu
#        COMMAND ${CMAKE_COMMAND} -E copy_if_different
#        ${CMAKE_SOURCE_DIR}/fmi2functiontest.fmu
#        $<TARGET_FILE_DIR:server>/fmi2functiontest.fmu
#)