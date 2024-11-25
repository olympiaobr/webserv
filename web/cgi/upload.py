#!/usr/bin/env python3

import cgi
import os
import sys

# Directory where uploaded files will be saved
UPLOAD_DIR = "../uploads"

# Ensure the upload directory exists
if not os.path.exists(UPLOAD_DIR):
    os.makedirs(UPLOAD_DIR, mode=0o755, exist_ok=True)

def main():
    print("Content-Type: text/html\n")  # HTTP header

    try:
        # Parse the form data
        # content_length = int(os.environ.get("CONTENT_LENGTH", 0))
        # body = sys.stdin.read(content_length)
        form = cgi.FieldStorage(fp=sys.stdin, environ=os.environ, keep_blank_values=True)

        # Check if the form contains files
        if "uploaded_files" not in form:
            print(form)
            # print(body)
            print("No files uploaded.")
            return

        files = form["uploaded_files"]

        # Ensure files is a list for single or multiple uploads
        if not isinstance(files, list):
            files = [files]

        uploaded_files = []
        errors = []

        for file_item in files:
            # Extract file name and validate
            file_name = os.path.basename(file_item.filename)
            if not file_name:
                errors.append("Invalid file name.")
                continue

            # Sanitize file name
            file_name = "".join(char if char.isalnum() or char in "._-" else "_" for char in file_name)

            # Limit file size (100MB)
            MAX_FILE_SIZE = 100 * 1024 * 1024  # 100MB
            file_size = os.fstat(file_item.file.fileno()).st_size
            if file_size > MAX_FILE_SIZE:
                errors.append(f"File '{file_name}' exceeds the maximum allowed size of 5MB.")
                continue

            # Save the uploaded file
            upload_file_path = os.path.join(UPLOAD_DIR, file_name)
            if os.path.exists(upload_file_path):
                errors.append(f"File '{file_name}' already exists.")
                continue

            try:
                with open(upload_file_path, "wb") as output_file:
                    while chunk := file_item.file.read(1024 * 1024):  # Read in chunks of 1MB
                        output_file.write(chunk)
                uploaded_files.append(file_name)
            except Exception as e:
                errors.append(f"Error saving file '{file_name}': {str(e)}")

        # Display the results
        if uploaded_files:
            print("<h2>Uploaded Files:</h2>")
            print("<ul>")
            for file_name in uploaded_files:
                print(f"<li>{cgi.escape(file_name)}</li>")
            print("</ul>")

        if errors:
            print("<h2>Errors:</h2>")
            print("<ul>")
            for error in errors:
                print(f"<li>{error}</li>")
            print("</ul>")

    except Exception as e:
        print(f"Error during file upload: {str(e)}")

if __name__ == "__main__":
    main()
