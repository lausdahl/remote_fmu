//
// Created by Guldbrandt Lausdahl, Kenneth on 03/01/2025.
//

#ifndef GRPC_FMI_MAPPING_H
#define GRPC_FMI_MAPPING_H
#include "fmi2Functions.h"

#define TO_UINT(variable) static_cast<uint32_t>(reinterpret_cast<std::uintptr_t>(variable))

#define GRPC_DECLARE_FMI_ARRAY(fmiType,variableName,size) \
fmiType variableName[size];

#define GRPC_TO_FMI_ARRAY(fmiType,variableName,grpcVar) \
auto variableName##_size = grpcVar.size();\
fmiType variableName[variableName##_size];\
for (int i = 0; i < variableName##_size; i++) {\
variableName[i] = static_cast<fmiType>(grpcVar[i]);\
}

// #define GRPC_FROM_FMI_ARRAY(grpc_variable,variableName,size,grpcSetName)\
// for (int i = 0; i < size; i++) {\
// grpc_variable->grpcSetName(variableName[i]);\
// }

#define GRPC_REPLY_FROM_FMI_ARRAY(variableName,size,grpcSetName) \
for (int i = 0; i < size; i++) {\
reply->grpcSetName(variableName[i]);\
}

#define GRPC_REQUEST_FROM_FMI_ARRAY(variableName,size,grpcSetName)\
for (int i = 0; i < size; i++) {\
    request.grpcSetName(variableName[i]);\
}

#endif //GRPC_FMI_MAPPING_H
