python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt

protobufDir="$(pwd)/protobuf-install"

if [ ! -d "$protobufDir	" ]; then

git clone --recurse-submodules https://github.com/protocolbuffers/protobuf.git
cd protobuf
git checkout v29.3
git submodule update --init --recursive

cmake . -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$protobufDir \
         -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_BUILD_EXAMPLES=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON

make -j$(nproc)
make install
cd ..
fi

echo "Generating the source files from the fmi headers"
python generate.py


cd src
cmake .
make -j$(nproc)

