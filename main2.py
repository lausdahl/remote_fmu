from abc import ABC

from pyclibrary import CParser, c_parser
import glob
from datetime import datetime
from pyclibrary.c_parser import flatten

header=f"""
/******************************************************************************

    GENERATED {str(datetime.now())}
    
******************************************************************************/

"""

header_files = glob.glob("FMI-Standard-2.0.4/**/*.h")

parser = CParser(header_files, cache="fmi_cache")

fmi2_to_proto_type = {'fmi2String': 'string', 'fmi2Component': 'uint32', 'fmi2FMUstate': 'uint32',
                      'fmi2EventInfo': 'uint32', 'fmi2CallbackFunctions': 'uint32'}

proto_types = [
    "double", "float", "int32", "int64", "uint32", "uint64", "sint32", "sint64",
    "fixed32", "fixed64", "sfixed32", "sfixed64", "bool", "string", "bytes",
    "enum", "message",
    "Any", "Duration", "Timestamp", "FieldMask", "Struct", "Value", "ListValue",
    "DoubleValue", "FloatValue", "Int64Value", "UInt64Value", "Int32Value",
    "UInt32Value", "BoolValue", "StringValue", "BytesValue", "google.protobuf.Empty"
]

cpp_to_protobuf = {
    "double": "double",
    "float": "float",
    "int": "int32",  # or "int64" depending on the range
    "long": "int64",
    "unsigned int": "uint32",
    "unsigned long": "uint64",
    "bool": "bool",
    "std::string": "string",
    "std::vector<uint8_t>": "bytes",
    "enum": "enum",
    "struct": "message",
    "std::chrono::duration": "Duration",
    "std::chrono::time_point": "Timestamp",
    "size_t": "uint64",
    "void": "google.protobuf.Empty",
    "char": "string",
}

indexing = {

    'fmi2SetDebugLogging': [{'index': 'nCategories', 'args': ['categories']}],
    'fmi2SerializeFMUstate': [{'index': 'size', 'args': ['serializedState']}],
    'fmi2DeSerializeFMUstate': [{'index': 'size', 'args': ['serializedState']}],
    'fmi2GetDirectionalDerivative': [{'index': 'nUnknown', 'args': ['vUnknown_ref', 'dvUnknown']},
                                     {'index': 'nKnown', 'args': ['vKnown_ref', 'dvKnown']}],
    'fmi2SetContinuousStates': [{'index': 'nx', 'args': ['x']}],
    'fmi2GetDerivatives': [{'index': 'nx', 'args': ['derivatives']}],
    'fmi2GetEventIndicators': [{'index': 'ni', 'args': ['eventIndicators']}],
    'fmi2GetContinuousStates': [{'index': 'nx', 'args': ['x']}],
    'fmi2GetNominalsOfContinuousStates': [{'index': 'nx', 'args': ['x_nominal']}],
    'fmi2SetRealInputDerivatives': [{'index': 'nvr', 'args': ['fmi2ValueReference']}],
    'fmi2GetRealOutputDerivatives': [{'index': 'nvr', 'args': ['fmi2ValueReference']}],

}

for t in ['Integer', 'Boolean', 'Real', 'String']:
    indexing['fmi2Get' + t] = [{'index': 'nvr', 'args': ['vr', 'value']}]
    indexing['fmi2Set' + t] = [{'index': 'nvr', 'args': ['vr', 'value']}]


class MyType(ABC):
    pass

    def is_array(self):
        return False

    def is_prt(self):
        return False

    def get_name(self):
        return None

    def is_function(self):
        return False


class CProxyType(MyType):
    def __init__(self, t: c_parser.Type, base_type: c_parser.Type = None):
        self.t = t
        self.base_type = base_type

    def get_name(self):
        if isinstance(self.t, str):
            return self.t
        return self.t.type_spec

    def is_array(self):
        if isinstance(self.t, str):
            return False
        return any([l for l in self.t.declarators if len(l) == 1] + (
            [l for l in self.base_type.declarators if len(l) == 1] if self.base_type else []))

    def is_prt(self):
        if isinstance(self.t, str):
            return False
        return '*' in self.t.declarators or '&' in self.t.declarators or (
                self.base_type and ('*' in self.base_type.declarators or '&' in self.base_type.declarators))

    def is_function(self):
        if isinstance(self.t, str):
            return False
        f_args =  [l for l in self.t.declarators if len(l) == 3] + (
            [l for l in self.base_type.declarators if len(l) == 3] if self.base_type else [])
        return len(f_args)>0 and all(f_args)

    def is_function_type(self):
        if isinstance(self.t, str):
            return False
        f_args =  [l for l in self.t.declarators if isinstance(l , tuple)] + (
            [l for l in self.base_type.declarators if isinstance(l , tuple)] if self.base_type else [])
        from itertools import chain
        f_args=list(chain.from_iterable(f_args))
        return len(f_args)>0 and all([len(f)==3 for f in f_args ])

