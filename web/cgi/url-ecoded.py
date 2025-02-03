#!./web/cgi/.venv/bin/python
import cgi
import cgitb
import html

# Enable detailed error reporting
cgitb.enable()

# Print necessary HTTP headers
print("Content-Type: text/html\r\n\r\n")

# Create instance of FieldStorage
form = cgi.FieldStorage()

# Start HTML output
print("""
<!DOCTYPE html>
<html>
<head>
    <title>Form Results</title>
</head>
<body>
""")

# Process form data
if form:
    for field in form.keys():
        # Get value and escape special characters
        value = html.escape(form[field].value)
        print(f"<p>{html.escape(field)}: {value}</p>")
else:
    print("<p>No form data received</p>")

# End HTML output
print("""
</body>
</html>
""")
