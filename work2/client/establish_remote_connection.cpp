//
// Created by Guldbrandt Lausdahl, Kenneth on 06/01/2025.
//
#include <filesystem>
#include <rapidjson/document.h>
#include <fstream>
#include <rapidjson/istreamwrapper.h>
#include "fmi2.grpc.pb.h"
#include <uri.h>
#include <grpcpp/grpcpp.h>
#include "execution_statistics.h"
namespace fs = std::filesystem;
std::unique_ptr<COMMUNICATION_STUB_TYPE> stub_;
const char * remote_url;


void establish_remote_connection(const std::string &fmuResourceLocation) {
    fs::path basePath = URIToNativePath(fmuResourceLocation.c_str());
    auto config = basePath / "remote-config.json";
    if (fs::exists(config)) {
        using namespace std;
        using namespace rapidjson;
        ifstream ifs(config);
        IStreamWrapper isw(ifs);

        Document d;
        d.ParseStream(isw);

        string url = "";



        if (d.IsObject()) {
            if (d.HasMember("RemoteConnection") && d["RemoteConnection"].IsObject()) {
                auto connectionObj = d["RemoteConnection"].GetObject();

                if (connectionObj.HasMember("url") && connectionObj["url"].IsString()) {
                    url = connectionObj["url"].GetString();
                } else {
                    cout << "No remote connection found" << endl;
                }

                if (connectionObj.HasMember("ShowStatistics") && connectionObj["ShowStatistics"].IsBool() && connectionObj["ShowStatistics"].GetBool()) {
                    g_show_statistics=true;
                }
            }
        }


        if (url.empty()) {
            cout << "Could not make connection" << endl;
        } else {
            cout << "Establishing connection to remote endpoint at URL: " << url << endl;
            remote_url = strdup(url.c_str());
            stub_ = Fmi2Service::NewStub(grpc::CreateChannel(
                url, grpc::InsecureChannelCredentials()));
        }
    } else {
        std::cerr << "Could not find remote config at " << config << std::endl;
    }
}