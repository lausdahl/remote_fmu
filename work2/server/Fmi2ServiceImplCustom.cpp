//
// Created by Guldbrandt Lausdahl, Kenneth on 28/11/2024.
//
#include "Fmi2ServiceImpl.h"
#include "fmi2Functions.h"
#include "grpc_fmi_mapping.h"
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using namespace std;

// #define GRPC_DECLARE_FMI_ARRAY(fmiType,variableName,size) \
// fmiType variableName[size];
//
// #define GRPC_TO_FMI_ARRAY(fmiType,variableName,grpcVar) \
// auto variableName##_size = grpcVar.size();\
// fmiType variableName[variableName##_size];\
// for (int i = 0; i < variableName##_size; i++) {\
//     variableName[i] = static_cast<fmiType>(grpcVar[i]);\
// }
//
// #define GRPC_FROM_FMI_ARRAY(variableName,size,grpcSetName)\
// for (int i = 0; i < size; i++) {\
//     reply->grpcSetName(variableName[i]);\
// }


Status Fmi2ServiceImpl::SetDebugLogging(ServerContext *context, const SetDebugLoggingRequest *request,
                                        Fmi2StatusResponse *reply) {
    auto start_record = FMI_REMOTE_RECORD_EXEC_START(SetDebugLogging)

    auto component = getComponent(request->c());
    if (component == nullptr) {
        reply->set_ret(fmi2StatusToAnon_enum0(fmi2Error));
        FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
        return Status::OK;
    }

    auto value_size = request->categories_size();
    fmi2String values[value_size];
    for (int i = 0; i < value_size; i++) {
        values[i] = request->categories()[i].c_str();
    }

    reply->set_ret(fmi2StatusToAnon_enum0(
        component->fmu->setDebugLogging(component->comp, request->loggingon(), request->categories().size(), values)));
    FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
    return Status::OK;
}


Status Fmi2ServiceImpl::Instantiate(ServerContext *context, const InstantiateRequest *request,
                                    InstantiateResponse *reply) {
    auto start_record = FMI_REMOTE_RECORD_EXEC_START(Instantiate)
    const auto guid = request->fmuguid();

    if (this->available_fmus.find(guid) != this->available_fmus.end()) {
        cout << "Fmi2ServiceImpl::Instantiate: " << guid << " fmu is available" << std::endl;
        const auto fmu = this->available_fmus.find(guid)->second;


    std:
        const shared_ptr<Fmi2Comp> instance(fmu->instantiate(request->instancename().c_str(), request->visible(),
                                                             request->loggingon()));

        if (instance == nullptr) {
            reply->set_ret(0);
        } else {
            this->fmu_instances.push_back(instance);
            auto index = this->fmu_instances.size();
            cout << "New instance for " << guid << " added at index-1 " << index << endl;
            reply->set_ret(index);
        }
    } else {
        cout << "Requested GUID " << guid << " not found" << endl;
        reply->set_ret(0);
    }
    FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
    return Status::OK;
}


//Define CUSTOM_SetString to manually specify an implementation
Status Fmi2ServiceImpl::SetString(ServerContext *context, const SetStringRequest *request, Fmi2StatusResponse *reply) {
    auto start_record = FMI_REMOTE_RECORD_EXEC_START(SetString)
    auto component = getComponent(request->c());
    if (component == nullptr) {
        reply->set_ret(fmi2StatusToAnon_enum0(fmi2Error));
        FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
        return Status::OK;
    }

    auto vr_size = request->vr().size();
    fmi2ValueReference vr[vr_size];
    for (int i = 0; i < vr_size; i++) {
        vr[i] = static_cast<fmi2ValueReference>(request->vr()[i]);
    }
    auto value_size = request->value_size();
    fmi2String values[value_size];
    for (int i = 0; i < value_size; i++) {
        values[i] = request->value()[i].c_str();
    }
    auto status = component->fmu->setString(component->comp, vr, vr_size, values);
    reply->set_ret(fmi2StatusToAnon_enum0(status));
    FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
    return Status::OK;
}

