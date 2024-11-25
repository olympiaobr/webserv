#!./web/cgi/.venv/bin/python3

import os
import cgi
import cgitb
import sys

cgitb.enable()

def handle_post():
    form = cgi.FieldStorage()
    file_item = form["file"] if "file" in form else None

    if file_item and file_item.file:
        file_path = f"/tmp/{file_item.filename}"
        with open(file_path, "wb") as f:
            f.write(file_item.file.read())
        print("Content-type: text/html\r\n\r\n")
        print("<html><body><h1>File uploaded successfully!</h1></body></html>")
    else:
        print("Content-type: text/html\r\n\r\n")
        print("<html><body><h1>No file uploaded</h1></body></html>")

def handle_delete():
    query = os.environ.get("QUERY_STRING", "")
    file_to_delete = query.split("=")[1] if "=" in query else None

    if file_to_delete:
        try:
            os.remove(f"/tmp/{file_to_delete}")
            print("Content-type: text/html\r\n\r\n")
            print(f"<html><body><h1>File {file_to_delete} deleted successfully!</h1></body></html>")
        except FileNotFoundError:
            print("Content-type: text/html\r\n\r\n")
            print(f"<html><body><h1>File {file_to_delete} not found</h1></body></html>")
    else:
        print("Content-type: text/html\r\n\r\n")
        print("<html><body><h1>No file specified for deletion</h1></body></html>")

def main():
    method = os.environ.get("REQUEST_METHOD", "")
    if method == "POST":
        handle_post()
    elif method == "DELETE":
        handle_delete()
    else:
        print("Content-type: text/html\r\n\r\n")
        print("<html><body><h1>Method not supported</h1></body></html>")

if __name__ == "__main__":
    main()
