#/bin/bash
cd build
cmake ..
make
./trattoria_client --strategy profit --print-blackboard