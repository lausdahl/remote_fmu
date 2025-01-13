# Remote FMU

This is a small tool that allow remote fmu execution i.e. it generates a proxy fmu from an ordinary fmu which forwards all fmi communication to a remote server.

The remote server can provide access to multiple fmus.

# Usage

Download the bundle

- `server_zmq` the server application which binds to a network interface
- `remote_fmu.zip` a bundle to be used to make client proxy fmus. It must be unzipped and contains the following:
  - `remote_fmu`
    - `binaries` a folder with the native code providing the proxy behavior
    - `create_remote_fmu.py` script to help generate the proxies


## Starting the server

```bash
# This will start the server and allow clients access to the specified fmu
./server_zmq --port "tcp://localhost:50051" --path "watertankcontroller-c.fmu" --guid "{8c4e810f-3df3-4a00-8276-176fa3c9f000}"
```

## Generating proxies

```bash
mkdir proxies
# This will make a proxy for the specified fmu which is identical in terms of fmi
python3 create_remote_fmu.py --connection_string "tcp://localhost:50051" --destination proxies watertankcontroller-c.fmu
```

## Simulation
Make sure the original fmu


## gRPS
We need to compile from source to make sure we have a consistent installation and can do static linking as required for FMI
```bash
# This take a long time to checkout and compile
git clone --recurse-submodules -b v1.67.0 https://github.com/grpc/grpc
cmake -DCMAKE_INSTALL_PREFIX=grpc-install -DgRPC_BUILD_TESTS=OFF -DgRPC_BUILD_EXAMPLES=OFF -DgRPC_USE_PRECOMPILED_HEADERS=ON -DCMAKE_BUILD_TYPE=Release -Bgrpc-build -Sgrpc
cd grpc
make -j9
make install
```


```bash

apt install unzip  nano build-essential cmake git zlib1g-dev python3 bzip2  lbzip2 zlib1g-dev libbz2-dev libssl-dev liblzma-dev libzstd-dev libssl-dev python3.12-venv libmbedtls-dev pkg-config -y
```