def map2protobuf_type(t: CProxyType):
    if t.get_name() in fmi2_to_proto_type:
        return ("repeated " if t.is_array() else "") + fmi2_to_proto_type[t.get_name()]
    if t.get_name() in cpp_to_protobuf:
        return ("repeated " if t.is_array() else "") + cpp_to_protobuf[t.get_name()]

    enum_name,_ = get_enum(t)
    if enum_name:
        return enum_name
    lookup = [a for a in all_known_types if a[0] == t.get_name()]
    if len(lookup) == 1:
        tl = lookup[0][1]
    #     if tl[0].startswith("enum "):
    #         type_name = tl[0].split(" ")[1]
    #         type_name = type_name[0].upper() + type_name[1:]
    #         return type_name

        return map2protobuf_type(CProxyType(tl, base_type=t.t))

    return ("repeated " if t.is_array() else "") + t.get_name()


class Argument:
    def __init__(self, name: str, arg_type: MyType):
        self.name = name
        self.arg_type = arg_type

    def __str__(self):
        return f"{self.name}: {self.arg_type}"


class Function:
    def __init__(self, name: str, return_args: [Argument], args: [Argument], definition: (str, c_parser.Type)):
        self.name = name
        self.return_args = return_args
        self.args = args
        self.definition = definition

    def __str__(self):
        return f"{self.name}({self.args}) -> {self.return_args}"


def is_const(arg):
    try:
        return arg[1].type_quals[0][0] == 'const'
    except AttributeError:
        pass
    except IndexError:
        pass
    return False


def is_array(arg):
    try:
        return arg[1][1][0] == -1
    except AttributeError:
        pass
    except IndexError:
        pass
    return False

def get_enum(t:CProxyType,name=None):
    if t.is_function() or t.is_function_type():
        return None,None
    lookup = [a for a in all_known_types if a[0] == t.get_name()]
    if len(lookup) == 1:
        tl = lookup[0][1]
        if isinstance(tl, c_parser.Type) and tl[0]=='enum':
            return tl[1],"enum_"+tl[1]

        if tl[0].startswith("enum "):
            type_name_ = tl[0].split(" ")[1]
            type_name = type_name_[0].upper() + type_name_[1:]
            return type_name,type_name_
    return None,None

def calculate_arguments(known_return_args, given_args):
    return_args = [known_return_args]
    args = []
    for arg in given_args:
        if not is_const(arg) and is_array(arg):
            return_args.append(arg)
        else:
            args.append(arg)

    return return_args, args


def create_argument(param):
    try:
        if isinstance(param, str):
            # its a string
            # type, d = resolve_type1(all_known_types, param)
            return Argument(None, CProxyType(param))

        else:
            if len(param) == 0:
                return Argument(None, CProxyType(param))
            else:

                if len(param) == 3:
                    name = param[0]
                    t = param[1]

                    return Argument(name, CProxyType(t))

    except Exception as e:
        print(f"Error: {e}")


functions = []

if 'types' in parser.defs:
    all_known_types = parser.defs['types'].items()
    for typename, typedef in all_known_types:
        if typename.startswith("fmi2") and typedef.type_quals == ((), ()) and isinstance(typedef.declarators[0], tuple):
            name = typename.replace("TYPE", "").replace("fmi2", "")
            print(f"Type name: {typename}")
            print(f"Definition: {typedef}")

            args_in = []
            args_out = []
            args_out.append(create_argument(typedef[0]))

            for arg in typedef[1]:
                t=CProxyType(arg[1])
                if not is_const(arg) and (t.is_prt() or t.is_array()):
                    args_out.append(create_argument(arg))
                else:
                    args_in.append(create_argument(arg))

            functions.append(Function(name, args_out, args_in, (typename.replace("TYPE", ""), typedef)))


else:
    print("No type declarations found.")

