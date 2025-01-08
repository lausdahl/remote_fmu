//
// Created by Guldbrandt Lausdahl, Kenneth on 06/01/2025.
//
#include "fmi2.grpc.pb.h"
#include <cstring> //memcopy
#include <grpc_fmi_enums.h>
#include "execution_statistics.h"


bool g_show_statistics = false;


FmiExecInfo fmi_function_executions[FMI_FUNCTION_COUNT];

std::chrono::time_point<std::chrono::high_resolution_clock> record_exec_start(FmiFunctionNames name) {
    fmi_function_executions[static_cast<int>(name)].count++;
    return std::chrono::high_resolution_clock::now();
}

void record_exec_end(FmiFunctionNames name, std::chrono::time_point<std::chrono::high_resolution_clock> start_point) {
    fmi_function_executions[static_cast<int>(name)].duration += (
        std::chrono::high_resolution_clock::now() - start_point);
}

void showStatistics(FmiExecInfo executions[]) {

    int total_count = 0;
    std::chrono::duration<double> total_duration = std::chrono::duration<double>::zero();
    std::cout << std::left << std::setw(50) << "EXECUTION STATISTICS" << std::setw(10) << "Count" << std::setw(20) <<"Duration(ns)"<< std::setw(20)<<"Duration(ms)"<< std::setw(20)<<"Duration(s)"<<std::endl;
    for (int i = 0; i < FMI_FUNCTION_COUNT; i++) {
        std::cout << std::left << std::setw(50) << FmiFunctionNamesStr[i] << std::setw(10) << executions[i].count <<
                std::setw(20) << std::chrono::duration_cast<std::chrono::nanoseconds>(executions[i].duration).count() <<
                    std::setw(20) << std::chrono::duration_cast<std::chrono::milliseconds>(executions[i].duration).count() <<
                std::setw(20) << std::chrono::duration_cast<std::chrono::seconds>(executions[i].duration).count() <<
                std::endl;
        total_count += executions[i].count;
        total_duration += executions[i].duration;
    }
    std::cout << std::left << std::setw(50) << "Total"  << std::setw(10)<< total_count<< std::setw(20)<<std::chrono::duration_cast<std::chrono::nanoseconds>(total_duration).count()<< std::setw(20)<<std::chrono::duration_cast<std::chrono::milliseconds>(total_duration).count()<< std::setw(20)<<std::chrono::duration_cast<std::chrono::seconds>(total_duration).count()<< std::endl;
    std::cout << std::left << std::setw(50) << "Avg"  << std::setw(10)<< "1"<< std::setw(20)<<std::chrono::duration_cast<std::chrono::nanoseconds>(total_duration).count()/(double)total_count<< std::setw(20)<<std::chrono::duration_cast<std::chrono::milliseconds>(total_duration).count()/(double)total_count<< std::setw(20)<<std::chrono::duration_cast<std::chrono::seconds>(total_duration).count()/(double)total_count<< std::endl;

}

void showFmiExecutionStatistics() {
    showStatistics(fmi_function_executions);
}
