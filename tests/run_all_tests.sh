#!/bin/sh

SERVER_URL=${1:-"http://localhost:8080"}

find tests/individual_tests -name "*[0-9]*.py" | while read test; do
    echo "Running $test..."
    python3 "$test" "$SERVER_URL"
    echo
done