rpc_recipie = """
// This is a service for FMI2.0.4
syntax = "proto3";

import "google/protobuf/empty.proto";

option optimize_for = SPEED;

service Fmi2Service {

"""

rpc_messages = {}

# find the common return type
common_returns = [f.return_args[0] for f in functions if len(f.return_args) == 1]
most_common_return = max(common_returns,
                         key=lambda x: sum(p.arg_type.get_name() == x.arg_type.get_name() for p in common_returns))
most_common_return_msg = most_common_return.arg_type.get_name()[0].upper() + most_common_return.arg_type.get_name()[
                                                                             1:] + "Response"


def mk_message(name, args):
    msg = f"message {name} " + "{\n"
    for idx, a in enumerate(args):
        msg += f"\t  {map2protobuf_type(a.arg_type)} {a.name if a.name else 'RET'} = {idx + 1};\n"
    msg += '}\n'
    return msg


rpc_messages[most_common_return_msg] = mk_message(most_common_return_msg, [most_common_return])


def is_index_arg(name, arg):
    for n in indexing.keys():
        if name in n:
            for idx in indexing[n]:
                if arg.name and arg.name == idx['index']:
                    print("\t Ignoring index: "+arg.name)
                    return True
    return False


def filter_messages(name, args):
    return [a for a in args if not a.arg_type.is_function() and not is_index_arg(name, a)]

def use_common_response(f):
    if len(f.return_args) == 1 and f.return_args[0].arg_type.get_name() == most_common_return.arg_type.get_name():
        return most_common_return_msg
    return f.name+ "Response"

for f in functions:
    print(f.name)

    request_name = f.name + "Request"
    response_name = use_common_response(f)

    rpc_recipie += "   rpc " + f.name + "(" + request_name + ") returns (" + response_name + ") {}\n"

    if not response_name == most_common_return_msg:
        rpc_messages[response_name] = mk_message(response_name, filter_messages(f.name, f.return_args))

    rpc_messages[request_name] = mk_message(request_name, filter_messages(f.name, f.args))

    print('\tout')
    for a in f.return_args:
        name = a.name if a.name else 'RET'
        print('\t\t' + name + ' : ' + map2protobuf_type(a.arg_type))

    print('\tin')
    for a in f.args:
        name = a.name if a.name else 'RET'
        print('\t\t' + name + ' : ' + map2protobuf_type(a.arg_type))

rpc_recipie += "}"

rpc_recipie += "\n\n"
rpc_recipie += "\n\n".join([v for v in rpc_messages.values() if v])
rpc_recipie += "\n\n"
if 'enums' in parser.defs:

    for enum in parser.defs['enums']:
        rpc_recipie += f"enum {enum[0].upper() + enum[1:]}"
        rpc_recipie += " {\n"
        for idx, v in enumerate(parser.defs['enums'][enum]):
            rpc_recipie += f"  g{v} = {idx};\n"
        rpc_recipie += "}\n"
        rpc_recipie += "\n\n"

with open("work2/proto/fmi2.proto", "w") as file:
    file.write(rpc_recipie)


