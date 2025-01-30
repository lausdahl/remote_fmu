mkdir -p testing
cd testing

rm *.json*
rm *.fmu*
rm *.jar*

echo Copying Remote FMU components

cp ../client/remote_fmu.zip .
unzip -o  remote_fmu.zip -d remote_fmu
cp ../server/server_zmq .


echo Downloading Maestro and test models

wget -q  -O maestro.jar https://github.com/INTO-CPS-Association/maestro/releases/download/Release%2F3.0.1/maestro-3.0.1-jar-with-dependencies.jar 

wget -q https://raw.githubusercontent.com/INTO-CPS-Association/maestro/refs/heads/development/maestro/src/test/resources/specifications/full/initialize_singleWaterTank/env.json 
sed -i.bak  's|src/test/resources/||g' env.json
wget -q https://raw.githubusercontent.com/INTO-CPS-Association/maestro/refs/heads/development/maestro/src/test/resources/specifications/full/initialize_singleWaterTank/config.json 

wget -q https://github.com/INTO-CPS-Association/maestro/raw/refs/heads/development/maestro/src/test/resources/watertankcontroller-c.fmu 
wget -q https://github.com/INTO-CPS-Association/maestro/raw/refs/heads/development/maestro/src/test/resources/singlewatertank-20sim.fmu 

echo Generating proxy fmu
mkdir -p proxies
python3 create_remote_fmu.py --connection_string "tcp://localhost:50051" --destination proxies watertankcontroller-c.fmu

echo Replacing original fmu with proxy
cat env.json | sed   's|watertankcontroller-c.fmu|proxies/watertankcontroller-c.fmu|g' > proxies.json

echo Starting proxy server
./server_zmq  --path "watertankcontroller-c.fmu" --guid "{8c4e810f-3df3-4a00-8276-176fa3c9f000}" > server.log  2>&1 &
bg_pid=$!

java -jar maestro.jar import sg1 proxies.json config.json -output simulation --interpret

wait $!

kill $bg_pid
