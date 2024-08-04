#!/bin/bash

# Set the server URL, default to localhost:8080 if not provided
SERVER_URL=${1:-"http://localhost:8080"}

# Define virtual environment name
VENV_NAME="tests/test_env"

# Create virtual environment if it doesn't exist
if [ ! -d "$VENV_NAME" ]; then
    echo "Creating virtual environment..."
    python3 -m venv "$VENV_NAME"
fi

# Activate virtual environment
source "$VENV_NAME/bin/activate"

# Install requirements
pip install -r tests/requirements.txt

# Run the tests
find tests/individual_tests -name "*[0-9]*.py" | while read test; do
    echo "Running $test..."
    python "$test" "$SERVER_URL"
    echo
done

# Deactivate virtual environment
deactivate