with open("work2/grpc_fmi_enums.cxx", "w") as file:
    # file.write('#include "fmi2.grpc.pb.h"\n')
    # file.write('#include "fmi2Functions.h"\n')
    file.write(header)
    file.write('#include "grpc_fmi_enums.h"\n')

    signatures=[]

    if 'enums' in parser.defs:

        k = [(t,get_enum(CProxyType(parser.defs['types'][t]),name=t)) for t in parser.defs['types'] ]
        k = [t for t in k if t[1][0]]

        for enum in k:
            fmi_name = enum[0]
            enum_name=enum[1][0]
            rpc_name = enum[1][0]
            rpc_name = rpc_name[0].upper() + rpc_name[1:]

            signature =f"{fmi_name} {rpc_name}To{fmi_name}({rpc_name} status)"
            signatures.append(signature)
            file.write(f" {signature} {{\n")
            file.write(f"   switch(status) {{\n")
            for idx, v in enumerate(parser.defs['enums'][enum_name]):
                file.write(f"     case {rpc_name}::g{v} : return {v};\n")
            file.write(f"     default: throw FmiEnumNotFoundException(\"Could not fund the enum\");\n")
            file.write(f"   }}\n")
            file.write(f"}}\n\n")

            signature=f"{rpc_name} {fmi_name}To{rpc_name}({fmi_name} status)"
            signatures.append(signature)
            file.write(f"{signature} {{\n")
            file.write(f"   switch(status) {{\n")
            for idx, v in enumerate(parser.defs['enums'][enum_name]):
                file.write(f"     case {v} : return {rpc_name}::g{v};\n")
            file.write(f"     default: throw FmiEnumNotFoundException(\"Could not fund the enum\");\n")
            file.write(f"   }}\n")
            file.write(f"}}\n\n")

            if fmi_name=='fmi2Status':
                signatures.append(f"#define gStatus2fmi2Status {rpc_name}To{fmi_name}\n")

    fmiFunctions = "enum class FmiFunctionNames : char{\n"
    for f in functions:
        fmiFunctions+="\t"+f.name+",\n"
    fmiFunctions+="}\n"
    signatures.append(fmiFunctions)
    fmiFunctions = "extern const char* FmiFunctionNamesStr[];\n"
    signatures.append(fmiFunctions)
    signatures.append("#define FMI_FUNCTION_COUNT "+str(len(functions))+"\n")

    fmiFunctions = "const char* FmiFunctionNamesStr[] ={\n"
    for f in functions:
        fmiFunctions += "\t\"" + f.name + "\",\n"
    fmiFunctions += "};\n"
    file.write(fmiFunctions)

    with open("work2/grpc_fmi_enums.h", "w") as file_h:
        file_h.write(header)
        file_h.write("#ifndef GRPC_FMI_ENUMS\n")
        file_h.write("#define GRPC_FMI_ENUMS\n")
        file_h.write('#include "fmi2.grpc.pb.h"\n')
        file_h.write('#include "fmi2Functions.h"\n')
        file_h.write('#include "grpc_fmi_enums.h"\n')

        file_h.write('''class FmiEnumNotFoundException : public std::runtime_error {
public:
    FmiEnumNotFoundException(const std::string& msg) : std::runtime_error(msg) {}
};

''')

        file_h.write(";\n".join(signatures)+';\n')
        file_h.write("#endif\n")

