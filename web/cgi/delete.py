#!/usr/bin/env python3

import cgi
import os
import sys
import html

UPLOAD_DIR = os.getenv("UPLOAD_DIR")

if not os.path.exists(UPLOAD_DIR):
    os.makedirs(UPLOAD_DIR, mode=0o755, exist_ok=True)

def main():
    print("Content-Type: text/plain\n")

	 try:
        # parse the query string to get filename
        query_string = os.environ.get("QUERY_STRING", "")
        params = parse_qs(query_string)

        # validate the 'file' parameter
        if "file" not in params or not params["file"]:
            print("<h2>Error:</h2><p>Missing 'file' parameter in the query string.</p>")
            return

        file_name = params["file"][0]  # extract file name
        sanitized_file_name = "".join(
            char if char.isalnum() or char in "._-" else "_" for char in file_name
        )  # Sanitize file name
        file_path = os.path.join(UPLOAD_DIR, sanitized_file_name)

        if not os.path.exists(file_path):
            print(f"<h2>Error:</h2><p>File '{sanitized_file_name}' not found.</p>")
            return

        try:
            os.remove(file_path)
            print(f"<h2>Success:</h2><p>File '{sanitized_file_name}' has been deleted.</p>")
        except Exception as e:
            print(f"<h2>Error:</h2><p>Unable to delete file '{sanitized_file_name}': {str(e)}</p>")

    except Exception as e:
        print(f"<h2>Error:</h2><p>An unexpected error occurred: {str(e)}</p>")

if __name__ == "__main__":
    main()
