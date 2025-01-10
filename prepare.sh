python3 -m venv .venv
source .venv/bin/activate

protobufDir=$(pwd)/protobuf-install

if [ ! -d "$protobufDir=" ]; then

git clone --recurse-submodules https://github.com/protocolbuffers/protobuf.git
cd protobuf
git checkout v24.4
git submodule update --init --recursive

cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$(pwd)/protobuf-install \
         -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_BUILD_EXAMPLES=OFF

make -j$(nproc)
make install
fi

echo "Generating the source files from the fmi headers"
python main2.py


cd work2
cmake .
make -j$(nproc)