with open("work2/client/client_fmi.cxx", "w") as file:
    file.write(header)
    file.write('#include "fmi2.grpc.pb.h"\n')
    file.write('#include "client_fmi.h"\n')
    file.write('#include "grpc_fmi_enums.h"\n')
    file.write("""#ifndef FMI_REMOTE_RECORD_EXEC_START
// must return a token for use in _END
#define FMI_REMOTE_RECORD_EXEC_START(name) 0
#endif

#ifndef FMI_REMOTE_RECORD_EXEC_END
#define FMI_REMOTE_RECORD_EXEC_END(name,start)
#endif\n""")
    # file.write('extern  std::unique_ptr<Fmi2Service::Stub> stub_;\n')
    # file.write('using grpc::ClientContext;\n\n')

    for f in functions:
        print(f.name)
        definition ='extern "C" '+ f.definition[1][0] + ' ' + f.definition[0] + '('
        for a in f.definition[1][1]:
            n = a[0]
            t = CProxyType(a[1])
            #Type('fmi2String', [-1], type_quals=(('const',), ()))
            prefix = ''
            if hasattr(a[1],'type_quals'):
                prefix=  ' '.join([tq[0] for tq in a[1].type_quals if len(tq)>0])+' '

            definition +=prefix+ t.get_name() + ' ' + ('*' if t.is_function() else '')  + ' ' + n+ (
                '[]' if t.is_array() else '') + ', '
        if definition[-2:] == ', ':
            definition = definition[:-2]
        definition += ') {\n'

        body="\tClientContext context;\n"
        body += f"\tauto start_record = FMI_REMOTE_RECORD_EXEC_START({f.name}) ;\n"
        body += f"\tauto request = {f.name}Request();\n"
        body += f"\tauto response = { use_common_response(f)}();\n"
        body+="\n\t//argument handling\n"

        handled_args = []
        f_indexed = indexing['fmi2'+f.name] if 'fmi2'+f.name in indexing else None

        has_status_return = any([a.name==None and a.arg_type.get_name()=='fmi2Status' for a in f.return_args])

        for a in f.args:
            if a in handled_args:
                continue
            if f_indexed:
                continue_next = False
                for idx in f_indexed:
                    if a.name and (a.name == idx['index'] or a.name in idx['args']):
                        if a.name == idx['index']:
                            handled_args.append(a)
                            continue_next=True
                            break
                        body+=f"\tfor ( int i = 0; i < {idx['index']}; i++) {{\n"

                        body += f"\t\trequest.add_{str(a.name).lower()}({a.name}[i]);\n"
                        body += f"\t}}\n"
                        handled_args.append(a)
                        # handled_args.append(idx['index'])
                        # for v in idx['args']:
                        #     handled_args.append(v)
                        continue_next=True
                        break
                if continue_next:
                    continue

            converter_function = ""
            converter_function_end = ""
            if a.arg_type.get_name() in fmi2_to_proto_type:
                if fmi2_to_proto_type[a.arg_type.get_name()]=='string':
                    converter_function=f"std::string("
                    converter_function_end=")"
                else:
                    converter_function = f"reinterpret_cast<::{fmi2_to_proto_type[a.arg_type.get_name()]}_t>("
                    converter_function_end=")"


            if a.arg_type.get_name() in ["fmi2Component" ,"fmi2FMUstate" ]:
                converter_function = "static_cast<uint32_t>(reinterpret_cast<std::uintptr_t>("
                converter_function_end = "))"


            enum_name,enum_name_original = get_enum(a.arg_type)
            if enum_name:
                pass



            body += f"\trequest.set_{str(a.name).lower()}({converter_function}{a.name}{converter_function_end});\n"
            handled_args.append(a)

        # and the call
        body+=f"\tauto status = stub_->{f.name}( &context, request, &response);\n"
        body+="\tif (status.ok())\n"
        body+="\t{\n"

        for a in f.return_args:
            if a.name:
                # this will not be status
                if a in handled_args:
                    continue
                if f_indexed:
                    continue_next = False
                    for idx in f_indexed:
                        if a.name and (a.name == idx['index'] or a.name in idx['args']):
                            if a.name == idx['index']:
                                handled_args.append(a)
                                continue_next = True
                                break
                            body += f"\t\tfor ( int i = 0; i < {idx['index']} && i < response.{a.name.lower()}_size(); i++) {{\n"

                            body += f"\t\t\t{a.name}[i] =response.{str(a.name).lower()}(i);\n"
                            body += f"\t\t}}\n"
                            handled_args.append(a)
                            # handled_args.append(idx['index'])
                            # for v in idx['args']:
                            #     handled_args.append(v)
                            continue_next = True
                            break
                    if continue_next:
                        continue

                converter_function = ""
                converter_function_end = ""
                if a.arg_type.get_name() in fmi2_to_proto_type:
                    if fmi2_to_proto_type[a.arg_type.get_name()] == 'string':
                        converter_function = f"std::string("
                        converter_function_end = ")"
                    else:
                        converter_function = f"reinterpret_cast<::{fmi2_to_proto_type[a.arg_type.get_name()]}_t>("
                        converter_function_end = ")"

                if a.arg_type.get_name() in ["fmi2Component", "fmi2FMUstate"]:
                    converter_function = "static_cast<uint32_t>(reinterpret_cast<std::uintptr_t>("
                    converter_function_end = "))"

                enum_name, enum_name_original = get_enum(a.arg_type)
                if enum_name:
                    pass

                body += f"\trequest.set_{str(a.name).lower()}({converter_function}{a.name}{converter_function_end});\n"
                handled_args.append(a)

        if has_status_return:
            body+=f"\t\tFMI_REMOTE_RECORD_EXEC_END({f.name}, start_record);\n"
            body+="\t\treturn gStatus2fmi2Status(response.ret());\n"

        body+="\t}\n"
        body+="\telse\n"
        body+="\t{\n"

        body+='\t\tg_component_env_map.at(c).callback.logger(nullptr, g_component_env_map[c].name.c_str(), fmi2Fatal, "fmi2Error", "Error %d - %s", status.error_code(),status.error_message().c_str());\n'
        # body+="\t\tstd::cerr << status.error_code() << \": \" << status.error_message() << std::endl;\n"
        if has_status_return:
            body += f"\t\tFMI_REMOTE_RECORD_EXEC_END({f.name}, start_record);\n"
            body+="\t\treturn fmi2Fatal;\n"
        body+="\t}\n"
    #     if (status.ok())
    #         {
    #         for (int i =0;i < response.value_size() & & i < nvr;i++) {
    #         value[i] = response.value(i);
    #         }
    #     return gStatus2fmi2Status(response.ret());
    #
    #     } else {
    #     std::cerr << status.error_code() << ": " << status.error_message() << std::endl;
    #     return fmi2Fatal;
    # }

        definition+=body

        definition += '}\n\n'

        definition=f"#ifndef CUSTOM_{f.name}\n//Define CUSTOM_{f.name} to manually specify an implementation\n"+definition+"#endif\n\n"
        file.write(definition)