void showStatistics(FmiExecInfo executions[]) {
    std::cout<< std::left<< std::setw(50) <<  "EXECUTION STATISTICS"  << std::setw(10)<<"Count"<< std::setw(20)<<"Duration(ns)"<< std::setw(20)<<"Duration(ms)"<< std::setw(20)<<"Duration(s)"<<std::endl;
    int total_count = 0;
    std::chrono::duration<double> total_duration = std::chrono::duration<double>::zero();
    for(int i = 0 ; i<FMI_FUNCTION_COUNT;i++) {
        std::cout << std::left << std::setw(50) << FmiFunctionNamesStr[i]  << std::setw(10)<< executions[i].count<< std::setw(20)<<std::chrono::duration_cast<std::chrono::nanoseconds>(executions[i].duration).count()<< std::setw(20)<<std::chrono::duration_cast<std::chrono::milliseconds>(executions[i].duration).count()<< std::setw(20)<<std::chrono::duration_cast<std::chrono::seconds>(executions[i].duration).count()<< std::endl;
total_count += executions[i].count;
        total_duration += executions[i].duration;
    }

    std::cout << std::left << std::setw(50) << "Total"  << std::setw(10)<< total_count<< std::setw(20)<<std::chrono::duration_cast<std::chrono::nanoseconds>(total_duration).count()<< std::setw(20)<<std::chrono::duration_cast<std::chrono::milliseconds>(total_duration).count()<< std::setw(20)<<std::chrono::duration_cast<std::chrono::seconds>(total_duration).count()<< std::endl;
    std::cout << std::left << std::setw(50) << "Avg"  << std::setw(10)<< "1"<< std::setw(20)<<std::chrono::duration_cast<std::chrono::nanoseconds>(total_duration).count()/(double)total_count<< std::setw(20)<<std::chrono::duration_cast<std::chrono::milliseconds>(total_duration).count()/(double)total_count<< std::setw(20)<<std::chrono::duration_cast<std::chrono::seconds>(total_duration).count()/(double)total_count<< std::endl;
}

Status Fmi2ServiceImpl::FreeInstance(ServerContext *context, const FreeInstanceRequest *request,
                                     FreeInstanceResponse *reply) {
    auto start_record = FMI_REMOTE_RECORD_EXEC_START(FreeInstance)
    auto component = getComponent(request->c());
    if (component == nullptr) {
        FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
        return Status::OK;
    }

    component->fmu->freeInstance(component->comp);

    showStatistics(this->fmi_function_executions);
    FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
    return Status::OK;
}


Status Fmi2ServiceImpl::GetFMUstate(ServerContext *context, const GetFMUstateRequest *request,
                                    GetFMUstateResponse *reply) {
    auto start_record = FMI_REMOTE_RECORD_EXEC_START(GetFMUstate)
    reply->set_ret(fmi2StatusToAnon_enum0(fmi2Discard));
    FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
    return Status::OK;
}

Status Fmi2ServiceImpl::SetFMUstate(ServerContext *context, const SetFMUstateRequest *request,
                                    Fmi2StatusResponse *reply) {
    auto start_record = FMI_REMOTE_RECORD_EXEC_START(SetFMUstate)
    reply->set_ret(fmi2StatusToAnon_enum0(fmi2Discard));
    FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
    return Status::OK;
}

Status Fmi2ServiceImpl::FreeFMUstate(ServerContext *context, const FreeFMUstateRequest *request,
                                     FreeFMUstateResponse *reply) {
    auto start_record = FMI_REMOTE_RECORD_EXEC_START(FreeFMUstate)
    reply->set_ret(fmi2StatusToAnon_enum0(fmi2Discard));
    FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
    return Status::OK;
}

Status Fmi2ServiceImpl::SerializedFMUstateSize(ServerContext *context, const SerializedFMUstateSizeRequest *request,
                                               SerializedFMUstateSizeResponse *reply) {
    auto start_record = FMI_REMOTE_RECORD_EXEC_START(SerializedFMUstateSize)
    reply->set_ret(fmi2StatusToAnon_enum0(fmi2Discard));
    FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
    return Status::OK;
}

