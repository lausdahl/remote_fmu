#include <iostream>
#include <memory>
#include <SimFmi2.h>
#include <string>
#include <utility>
#include "fmi2.pb.h"
#include <vector>
#include <regex>
#include <unordered_map>
#include "Fmi2ZmqServerTransport.h"
#include "Fmi2ServiceImpl.h"


int fmi_function_executions[FMI_FUNCTION_COUNT];


// Struct to store each tuple (path, name)
struct ArgumentTuple {
    std::string path;
    std::string guid;
};

// Validate path ends with .fmu
bool validatePath(const std::string &path) {
    return std::regex_match(path, std::regex(".*\\.fmu$"));
}

// Function to parse command-line arguments
void parseArguments(int argc, char *argv[], std::string &port, std::vector<ArgumentTuple> &tuples) {
    bool portSet = false;

    for (int i = 1; i < argc;) {
        std::string arg = argv[i];

        if (arg == "--port" && i + 1 < argc && !portSet) {
            port = argv[++i];
            portSet = true;
            ++i;
        } else if (arg == "--path" && i + 1 < argc) {
            ArgumentTuple tuple;
            tuple.path = argv[++i];
            if (!validatePath(tuple.path)) {
                std::cerr << "Error: --path must end with .fmu\n";
                exit(EXIT_FAILURE);
            }
            ++i;

            if (i < argc && std::string(argv[i]) == "--guid" && i + 1 < argc) {
                tuple.guid = argv[++i];
                ++i;
            } else {
                std::cerr << "Error: Missing or malformed --guid argument.\n";
                exit(EXIT_FAILURE);
            }

            tuples.push_back(tuple);
        } else {
            std::cerr << "Unknown or malformed argument: " << arg << "\n";
            exit(EXIT_FAILURE);
        }
    }

    if (!portSet) {
        //std::cerr << "Error: --port argument is required.\n";
        //exit(EXIT_FAILURE);
        port = "50051";
    }
}

void RunServer(const std::string &port, std::map<std::string, std::shared_ptr<Fmi2Impl> > allowed_fmus) {
    std::string server_address(std::string("tcp://*:") + port);
    Fmi2ServiceImpl service(std::move(allowed_fmus));

    auto server = Fmi2ZmqServerTransport(server_address);
    server.RegisterService(&service);
    server.Start();
}

int main(int argc, char **argv) {
    std::string port;
    std::vector<ArgumentTuple> fmus;

    parseArguments(argc, argv, port, fmus);

    std::cout << "Starting server with port: " << port << "\n\n";

    std::map<std::string, std::shared_ptr<Fmi2Impl> > allowed_fmus;
    // allowed_fmus["{12345678-9999-9999-9999-000000000000}"] = load_FMI2("{12345678-9999-9999-9999-000000000000}",
    //                                                                    "fmi2functiontest.fmu");

    // allowed_fmus["{8c4e810f-3df3-4a00-8276-176fa3c9f000}"] = load_FMI2("{8c4e810f-3df3-4a00-8276-176fa3c9f000}",
    //                                                                   "/Users/kel/data/au/into-cps-association/maestro/maestro/src/test/resources/watertankcontroller-c.fmu");

    if (!fmus.empty()) {
        allowed_fmus.clear();
    }


    for (size_t i = 0; i < fmus.size(); ++i) {
        std::cout << "Loading FMU" << i + 1 << ":\n";
        std::cout << "  Path: " << fmus[i].path << "\n";
        std::cout << "  GUID: " << fmus[i].guid << "\n";
        allowed_fmus[fmus[i].guid] = load_FMI2(fmus[i].guid.c_str(),
                                               fmus[i].path.c_str());
    }

    std::cout << "\n\n";
    RunServer(port, allowed_fmus);
    return EXIT_SUCCESS;
}
