#!/usr/bin/env python3

import cgi
import os
import sys
import html
from urllib.parse import parse_qs

UPLOAD_DIR = os.getenv("UPLOAD_DIR")
if UPLOAD_DIR is None:
    print("Content-Type: text/plain\r\n\r\n")
    print("Error: UPLOAD_DIR environment variable not set")
    sys.exit(1)

if not os.path.exists(UPLOAD_DIR):
    os.makedirs(UPLOAD_DIR, mode=0o755, exist_ok=True)


def main():
    print("Content-Type: text/plain\r\n\r\n")

    try:
        # Parse the query string to get the filename
        query_string = os.environ.get("QUERY_STRING", "")
        params = parse_qs(query_string)

        # Validate the 'file' parameter
        if "file" not in params or not params["file"]:
            print("<h2>Error:</h2><p>Missing 'file' parameter in the query string.</p>")
            return

        # Extract the file name directly (do not sanitize here)
        file_name = params["file"][0]
        file_path = os.path.join(UPLOAD_DIR, file_name)

        # Check if the file exists
        if not os.path.exists(file_path):
            print(f"<h2>Error:</h2><p>File '{file_name}' not found.</p>")
            return

        # Attempt to delete the file
        try:
            os.remove(file_path)
            print(f"<h2>Success:</h2><p>File '{file_name}' has been deleted.</p>")
        except Exception as e:
            print(f"<h2>Error:</h2><p>Unable to delete file '{file_name}': {str(e)}</p>")

    except Exception as e:
        print(f"<h2>Error:</h2><p>An unexpected error occurred: {str(e)}</p>")

if __name__ == "__main__":
    main()
