#include <iostream>
#include <string>
#include <uri.h>

#include <grpcpp/grpcpp.h>
#include "fmi2.grpc.pb.h"
#include <stdio.h>
#include <stdlib.h>
#include "fmi2Functions.h"
#include "fmi_metrics.h"
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
#ifndef FMI2_H
#define FMI2_H


#endif

#include <filesystem>
#include <regex>
#include <stdexcept>
#include <sstream>
#include "execution_statistics.h"


namespace fs = std::filesystem;

void fmi2Logger(
    fmi2ComponentEnvironment componentEnvironment,
    fmi2String instanceName,
    fmi2Status status,
    fmi2String category,
    fmi2String message,
    ...) {
    // Convert fmi2Status to a readable string
    const char *statusStr;
    switch (status) {
        case fmi2OK: statusStr = "OK";
            break;
        case fmi2Warning: statusStr = "Warning";
            break;
        case fmi2Discard: statusStr = "Discard";
            break;
        case fmi2Error: statusStr = "Error";
            break;
        case fmi2Fatal: statusStr = "Fatal";
            break;
        case fmi2Pending: statusStr = "Pending";
            break;
        default: statusStr = "Unknown";
            break;
    }

    // Print the log message
    printf("[%s][%s] %s: ", instanceName, category, statusStr);

    // Process the variable arguments
    va_list args;
    va_start(args, message);
    vprintf(message, args);
    va_end(args);

    printf("\n");
}


#define STEP_SIZE 0.01   // Simulation step size
#define STOP_TIME 100.0    // Simulation stop time

void runFmiSimulation(const char *name, const char *guid, const char *resourcePath) {
    // Instantiate the FMI callbacks
    fmi2CallbackFunctions callbacks = {
        .logger = fmi2Logger,
        .allocateMemory = calloc,
        .freeMemory = free,
        .stepFinished = NULL,
        .componentEnvironment = NULL
    };

    // Load the FMU (assuming a shared library mechanism)
    fmi2Component component = fmi2Instantiate(name, fmi2CoSimulation, guid, resourcePath, &callbacks, fmi2False,
                                              fmi2False);
    if (!component) {
        printf("Failed to instantiate FMU.\n");
        return;
    }

    // Initialize the FMU
    fmi2Status status = fmi2SetupExperiment(component, fmi2False, 0.0, 0.0, fmi2True, STOP_TIME);
    if (status != fmi2OK) {
        printf("Failed to set up experiment.\n");
        fmi2FreeInstance(component);
        return;
    }

    status = fmi2EnterInitializationMode(component);
    if (status != fmi2OK) {
        printf("Failed to enter initialization mode.\n");
        fmi2FreeInstance(component);
        return;
    }

    status = fmi2ExitInitializationMode(component);
    if (status != fmi2OK) {
        printf("Failed to exit initialization mode.\n");
        fmi2FreeInstance(component);
        return;
    }

    // Perform the simulation loop
    double currentTime = 0.0;
    while (currentTime < STOP_TIME) {
        status = fmi2DoStep(component, currentTime, STEP_SIZE, fmi2True);
        if (status != fmi2OK) {
            printf("Simulation step failed at time %f.\n", currentTime);
            break;
        }

        currentTime += STEP_SIZE;
       // printf("Time: %f\n", currentTime);
    }

    // Clean up
    fmi2Terminate(component);
    fmi2FreeInstance(component);
}


std::string pathToURI(const std::filesystem::path &path) {
    std::ostringstream uri;
    uri << "file://";

#ifdef _WIN32
    // On Windows, replace backslashes with forward slashes
    std::string pathStr = path.string();
    for (char& ch : pathStr) {
        if (ch == '\\') ch = '/';
    }
    uri << '/' << pathStr;  // Add extra '/' for Windows drive letters
#else
    uri << path.string();
#endif

    return uri.str();
}

int main(int argc, char **argv) {
    auto guid = "{12345678-9999-9999-9999-000000000000}";
    std::filesystem::path currentPath = pathToURI(std::filesystem::current_path());
    std::cout << "Current path: " << currentPath << std::endl;

    runFmiSimulation("my first fmu", guid, currentPath.c_str());
    showStatistics(fmi_function_executions);
    return EXIT_SUCCESS;
}
