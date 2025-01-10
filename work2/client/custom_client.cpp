#include "fmi2.pb.h"
#include "Status.h"
#include "client_fmi.h"
#include <cstring> //memcopy
#include <grpc_fmi_enums.h>

#include "grpc_fmi_mapping.h"
#include "execution_statistics.h"

extern "C" {
#include "fmi2.h"
#include "sim_support.h"
}

extern std::unique_ptr<COMMUNICATION_STUB_TYPE> stub_;
extern const char *remote_url;


// ---------------------------------------------------------------------------
// FMI functions: class methods not depending of a specific model instance
// ---------------------------------------------------------------------------

extern "C" const char * fmi2GetVersion() {
    return fmi2Version;
}

extern "C" const char * fmi2GetTypesPlatform() {
    return fmi2TypesPlatform;
}


inline void stepFinished(fmi2ComponentEnvironment, fmi2Status) {
}

//Define CUSTOM_Instantiate to manually specify an implementation
extern "C" fmi2Component fmi2Instantiate(fmi2String instanceName,
                                         fmi2Type fmuType, fmi2String fmuGUID,
                                         fmi2String fmuResourceLocation,
                                         const fmi2CallbackFunctions *functions,
                                         fmi2Boolean visible,
                                         fmi2Boolean loggingOn) {
    establish_remote_connection(instanceName,fmuResourceLocation,functions);
    auto start_record = FMI_REMOTE_RECORD_EXEC_START(Instantiate);


    auto request = InstantiateRequest();
    auto response = InstantiateResponse();
    // printf("fmi2Instantiate to grpc\n");
    //argument handling
    request.set_instancename(std::string(instanceName));
    request.set_fmutype(Anon_enum1::gfmi2CoSimulation);
    request.set_fmuguid(std::string(fmuGUID));
    request.set_fmuresourcelocation(std::string(fmuResourceLocation));
    //    request.set_functions(reinterpret_cast<::uint32_t>(functions));
    request.set_visible(visible);
    request.set_loggingon(loggingOn);

    functions->logger(nullptr, instanceName, fmi2OK, "fmi2Info", "Connecting to remote host on  %s using guid %s",
                      remote_url, fmuGUID);

    auto status = stub_->Instantiate(request, &response);
    if (status.ok()) {
        // printf("fmi2Instantiate to grpc: ok\n");
        auto comp = reinterpret_cast<fmi2Component>(response.ret());
        fmi2CallbackFunctions cb = {
            .logger = &fmuLogger, .allocateMemory = calloc, .freeMemory = free, .stepFinished = &stepFinished,
            .componentEnvironment = nullptr
        };;
        if (functions != nullptr) {
            memcpy(&cb, functions, sizeof(fmi2CallbackFunctions));
        }

        g_component_env_map.emplace(comp, FmiComponentEnv(std::string(instanceName), cb));
        FMI_REMOTE_RECORD_EXEC_END(Instantiate, start_record);
        return comp;
    } else {
        if (functions != nullptr) {
            functions->logger(nullptr, instanceName, fmi2Fatal, "fmi2Error", "Error %d - %s", status.error_code(),
                              status.error_message().c_str());
        } else {
            std::cerr << status.error_code() << ": " << status.error_message() << std::endl;
        }
    }
    FMI_REMOTE_RECORD_EXEC_END(Instantiate, start_record);
    return nullptr;
}


//Define CUSTOM_GetString to manually specify an implementation
extern "C" fmi2Status fmi2GetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]) {
    auto start_record = FMI_REMOTE_RECORD_EXEC_START(GetString);

    auto request = GetStringRequest();
    auto response = GetStringResponse();

    //argument handling
    request.set_c(static_cast<uint32_t>(reinterpret_cast<std::uintptr_t>(c)));
    for (int i = 0; i < nvr; i++) {
        request.add_vr(vr[i]);
    }
    auto status = stub_->GetString(request, &response);
    if (status.ok()) {
        for (int i = 0; i < nvr && i < response.value_size(); i++) {
            value[i] = strdup(response.value(i).c_str());
        }
        FMI_REMOTE_RECORD_EXEC_END(GetString, start_record);
        return Anon_enum0Tofmi2Status(response.ret());
    } else {
        g_component_env_map.at(c).callback.logger(nullptr, g_component_env_map[c].name.c_str(), fmi2Fatal, "fmi2Error",
                                                  "Error %d - %s", status.error_code(),
                                                  status.error_message().c_str());
        FMI_REMOTE_RECORD_EXEC_END(GetString, start_record);
        return fmi2Fatal;
    }
}


//Define CUSTOM_GetFMUstate to manually specify an implementation
extern "C" fmi2Status fmi2GetFMUstate(fmi2Component c, fmi2FMUstate *FMUstate) {
    return fmi2Fatal;
}


//Define CUSTOM_FreeFMUstate to manually specify an implementation
extern "C" fmi2Status fmi2FreeFMUstate(fmi2Component c, fmi2FMUstate *FMUstate) {
    return fmi2Fatal;
}


