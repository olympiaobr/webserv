#!./web/cgi/.venv/bin/python3
import cgi
import cgitb; cgitb.enable()
from openai import OpenAI
import sys
from dotenv import load_dotenv
import os

print("Content-Type: text/html\r\n\r\n")  # HTTP header

# Set up your OpenAI API key
load_dotenv()

client = OpenAI (
    organization='org-EUar1yTRIuR2dsQUPNZFKQGK',
    project='proj_SmXXIFVlpK2HcgrsYVMTgQSV',
    api_key = os.getenv("OPENAI_API_KEY"),
)

try:

    form = cgi.FieldStorage()

    if form.getvalue("prompt"):
        prompt = form.getvalue("prompt")
        chat_prompt = f"Help me with this question: {prompt}"
        response = client.chat.completions.create(
            messages=[
                {
                    "role": "user",
                    "content": chat_prompt,
                }
            ],
            model="gpt-3.5-turbo-0125",
        )
        chat_response = response.choices[0].message.content

        print(f"""
            <html>
                <head>
                    <title>Here is your answer:</title>
                    <link rel="stylesheet" href="/css/styles.css">
                </head>
                <body>
                    <div class="container">
                        <h1>Here is your answer:</h1>
                        <p>{chat_response}</p>
                        <a href="/">Return to homepage</a>
                    </div>
                </body>
            </html>
        """)
    else:
        print("""
        <!DOCTYPE html>
        <html lang="en">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <link rel="stylesheet" href="/css/styles.css">
            <title>Error</title>
        </head>
        <body>
            <div class="container">
                <h1>Error</h1>
                <p>No prompt provided. Please go back and enter a prompt.</p>
                <a href="/">Return to homepage</a>
            </div>
        </body>
        </html>
        """)
    sys.stdout.flush()
except Exception as e:
    print(f"""
        <!DOCTYPE html>
        <html lang="en">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <link rel="stylesheet" href="/css/styles.css">
            <title>Error</title>
        </head>
        <body>
            <div class="container">
                <h1>Error</h1>
                <p>CGI script error: {e}</p>
                <a href="/">Return to homepage</a>
            </div>
        </body>
        </html>
    """)
