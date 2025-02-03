#!/usr/bin/env python3

import os
import sys
import cgi
import cgitb
cgitb.enable()

def main():
    try:
        # Get the upload directory from environment
        UPLOAD_DIR = os.getenv("UPLOAD_DIR")
        if not UPLOAD_DIR:
            print("Status: 500 Internal Server Error")
            print("Content-Type: text/plain\n")
            print("Error: UPLOAD_DIR environment variable not set")
            return

        # Create upload directory if it doesn't exist
        if not os.path.exists(UPLOAD_DIR):
            os.makedirs(UPLOAD_DIR, mode=0o755, exist_ok=True)

        # Read raw data from stdin until EOF
        raw_data = sys.stdin.buffer.read()

        # Get filename from environment variable or use default
        filename = os.environ.get('HTTP_X_FILENAME', 'chunked_file')

        # Sanitize filename
        filename = "".join(char if char.isalnum() or char in "._-" else "_" for char in filename)

        # Create full path
        filepath = os.path.join(UPLOAD_DIR, filename)

        # Save the raw data
        with open(filepath, 'wb') as f:
            f.write(raw_data)

        print("Status: 200 OK")
        print("Content-Type: text/plain\n")
        print(f"Success: Chunked file {filename} has been saved")

    except Exception as e:
        print("Status: 500 Internal Server Error")
        print("Content-Type: text/plain\n")
        print(f"Error: {str(e)}")

if __name__ == "__main__":
    main()
