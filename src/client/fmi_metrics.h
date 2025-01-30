//
// Created by Guldbrandt Lausdahl, Kenneth on 03/01/2025.
//

#ifndef FMI_METRICS_H
#define FMI_METRICS_H
#include <chrono>
#include "grpc_fmi_enums.h"

std::chrono::time_point<std::chrono::high_resolution_clock> record_exec_start(FmiFunctionNames name);

void record_exec_end(FmiFunctionNames name, std::chrono::time_point<std::chrono::high_resolution_clock> start_point);

#define FMI_REMOTE_RECORD_EXEC_START(name) record_exec_start(FmiFunctionNames::name);
#define FMI_REMOTE_RECORD_EXEC_END(name,start) record_exec_end(FmiFunctionNames::name, start);

#endif //FMI_METRICS_H
