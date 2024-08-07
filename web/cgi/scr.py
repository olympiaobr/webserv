#!/usr/bin/python3
import cgi
import cgitb; cgitb.enable()

print("Content-type: text/html\n")

form = cgi.FieldStorage()

if form.getvalue("data"):
    data = form.getvalue("data")
    print("<h1>Data received: {}</h1>".format(data))
else:
    print("""
        <html>
        <body>
            <form method="post" action="/cgi-bin/form_handler.py">
                <label for="data">Enter Data:</label>
                <input type="text" id="data" name="data">
                <input type="submit" value="Submit">
            </form>
        </body>
        </html>
    """)
