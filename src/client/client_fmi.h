//
// Created by Guldbrandt Lausdahl, Kenneth on 02/01/2025.
//

#ifndef CLIENT_FMI_H
#define CLIENT_FMI_H

#include "fmi2Functions.h"
#include <map>
#include "Fmi2ZmqClientTransport.h"

struct FmiComponentEnv {
    fmi2CallbackFunctions callback;
    std::string name;

    FmiComponentEnv(std::string name, fmi2CallbackFunctions callback): name(std::move(name)), callback(callback) {
    }

    FmiComponentEnv(): callback({}), name(nullptr) {
    }
};

static std::map<fmi2Component, FmiComponentEnv> g_component_env_map;

extern std::unique_ptr<COMMUNICATION_STUB_TYPE> stub_;

void establish_remote_connection(fmi2String instanceName,const std::string &fmuResourceLocation,const fmi2CallbackFunctions *functions);

#endif //CLIENT_FMI_H