with open("work2/server/Fmi2ServiceImpl.h", "w") as file:
    file.write(header)
    file.write('#include "fmi2.grpc.pb.h"\n')
    file.write('#include "fmi2Functions.h"\n')
    file.write("""
#ifndef FMI2SERVICEIMPL_H
#define FMI2SERVICEIMPL_H
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

#include "SimFmi2.h"
#include "grpc_fmi_enums.h"

struct FmiExecInfo{
    int count;
    std::chrono::duration<double> duration;
    
    FmiExecInfo() : duration(std::chrono::duration<double>::zero()), count(0) {}
};

// Service implementation
class Fmi2ServiceImpl final : public Fmi2Service::Service {

public:
	explicit Fmi2ServiceImpl(std::map<std::string, std::shared_ptr<Fmi2Impl> > available_fmus):available_fmus(std::move(available_fmus)){};


""")

    for f in functions:
        print(f.name)
        definition = f.definition[1][0] + ' ' + f.definition[0] + '('
        for a in f.definition[1][1]:
            n = a[0]
            t = CProxyType(a[1])
            definition += t.get_name() + ' ' + ('*' if t.is_function() else '') + (
                '*' if t.is_array() else '') + ' ' + n + ', '
        if definition[-2:] == ', ':
            definition = definition[:-2]
        definition += ') {\n'

        body="\tClientContext context;\n"
        body += f"\tauto request = {f.name}Request();\n"
        body += f"\tauto response = { use_common_response(f)}();\n"
        body+="\n\t//argument handling\n"
        file.write(f"\tStatus {f.name}(ServerContext* context, const {f.name}Request* request, {use_common_response(f)}* reply) override;\n")

    file.write("private:\n")
    file.write("\tstd::map<std::string, std::shared_ptr<Fmi2Impl> > available_fmus;\n")
    file.write("\tstd::vector<std::shared_ptr<Fmi2Comp> > fmu_instances;\n")
    file.write("\tstd::shared_ptr<Fmi2Comp> getComponent(int index); \n")
    file.write("\tFmiExecInfo fmi_function_executions[FMI_FUNCTION_COUNT];\n")
    file.write("\tvirtual std::chrono::time_point<std::chrono::high_resolution_clock> record_exec_start(FmiFunctionNames name)\n\t{\n\t\tthis->fmi_function_executions[static_cast<int>(name)].count++;\n\t\treturn std::chrono::high_resolution_clock::now();\n\t};\n")
    file.write(
        "\tvirtual void record_exec_end(FmiFunctionNames name,std::chrono::time_point<std::chrono::high_resolution_clock> start_point)\n\t{\n\t\tthis->fmi_function_executions[static_cast<int>(name)].duration+=(std::chrono::high_resolution_clock::now()-start_point);\n\t};\n")

    file.write("""\n};\n\n#endif //FMI2SERVICEIMPL_H""")