Status Fmi2ServiceImpl::SerializeFMUstate(ServerContext *context, const SerializeFMUstateRequest *request,
                                          SerializeFMUstateResponse *reply) {
    auto start_record = FMI_REMOTE_RECORD_EXEC_START(SerializeFMUstate)
    reply->set_ret(fmi2StatusToAnon_enum0(fmi2Discard));
    FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
    return Status::OK;
}

Status Fmi2ServiceImpl::DeSerializeFMUstate(ServerContext *context, const DeSerializeFMUstateRequest *request,
                                            DeSerializeFMUstateResponse *reply) {
    auto start_record = FMI_REMOTE_RECORD_EXEC_START(DeSerializeFMUstate)
    reply->set_ret(fmi2StatusToAnon_enum0(fmi2Discard));
    FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
    return Status::OK;
}


Status Fmi2ServiceImpl::GetDirectionalDerivative(ServerContext *context, const GetDirectionalDerivativeRequest *request,
                                                 GetDirectionalDerivativeResponse *reply) {
    auto start_record = FMI_REMOTE_RECORD_EXEC_START(GetDirectionalDerivative)
    auto component = getComponent(request->c());
    if (component == nullptr) {
        reply->set_ret(fmi2StatusToAnon_enum0(fmi2Error));
        FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
        return Status::OK;
    }

    GRPC_TO_FMI_ARRAY(fmi2ValueReference, vunknown_ref, request->vunknown_ref())
    GRPC_TO_FMI_ARRAY(fmi2ValueReference, vknown_ref, request->vknown_ref())
    GRPC_TO_FMI_ARRAY(fmi2Real, dvknown, request->dvknown())
    GRPC_DECLARE_FMI_ARRAY(fmi2Real, dvUnknown, dvknown_size)
    //FIXME not sure this is correct

    reply->set_ret(fmi2StatusToAnon_enum0(component->fmu->getDirectionalDerivative(
        component->comp, vunknown_ref, vknown_ref_size, vknown_ref, vknown_ref_size, dvknown, dvUnknown)));

    GRPC_REPLY_FROM_FMI_ARRAY(dvUnknown, dvknown_size, dvunknown)
    FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
    return Status::OK;
}


Status Fmi2ServiceImpl::NewDiscreteStates(ServerContext *context, const NewDiscreteStatesRequest *request,
                                          NewDiscreteStatesResponse *reply) {
    auto start_record = FMI_REMOTE_RECORD_EXEC_START(NewDiscreteStates)
    reply->set_ret(fmi2StatusToAnon_enum0(fmi2Discard));
    FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
    return Status::OK;
}

Status Fmi2ServiceImpl::CompletedIntegratorStep(ServerContext *context, const CompletedIntegratorStepRequest *request,
                                                CompletedIntegratorStepResponse *reply) {
   auto start_record = FMI_REMOTE_RECORD_EXEC_START(CompletedIntegratorStep)
    reply->set_ret(fmi2StatusToAnon_enum0(fmi2Discard));
    FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
    return Status::OK;
}

Status Fmi2ServiceImpl::SetContinuousStates(ServerContext *context, const SetContinuousStatesRequest *request,
                                            Fmi2StatusResponse *reply) {

     auto start_record = FMI_REMOTE_RECORD_EXEC_START(SetContinuousStates)
    reply->set_ret(fmi2StatusToAnon_enum0(fmi2Discard));
    FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
    return Status::OK;
}

/*Model exchange
*
typedef fmi2Status fmi2EnterEventModeTYPE         (fmi2Component);
typedef fmi2Status fmi2NewDiscreteStatesTYPE      (fmi2Component, fmi2EventInfo*);
typedef fmi2Status fmi2EnterContinuousTimeModeTYPE(fmi2Component);
typedef fmi2Status fmi2CompletedIntegratorStepTYPE(fmi2Component, fmi2Boolean, fmi2Boolean*, fmi2Boolean*);

typedef fmi2Status fmi2SetTimeTYPE            (fmi2Component, fmi2Real);
typedef fmi2Status fmi2SetContinuousStatesTYPE(fmi2Component, const fmi2Real[], size_t);

typedef fmi2Status fmi2GetDerivativesTYPE               (fmi2Component, fmi2Real[], size_t);
typedef fmi2Status fmi2GetEventIndicatorsTYPE           (fmi2Component, fmi2Real[], size_t);
typedef fmi2Status fmi2GetContinuousStatesTYPE          (fmi2Component, fmi2Real[], size_t);
typedef fmi2Status fmi2GetNominalsOfContinuousStatesTYPE(fmi2Component, fmi2Real[], size_t);

 */


