#!/usr/bin/env python3

import cgi
import os
import sys
import html

UPLOAD_DIR = os.getenv("UPLOAD_DIR")

if not os.path.exists(UPLOAD_DIR):
    os.makedirs(UPLOAD_DIR, mode=0o755, exist_ok=True)

def main():
    print("Content-Type: text/html\r\n\r\n")  # HTTP header

    try:
		# parse the query string to get filename
        query_string = os.environ.get("QUERY_STRING", "")
        filename = query_string.split("=")[1] if "=" in query_string else None

        if not filename:
            print("Error: No filename specified.")
            return

        # sanitize filename
        sanitized_filename = "".join(c for c in filename if c.isalnum() or c in "._-")
        file_path = os.path.join(UPLOAD_DIR, sanitized_filename)

        # read content from stdin
        content_length = int(os.environ.get("CONTENT_LENGTH", 0))
        if content_length == 0:
            print("Error: No content received.")
            return

        file_content = sys.stdin.read(content_length)

        # write file (create or overwrite)
        try:
            with open(file_path, "w") as file:
                file.write(file_content)
            if os.path.exists(file_path):
                print(f"File '{sanitized_filename}' has been updated successfully.")
            else:
                print(f"File '{sanitized_filename}' has been created successfully.")
        except Exception as e:
            print(f"Error writing to file '{sanitized_filename}': {str(e)}")
    except Exception as e:
        print(f"Error during PUT request: {str(e)}")

if __name__ == "__main__":
    main()