with open("work2/server/Fmi2ServiceImpl.cpp", "w") as file:
    file.write(header)
    file.write('#include "Fmi2ServiceImpl.h"\n')
    file.write('#include "fmi2Functions.h"\n')
    file.write("""
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using namespace std;
#ifndef FMI_REMOTE_RECORD_EXEC_START
// must return a token for use in _END
#define FMI_REMOTE_RECORD_EXEC_START(name) 0
#endif

#ifndef FMI_REMOTE_RECORD_EXEC_END
#define FMI_REMOTE_RECORD_EXEC_END(name,start)
#endif

shared_ptr<Fmi2Comp> Fmi2ServiceImpl::getComponent(int index) {
    auto array_index = index -1;
    if (array_index >= 0 and array_index < this->fmu_instances.size()) {
        return this->fmu_instances.at(array_index);
    }
    return nullptr;
}

\n""")
    for f in functions:
        print(f.name)
        definition = f.definition[1][0] + ' ' + f.definition[0] + '('
        for a in f.definition[1][1]:
            n = a[0]
            t = CProxyType(a[1])
            definition += t.get_name() + ' ' + ('*' if t.is_function() else '') + (
                '*' if t.is_array() else '') + ' ' + n + ', '
        if definition[-2:] == ', ':
            definition = definition[:-2]
        definition += ') {\n'

        body="\tClientContext context;\n"
        body += f"\tauto request = {f.name}Request();\n"
        body += f"\tauto response = { use_common_response(f)}();\n"
        body+="\n\t//argument handling\n"
        definition=(f"Status Fmi2ServiceImpl::{f.name}(ServerContext* context, const {f.name}Request* request, {use_common_response(f)}* reply) \n")
        if not any([n == f.name for n in ["GetReal","GetInteger","GetBoolean","GetString","SetInteger","SetReal","SetBoolean","SetString"]]):
            definition += ("{\n")
            definition += """\tauto start_record = FMI_REMOTE_RECORD_EXEC_START("""+f.name+""")
    
    auto component = getComponent(request->c());
    if (component == nullptr) {
            reply->set_ret(fmi2StatusToAnon_enum0(fmi2Error));
            FMI_REMOTE_RECORD_EXEC_END("""+f.name+""",start_record);
            return Status::OK;
    }
                        \n"""
            if len(f.args) == 1:
                definition += f"\treply->set_ret(fmi2StatusToAnon_enum0(component->fmu->{f.name[0].lower() + f.name[1:]}(component->comp)));\n"
            else:

                arguments = "," + ",".join(["request->"+a.name.lower()+"()" for a in f.args[1:]]) if len(f.args) > 1 else ""
                definition += f"\treply->set_ret(fmi2StatusToAnon_enum0(component->fmu->{f.name[0].lower() + f.name[1:]}(component->comp"+arguments+")));\n"
            definition+="""FMI_REMOTE_RECORD_EXEC_END("""+f.name+""",start_record);"""
            definition+=("\n\treturn Status::OK;\n")
        else:
            definition += ("{\n")
            definition+="""
    auto component = getComponent(request->c());
    if (component == nullptr) {
        reply->set_ret(fmi2StatusToAnon_enum0(fmi2Error));
        return Status::OK;
    }
            \n"""
            if any([n == f.name for n in ["SetReal","SetInteger","SetBoolean"]]):
                definition+=f"""    auto vr_size = request->vr().size();
    fmi2ValueReference vr[vr_size];
    for (int i = 0; i < vr_size; i++) {{
        vr[i] = static_cast<fmi2ValueReference>(request->vr()[i]);
    }}
    auto value_size = request->value().size();
    fmi2{f.name[3:]} values[value_size];
    for (int i = 0; i < value_size; i++) {{
        values[i] = request->value()[i];
    }}
    auto status = component->fmu->{f.name[0].lower()+f.name[1:]}(component->comp,vr,vr_size,values);
    reply->set_ret(fmi2StatusToAnon_enum0(status));
    return Status::OK;\n"""


            elif any([n == f.name for n in ["GetReal","GetInteger","GetBoolean"]]):
                definition+=f"""    auto vr_size = request->vr().size();
    fmi2ValueReference vr[vr_size];
    for (int i = 0; i < vr_size; i++) {{
        vr[i] = static_cast<fmi2ValueReference>(request->vr()[i]);
    }}
    auto value_size =vr_size;
    fmi2{f.name[3:]} values[value_size];

    auto status = component->fmu->{f.name[0].lower()+f.name[1:]}(component->comp,vr,vr_size,values);
    for (int i = 0; i < value_size; i++) {{
        reply->add_value(values[i]);
    }}
    reply->set_ret(fmi2StatusToAnon_enum0(status));
    return Status::OK;\n"""


        definition+=("}\n\n\n")

        definition = f"#ifndef CUSTOM_{f.name}\n//Define CUSTOM_{f.name} to manually specify an implementation\n" + definition + "#endif\n\n"
        file.write(definition)



with open("work2/transport/Fmi2ZmqClientTransport.h", "w") as file:
    file.write(header)
    file.write('#include <zmq.hpp>\n')
    file.write('#include "fmi2.grpc.pb.h"\n\n')
    file.write("""class Fmi2ZmqClientTransport{
    public: 
        explicit Fmi2ZmqClientTransport(zmq::socket_t* socket): socket(socket){};
        
    """)

    for f in functions:
        request_name = f.name + "Request"
        response_name = use_common_response(f)

