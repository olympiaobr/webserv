#!/bin/bash

SERVER_URL=${1:-"http://localhost:8080"}

for test in individual_tests/*.py; do
    echo "Running $test..."
    python3 "$test" "$SERVER_URL"
    echo
done
