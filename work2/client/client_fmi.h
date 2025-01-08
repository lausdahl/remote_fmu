//
// Created by Guldbrandt Lausdahl, Kenneth on 02/01/2025.
//

#ifndef CLIENT_FMI_H
#define CLIENT_FMI_H

#include "fmi2Functions.h"
extern "C" {
#include "fmi2.h"
#include "sim_support.h"

}
#include <map>
#include <utility>
#include "Fmi2ZmqClientTransport.h"

struct FmiComponentEnv {
    fmi2CallbackFunctions callback;
    std::string name;
    FmiComponentEnv(std::string name, fmi2CallbackFunctions callback): name(std::move(name)), callback(callback) {}
    FmiComponentEnv():callback({}),name(nullptr){}
};

static std::map<fmi2Component, FmiComponentEnv> g_component_env_map;

extern  std::unique_ptr<COMMUNICATION_STUB_TYPE> stub_;
using grpc::ClientContext;

// inline void stepFinished(fmi2ComponentEnvironment, fmi2Status) {}
//
// static fmi2CallbackFunctions g_callback =  {
//     .logger = &fmuLogger, .allocateMemory = calloc, .freeMemory = free, .stepFinished=&stepFinished, .componentEnvironment = nullptr};
void establish_remote_connection(const std::string &fmuResourceLocation);

#endif //CLIENT_FMI_H
