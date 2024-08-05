#!/bin/bash

# Set the server URL, default to localhost:8080 if not provided
SERVER_URL="${1:-"http://localhost:8080"}"

# Define virtual environment name
VENV_NAME="tests/test_env"

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Determine the Python command to use
if command_exists python3; then
    PYTHON_CMD="python3"
elif command_exists python; then
    PYTHON_CMD="python"
else
    echo "Error: Python is not installed or not in PATH"
    exit 1
fi

# Create virtual environment if it doesn't exist
if [ ! -d "$VENV_NAME" ]; then
    echo "Creating virtual environment..."
    if $PYTHON_CMD -m venv "$VENV_NAME" 2>/dev/null; then
        echo "Virtual environment created successfully."
    else
        echo "Warning: Could not create virtual environment. Proceeding without it."
        VENV_NAME=""
    fi
fi

# Activate virtual environment if it exists
if [ -n "$VENV_NAME" ] && [ -f "$VENV_NAME/bin/activate" ]; then
    source "$VENV_NAME/bin/activate"
    echo "Virtual environment activated."
else
    echo "Warning: Running without virtual environment."
fi

# Install requirements if pip is available
if command_exists pip; then
    pip install -r tests/requirements.txt
else
    echo "Warning: pip is not available. Skipping package installation."
fi

# Run the tests
find tests/individual_tests -name "*[0-9]*.py" | while read test; do
    echo "Running $test..."
    $PYTHON_CMD "$test" "$SERVER_URL"
    echo
done

# Deactivate virtual environment if it was activated
if [ -n "$VENV_NAME" ]; then
    deactivate
    echo "Virtual environment deactivated."
fi