//Define CUSTOM_SerializedFMUstateSize to manually specify an implementation
extern "C" fmi2Status fmi2SerializedFMUstateSize(fmi2Component c, fmi2FMUstate FMUstate, size_t *size) {
    return fmi2Fatal;
}


//Define CUSTOM_SerializeFMUstate to manually specify an implementation
extern "C" fmi2Status fmi2SerializeFMUstate(fmi2Component c, fmi2FMUstate FMUstate, fmi2Byte *serializedState,
                                            size_t size) {
    return fmi2Fatal;
}


//Define CUSTOM_DeSerializeFMUstate to manually specify an implementation
extern "C" fmi2Status fmi2DeSerializeFMUstate(fmi2Component c, const fmi2Byte serializedState[], size_t size,
                                              fmi2FMUstate FMUstate[]) {
    return fmi2Fatal;
}


//Define CUSTOM_NewDiscreteStates to manually specify an implementation
extern "C" fmi2Status fmi2NewDiscreteStates(fmi2Component c, fmi2EventInfo *fmi2eventInfo) {
    return fmi2Fatal;
}


//Define CUSTOM_CompletedIntegratorStep to manually specify an implementation
extern "C" fmi2Status fmi2CompletedIntegratorStep(fmi2Component c, fmi2Boolean noSetFMUStatePriorToCurrentPoint,
                                                  fmi2Boolean *enterEventMode, fmi2Boolean *terminateSimulation) {
    return fmi2Fatal;
}


//Define CUSTOM_SetRealInputDerivatives to manually specify an implementation
extern "C" fmi2Status fmi2SetRealInputDerivatives(fmi2Component c, const fmi2ValueReference vr[], size_t nvr,
                                                  const fmi2Integer order[], const fmi2Real value[]) {
    return fmi2Fatal;
}


//Define CUSTOM_GetRealOutputDerivatives to manually specify an implementation
extern "C" fmi2Status fmi2GetRealOutputDerivatives(fmi2Component c, const fmi2ValueReference vr[], size_t nvr,
                                                   const fmi2Integer order[], fmi2Real value[]) {
    auto start_record = FMI_REMOTE_RECORD_EXEC_START(GetRealOutputDerivatives);

    auto request = GetRealOutputDerivativesRequest();
    auto response = GetRealOutputDerivativesResponse();

    //argument handling
    request.set_c(TO_UINT(c));

    GRPC_REQUEST_FROM_FMI_ARRAY(vr, nvr, add_vr);
    GRPC_REQUEST_FROM_FMI_ARRAY(order, nvr, add_order);

    auto status = stub_->GetRealOutputDerivatives(request, &response);
    if (status.ok()) {
        GRPC_TO_FMI_ARRAY(fmi2Real, value, response.value())
        FMI_REMOTE_RECORD_EXEC_END(GetRealOutputDerivatives, start_record);
        return gStatus2fmi2Status(response.ret());
    } else {
        g_component_env_map.at(c).callback.logger(nullptr, g_component_env_map[c].name.c_str(), fmi2Fatal, "fmi2Error",
                                                  "Error %d - %s", status.error_code(),
                                                  status.error_message().c_str());
        FMI_REMOTE_RECORD_EXEC_END(GetRealOutputDerivatives, start_record);
        return fmi2Fatal;
    }
}


//Define CUSTOM_GetStatus to manually specify an implementation
extern "C" fmi2Status fmi2GetStatus(fmi2Component c, fmi2StatusKind s, fmi2Status *value) {
    auto start_record = FMI_REMOTE_RECORD_EXEC_START(GetStatus);

    auto request = GetStatusRequest();
    auto response = GetStatusResponse();

    //argument handling
    request.set_c(TO_UINT(c));
    request.set_s(fmi2StatusKindToAnon_enum2(s));

    auto status = stub_->GetStatus(request, &response);
    if (status.ok()) {
        *value = Anon_enum0Tofmi2Status(response.value());
        FMI_REMOTE_RECORD_EXEC_END(GetStatus, start_record);
        return gStatus2fmi2Status(response.ret());
    } else {
        g_component_env_map.at(c).callback.logger(nullptr, g_component_env_map[c].name.c_str(), fmi2Fatal, "fmi2Error",
                                                  "Error %d - %s", status.error_code(),
                                                  status.error_message().c_str());
        FMI_REMOTE_RECORD_EXEC_END(GetStatus, start_record);
        return fmi2Fatal;
    }
}


