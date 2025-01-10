//
// Created by Guldbrandt Lausdahl, Kenneth on 06/01/2025.
//

#ifndef CUSTOM_STATISTICS_H
#define CUSTOM_STATISTICS_H
#include "fmi_metrics.h"


struct FmiExecInfo {
    int count;
    std::chrono::duration<double> duration;

    FmiExecInfo() : duration(std::chrono::duration<double>::zero()), count(0) {
    }
};

void showFmiExecutionStatistics();

void showStatistics(FmiExecInfo executions[]);

extern bool g_show_statistics;
extern FmiExecInfo fmi_function_executions[FMI_FUNCTION_COUNT];
#endif //CUSTOM_STATISTICS_H
