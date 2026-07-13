# Mini-Exchange-Simulator





CMD:

cmake -S . -B build -DCMAKE\_BUILD\_TYPE=Debug

cmake --build build --target common



// ------------------- In Docker: ------------------- //

docker build -f docker/base.Dockerfile -t mini-exchange-base .



Run a shell inside it:

docker run --rm -it mini-exchange-base bash



Verify build atifacts:

ls /src/build

ls /src/build/common



// --------------------------------------------------- //