//Define CUSTOM_GetRealStatus to manually specify an implementation
extern "C" fmi2Status fmi2GetRealStatus(fmi2Component c, fmi2StatusKind s, fmi2Real *value) {
    auto start_record = FMI_REMOTE_RECORD_EXEC_START(GetRealStatus);

    auto request = GetRealStatusRequest();
    auto response = GetRealStatusResponse();

    //argument handling
    request.set_c(TO_UINT(c));
    request.set_s(fmi2StatusKindToAnon_enum2(s));

    auto status = stub_->GetRealStatus(request, &response);
    if (status.ok()) {
        *value = response.value(0);
        FMI_REMOTE_RECORD_EXEC_END(GetRealStatus, start_record);
        return gStatus2fmi2Status(response.ret());
    } else {
        g_component_env_map.at(c).callback.logger(nullptr, g_component_env_map[c].name.c_str(), fmi2Fatal, "fmi2Error",
                                                  "Error %d - %s", status.error_code(),
                                                  status.error_message().c_str());
        FMI_REMOTE_RECORD_EXEC_END(GetRealStatus, start_record);
        return fmi2Fatal;
    }
}


//Define CUSTOM_GetIntegerStatus to manually specify an implementation
extern "C" fmi2Status fmi2GetIntegerStatus(fmi2Component c, fmi2StatusKind s, fmi2Integer *value) {
    auto start_record = FMI_REMOTE_RECORD_EXEC_START(GetIntegerStatus);

    auto request = GetIntegerStatusRequest();
    auto response = GetIntegerStatusResponse();

    //argument handling
    request.set_c(TO_UINT(c));
    request.set_s(fmi2StatusKindToAnon_enum2(s));

    auto status = stub_->GetIntegerStatus(request, &response);
    if (status.ok()) {
        *value = response.value(0);
        FMI_REMOTE_RECORD_EXEC_END(GetIntegerStatus, start_record);
        return gStatus2fmi2Status(response.ret());
    } else {
        g_component_env_map.at(c).callback.logger(nullptr, g_component_env_map[c].name.c_str(), fmi2Fatal, "fmi2Error",
                                                  "Error %d - %s", status.error_code(),
                                                  status.error_message().c_str());
        FMI_REMOTE_RECORD_EXEC_END(GetIntegerStatus, start_record);
        return fmi2Fatal;
    }
}


//Define CUSTOM_GetBooleanStatus to manually specify an implementation
extern "C" fmi2Status fmi2GetBooleanStatus(fmi2Component c, fmi2StatusKind s, fmi2Boolean *value) {
    auto start_record = FMI_REMOTE_RECORD_EXEC_START(GetBooleanStatus);

    auto request = GetBooleanStatusRequest();
    auto response = GetBooleanStatusResponse();

    //argument handling
    request.set_c(TO_UINT(c));
    request.set_s(fmi2StatusKindToAnon_enum2(s));

    auto status = stub_->GetBooleanStatus(request, &response);
    if (status.ok()) {
        *value = response.value(0);
        FMI_REMOTE_RECORD_EXEC_END(GetBooleanStatus, start_record);
        return gStatus2fmi2Status(response.ret());
    } else {
        g_component_env_map.at(c).callback.logger(nullptr, g_component_env_map[c].name.c_str(), fmi2Fatal, "fmi2Error",
                                                  "Error %d - %s", status.error_code(),
                                                  status.error_message().c_str());
        FMI_REMOTE_RECORD_EXEC_END(GetBooleanStatus, start_record);
        return fmi2Fatal;
    }
}


//Define CUSTOM_GetStringStatus to manually specify an implementation
extern "C" fmi2Status fmi2GetStringStatus(fmi2Component c, fmi2StatusKind s, fmi2String *value) {
    auto start_record = FMI_REMOTE_RECORD_EXEC_START(GetStringStatus);

    auto request = GetStringStatusRequest();
    auto response = GetStringStatusResponse();

    //argument handling
    request.set_c(TO_UINT(c));
    request.set_s(fmi2StatusKindToAnon_enum2(s));

    auto status = stub_->GetStringStatus(request, &response);
    if (status.ok()) {
        *value = strdup(response.value(0).c_str());
        FMI_REMOTE_RECORD_EXEC_END(GetStringStatus, start_record);
        return gStatus2fmi2Status(response.ret());
    } else {
        g_component_env_map.at(c).callback.logger(nullptr, g_component_env_map[c].name.c_str(), fmi2Fatal, "fmi2Error",
                                                  "Error %d - %s", status.error_code(),
                                                  status.error_message().c_str());
        FMI_REMOTE_RECORD_EXEC_END(GetStringStatus, start_record);
        return fmi2Fatal;
    }
}


extern "C" void fmi2FreeInstance(fmi2Component c) {
    auto start_record = FMI_REMOTE_RECORD_EXEC_START(FreeInstance);
    auto request = FreeInstanceRequest();
    auto response = FreeInstanceResponse();

    //argument handling
    request.set_c(static_cast<uint32_t>(reinterpret_cast<std::uintptr_t>(c)));
    auto status = stub_->FreeInstance(request, &response);
    if (status.ok()) {
    } else {
        g_component_env_map.at(c).callback.logger(nullptr, g_component_env_map[c].name.c_str(), fmi2Fatal, "fmi2Error",
                                                  "Error %d - %s", status.error_code(), status.error_message().c_str());
    }
    FMI_REMOTE_RECORD_EXEC_END(FreeInstance, start_record);
    if (g_show_statistics) {
        showFmiExecutionStatistics();
    }
}