Status Fmi2ServiceImpl::GetDerivatives(ServerContext *context, const GetDerivativesRequest *request,
                                       GetDerivativesResponse *reply) {
    auto start_record = FMI_REMOTE_RECORD_EXEC_START(GetDerivatives)
    reply->set_ret(fmi2StatusToAnon_enum0(fmi2Discard));
    FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
    return Status::OK;
}


Status Fmi2ServiceImpl::GetEventIndicators(ServerContext *context, const GetEventIndicatorsRequest *request,
                                           GetEventIndicatorsResponse *reply) {
     auto start_record = FMI_REMOTE_RECORD_EXEC_START(GetEventIndicators)
    reply->set_ret(fmi2StatusToAnon_enum0(fmi2Discard));
    FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
    return Status::OK;
}


Status Fmi2ServiceImpl::GetContinuousStates(ServerContext *context, const GetContinuousStatesRequest *request,
                                            GetContinuousStatesResponse *reply) {
    auto start_record = FMI_REMOTE_RECORD_EXEC_START(GetContinuousStates)
    reply->set_ret(fmi2StatusToAnon_enum0(fmi2Discard));
    FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
    return Status::OK;
}

Status Fmi2ServiceImpl::GetNominalsOfContinuousStates(ServerContext *context,
                                                      const GetNominalsOfContinuousStatesRequest *request,
                                                      GetNominalsOfContinuousStatesResponse *reply) {
    auto start_record = FMI_REMOTE_RECORD_EXEC_START(GetNominalsOfContinuousStates)
    reply->set_ret(fmi2StatusToAnon_enum0(fmi2Discard));
    FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
    return Status::OK;
}


Status Fmi2ServiceImpl::SetRealInputDerivatives(ServerContext *context, const SetRealInputDerivativesRequest *request,
                                                Fmi2StatusResponse *reply) {
    auto start_record = FMI_REMOTE_RECORD_EXEC_START(SetRealInputDerivatives)
    auto component = getComponent(request->c());
    if (component == nullptr) {
        reply->set_ret(fmi2StatusToAnon_enum0(fmi2Error));
        FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
        return Status::OK;
    }

    GRPC_TO_FMI_ARRAY(fmi2ValueReference, vr, request->vr())
    GRPC_TO_FMI_ARRAY(fmi2Integer, order, request->order())
    GRPC_TO_FMI_ARRAY(fmi2Real, value, request->value())

    reply->set_ret(fmi2StatusToAnon_enum0(
        component->fmu->setRealInputDerivatives(component->comp, vr, vr_size, order,
                                                value)));

    FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
    return Status::OK;
}


Status Fmi2ServiceImpl::GetRealOutputDerivatives(ServerContext *context, const GetRealOutputDerivativesRequest *request,
                                                 GetRealOutputDerivativesResponse *reply) {
      auto start_record = FMI_REMOTE_RECORD_EXEC_START(GetRealOutputDerivatives)
    auto component = getComponent(request->c());
    if (component == nullptr) {
        reply->set_ret(fmi2StatusToAnon_enum0(fmi2Error));
        FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
        return Status::OK;
    }

    GRPC_TO_FMI_ARRAY(fmi2ValueReference, vr, request->vr())
    GRPC_TO_FMI_ARRAY(fmi2Integer, order, request->order())
    GRPC_DECLARE_FMI_ARRAY(fmi2Real, value, vr_size)


    reply->set_ret(fmi2StatusToAnon_enum0(
        component->fmu->getRealOutputDerivatives(component->comp, vr, vr_size, order, value)));

    GRPC_REPLY_FROM_FMI_ARRAY(value, vr_size, value)
    FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
    return Status::OK;
}


