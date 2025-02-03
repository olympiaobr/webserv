#!./web/cgi/.venv/bin/python3
import cgi
import cgitb; cgitb.enable()
import sys

print("Content-Type: text/html\r\n\r\n")  # HTTP header

form = cgi.FieldStorage()

if form.getvalue("username"):
    username = form.getvalue("username")
    print(f"""
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Error</title>
        <link rel="stylesheet" href="/css/styles.css">
    </head>
    <body>
        <h1>Welcome, {username}!</h1>
        <p>Thank you for submitting your username.</p>
        <a href="/">Return to homepage</a>
    </body>
    </html>
    """)
else:
    print("""
    <html>
    <head>
        <title>Error</title>
        <link rel="stylesheet" href="/css/styles.css">
    </head>
    <body>
        <h1>Error</h1>
        <p>No username provided. Please go back and enter a username.</p>
        <a href="/">Return to homepage</a>
    </body>
    </html>
    """)

sys.stdout.flush()
