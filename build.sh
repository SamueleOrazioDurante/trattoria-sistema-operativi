#!/bin/bash
cd build
cmake ..
make

if [ "$1" == "--verify" ]; then
    echo "=== RUNNING VERIFY ==="
    ./trattoria_client --strategy profit | grep -e "Metriche" -e "completata"
else
    echo "=== RUNNING PROFIT ==="
    ./trattoria_client --strategy profit | grep "Metriche"

    echo "=== RUNNING REPUTATION ==="
    ./trattoria_client --strategy reputation | grep "Metriche"
fi