#        rpc_recipie += "   rpc " + f.name + "(" + request_name + ") returns (" + response_name + ") {}\n"

        file.write(f"\n\t\t::grpc::Status {f.name}(::grpc::ClientContext* context, const ::{request_name}& request, ::{response_name}* response){{ return communicate(FmiFunctionNames::{f.name},request, response);}};")
    file.write("""
    
    private:
        zmq::socket_t* socket;
        
        template <typename T1, typename T2>
        ::grpc::Status communicate(FmiFunctionNames type, T1 request, T2 response) {
            // Function body
          //  google::protobuf::Any any_msg;
          //  any_msg.PackFrom(request);
        
            std::string serialized_data;
            request.SerializeToString(&serialized_data);
        
           	zmq::message_t zmq_msg(serialized_data.size()+sizeof(char));
		    memcpy(zmq_msg.data(), &type, sizeof(char));
		    memcpy(static_cast<char*>(zmq_msg.data()) + sizeof(char), serialized_data.data(), serialized_data.size());
		    socket->send(zmq_msg, zmq::send_flags::none);
        
            zmq::message_t zmq_response;
        
            if(!socket->recv(zmq_response, zmq::recv_flags::none)) {
                return ::grpc::Status(::grpc::StatusCode::INTERNAL, "recv failed");
            }
            response->ParseFromString(std::string(static_cast<char*>(zmq_response.data()), zmq_response.size()));
            return grpc::Status::OK;
        }
    };""")


with open("work2/transport/Fmi2ZmqServerTransport.h", "w") as file:
    file.write(header)
    file.write('#include <zmq.hpp>\n')
    file.write('#include "grpc_fmi_enums.h"\n')
    file.write('#include "fmi2.grpc.pb.h"\n\n')
    file.write("""class Fmi2ZmqServerTransport{
    public: 
         
        Fmi2ZmqServerTransport(const std::string& addr_uri): url(addr_uri), running(false),socket(nullptr) {};
        void Start();
        void RegisterService(Fmi2Service::Service* service){this->service = service;};
    private:
        void dispatch(FmiFunctionNames type, std::string msg);
        std::string url;
        bool running;
        Fmi2Service::Service* service;
        zmq::socket_t* socket;
    };
    """)

with open("work2/transport/Fmi2ZmqServerTransport.cpp", "w") as file:
    file.write(header)

    file.write('#include "Fmi2ZmqServerTransport.h"\n\n')

    file.write("""void Fmi2ZmqServerTransport::Start() {

 // Create ZMQ Context
    zmq::context_t context ( 1 );
    // Create the Subscribe socket
    zmq::socket_t socket ( context, zmq::socket_type::rep );
    this->socket = &socket;
    // Connect to a tcp socket
    socket.bind( url.c_str() );
    // Set the socket option to subscribe
   // socket.setsockopt( ZMQ_SUBSCRIBE, "", 0 );
    
    running = true;
    
    google::protobuf::Any received_any_msg;
    std::cout << "Listening on " <<this->url << std::endl;
    
    while(running){
        zmq::message_t update;
         // Receive the message and convert to string
        if (socket.recv(update, zmq::recv_flags::none)) {

            char msg_type =*static_cast<char*>(update.data());

            dispatch(static_cast<FmiFunctionNames>(msg_type),std::string(static_cast<char*>(update.data()) + sizeof(char), update.size()-+sizeof(char)));
           
        }
    }
}
    
    
void Fmi2ZmqServerTransport::dispatch(FmiFunctionNames type, std::string msg){
    //auto msg_type = msg.type_url();
    //lets try to match the messages  
    switch(type)
    {  
""")
    for f in functions:
        request_name = f.name + "Request"
        response_name = use_common_response(f)

        #        rpc_recipie += "   rpc " + f.name + "(" + request_name + ") returns (" + response_name + ") {}\n"

        file.write(
            f"\n\t case FmiFunctionNames::{f.name}:")
        file.write(f"""{{
            {request_name} request_msg;
            request_msg.ParseFromString(msg);
           // msg.UnpackTo(&request_msg);
            {response_name} response_msg;
            service->{f.name}(nullptr, &request_msg, &response_msg);
            
            std::string serialized_data;
            response_msg.SerializeToString(&serialized_data);
            zmq::message_t reply(serialized_data.size());
            memcpy(reply.data(), serialized_data.data(), serialized_data.size());
            socket->send(reply, zmq::send_flags::none);
            return;
        }}
""")
    file.write("\n\t}\n}\n")