Status Fmi2ServiceImpl::GetStatus(ServerContext *context, const GetStatusRequest *request, GetStatusResponse *reply) {

    auto start_record = FMI_REMOTE_RECORD_EXEC_START(GetStatus)
    auto component = getComponent(request->c());
    if (component == nullptr) {
        reply->set_ret(fmi2StatusToAnon_enum0(fmi2Error));
        FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
        return Status::OK;
    }
    fmi2Status status = fmi2Fatal;

    reply->set_ret(
        fmi2StatusToAnon_enum0(
            component->fmu->getStatus(component->comp, Anon_enum2Tofmi2StatusKind(request->s()), &status)));

     reply->set_value(fmi2StatusToAnon_enum0(status));
    FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
    return Status::OK;
}

Status Fmi2ServiceImpl::GetRealStatus(ServerContext *context, const GetRealStatusRequest *request,
                                      GetRealStatusResponse *reply) {
   auto start_record = FMI_REMOTE_RECORD_EXEC_START(GetRealStatus)
    auto component = getComponent(request->c());
    if (component == nullptr) {
        reply->set_ret(fmi2StatusToAnon_enum0(fmi2Error));
        FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
        return Status::OK;
    }

    fmi2Real value;
    reply->set_ret(
        fmi2StatusToAnon_enum0(
            component->fmu->getRealStatus(component->comp, Anon_enum2Tofmi2StatusKind(request->s()), &value)));
reply->set_value(0,value);

    FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
    return Status::OK;
}


Status Fmi2ServiceImpl::GetIntegerStatus(ServerContext *context, const GetIntegerStatusRequest *request,
                                         GetIntegerStatusResponse *reply) {
      auto start_record = FMI_REMOTE_RECORD_EXEC_START(GetIntegerStatus)
    auto component = getComponent(request->c());
    if (component == nullptr) {
        reply->set_ret(fmi2StatusToAnon_enum0(fmi2Error));
        FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
        return Status::OK;
    }

    fmi2Integer value;
    reply->set_ret(
        fmi2StatusToAnon_enum0(
            component->fmu->getIntegerStatus(component->comp, Anon_enum2Tofmi2StatusKind(request->s()), &value)));
    reply->set_value(0,value);

    FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
    return Status::OK;
}


Status Fmi2ServiceImpl::GetBooleanStatus(ServerContext *context, const GetBooleanStatusRequest *request,
                                         GetBooleanStatusResponse *reply) {
    auto start_record = FMI_REMOTE_RECORD_EXEC_START(GetBooleanStatus)
    auto component = getComponent(request->c());
    if (component == nullptr) {
        reply->set_ret(fmi2StatusToAnon_enum0(fmi2Error));
        FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
        return Status::OK;
    }

    fmi2Boolean value;
    reply->set_ret(
        fmi2StatusToAnon_enum0(
            component->fmu->getBooleanStatus(component->comp, Anon_enum2Tofmi2StatusKind(request->s()), &value)));
    reply->set_value(0,value);


    FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
    return Status::OK;
}


Status Fmi2ServiceImpl::GetStringStatus(ServerContext *context, const GetStringStatusRequest *request,
                                        GetStringStatusResponse *reply) {
    auto start_record = FMI_REMOTE_RECORD_EXEC_START(GetStringStatus)
    auto component = getComponent(request->c());
    if (component == nullptr) {
        reply->set_ret(fmi2StatusToAnon_enum0(fmi2Error));
        FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
        return Status::OK;
    }

    fmi2String value;
    reply->set_ret(
        fmi2StatusToAnon_enum0(
            component->fmu->getStringStatus(component->comp, Anon_enum2Tofmi2StatusKind(request->s()), &value)));
    reply->set_value(0,std::string(value));

    FMI_REMOTE_RECORD_EXEC_END(SetupExperiment,start_record);
    return Status::OK;
}
