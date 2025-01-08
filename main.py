from abc import ABC

from pyclibrary import CParser
import glob

header_files = glob.glob("FMI-Standard-2.0.4/**/*.h")

parser = CParser(header_files, cache="fmi_cache")

rpc_recipie = """
// This is a service for FMI2.0.4
syntax = "proto3";

import "google/protobuf/empty.proto";

service Fmi2Service {

"""

rpc_messages = {}

fmi2_to_proto_type = {'fmi2String': 'string', 'fmi2Component': 'uint32', 'fmi2FMUstate': 'uint32'}

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


class Type(ABC):
    pass

    def is_array(self):
        return False

    def is_prt(self):
        return False


class BasicType:
    def __init__(self, name, is_array, is_ptr):
        self.name = name
        self.is_array = is_array
        self.is_ptr = is_ptr

    def is_array(self):
        return self.is_array

    def is_ptr(self):
        return self.is_ptr


class Argument:
    def __init__(self, name: str, arg_type: Type):
        self.name = name
        self.arg_type = arg_type

    def __str__(self):
        return f"{self.name}: {self.arg_type}"


class Function:
    def __init__(self, name: str, return_args: [Argument], args: [Argument]):
        self.name = name
        self.return_args = return_args
        self.args = args

    def __str__(self):
        return f"{self.name}({self.args}) -> {self.return_args}"


def resolve_type(all_known_types, type):
    if isinstance(type, str):
        name = type
    else:
        name = type[0]

    if name in fmi2_to_proto_type:
        type_name = fmi2_to_proto_type[name]
        return type_name, type

    rt = [a for a in all_known_types if a[0] == name]
    if len(rt) > 0:
        type = rt[0][1]

        if name in fmi2_to_proto_type:
            type_name = fmi2_to_proto_type[name]
        elif type[0].startswith("enum "):
            type_name = type[0].split(" ")[1]
            type_name = type_name[0].upper() + type_name[1:]
            return type_name, type
        else:
            type_name = type[0]

        if type_name in fmi2_to_proto_type:
            type_name = fmi2_to_proto_type[type_name]
        if type_name in cpp_to_protobuf:
            type_name = cpp_to_protobuf[type_name]

        if type_name not in proto_types:
            return resolve_type(all_known_types, type_name)
        return type_name, type
    else:
        type_name = name
        if type_name in fmi2_to_proto_type:
            type_name = fmi2_to_proto_type[type_name]
        if type_name in cpp_to_protobuf:
            type_name = cpp_to_protobuf[type_name]
        return type_name, None


def mk_request_message(all_known_types, response_name, param):
    message = "message " + response_name + " {\n"
    try:
        if isinstance(param, str):
            # its a string
            type, d = resolve_type(all_known_types, param)
            message += ("  " + type + " value=1;\n")

        else:
            if len(param) == 0:
                return "google.protobuf.Empty", None
            else:

                for idx, f in enumerate(param):
                    if isinstance(f, str) and f == 'fmi2Status':
                        # its a string
                        type, d = resolve_type(all_known_types, f)
                        message += ("  " + type + " value=" + str(idx) + ";\n")
                        continue

                    name = f[0]
                    type = f[1]

                    type, d = resolve_type(all_known_types, type)
                    # lets resolve the type
                    print(type)
                    if type == 'message':
                        message += "//"
                    prefix = ""
                    try:
                        if d[1] == [-1]:
                            prefix = "repeated "
                        else:
                            pass
                    except Exception as e:
                        pass

                    # type = fmi2_to_proto_type[type]

                    message += ("  " + prefix + type + " " + name + "=" + str(idx + 1) + ";\n")
    except Exception as e:
        print(f"Error: {e}")
    message += "}"
    return response_name, message


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


def calculate_arguments(known_return_args, given_args):
    return_args = [known_return_args]
    args = []
    for arg in given_args:
        if not is_const(arg) and is_array(arg):
            return_args.append(arg)
        else:
            args.append(arg)

    return return_args, args


if 'types' in parser.defs:
    for typename, typedef in parser.defs['types'].items():
        if typename.startswith("fmi2") and typedef.type_quals == ((), ()) and isinstance(typedef.declarators[0], tuple):
            name = typename.replace("TYPE", "").replace("fmi2", "")
            print(f"Type name: {typename}")
            print(f"Definition: {typedef}")

            ret_type = name + "Response"

            # if typedef[0] == 'fmi2Status':
            #     ret_type = "StatusResponse"

            request_name = name + "Request"
            response_name = ret_type

            returnArgs, args = calculate_arguments(typedef[0], typedef[1])

            if request_name not in rpc_messages:
                request_name, rpc_messages[request_name] = mk_request_message(parser.defs['types'].items(),
                                                                              request_name, args)
            if response_name not in rpc_messages:
                response_name, rpc_messages[response_name] = mk_request_message(parser.defs['types'].items(),
                                                                                response_name, returnArgs)

            rpc_recipie += "   rpc " + name + "(" + name + "Request) returns (" + ret_type + ") {}\n"
else:
    print("No type declarations found.")

rpc_recipie += "}"

rpc_recipie += "\n\n"
rpc_recipie += "\n\n".join([v for v in rpc_messages.values() if v])
rpc_recipie += "\n\n"
if 'enums' in parser.defs:

    for enum in parser.defs['enums']:
        rpc_recipie += f"enum {enum[0].upper() + enum[1:]}"
        rpc_recipie += " {\n"
        for idx, v in enumerate(parser.defs['enums'][enum]):
            rpc_recipie += f"  {v} = {idx};\n"
        rpc_recipie += "}\n"
        rpc_recipie += "\n\n"

with open("fmi2.proto", "w") as file:
    file.write(rpc_recipie)

# print(parser)
