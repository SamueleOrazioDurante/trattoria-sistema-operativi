#!/bin/bash
cd build
cmake ..
make

echo "=== RUNNING PROFIT ==="
./trattoria_client --strategy profit | grep "Metriche"

echo "=== RUNNING REPUTATION ==="
./trattoria_client --strategy reputation | grep "Metriche"