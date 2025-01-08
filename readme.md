
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