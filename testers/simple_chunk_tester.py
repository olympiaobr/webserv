import http.client
import time

# Connect to localhost on port 8080
conn = http.client.HTTPConnection('localhost', 8080)

# Define chunks
chunks = [
    b'4\r\nwiki\r\n',
    b'6\r\nwikipedia\r\n',
    b'0\r\n\r\n'
]

# Send first request
conn.request(
    'POST',
    '/',
    body=chunks[0],
    headers={
        'Transfer-Encoding': 'chunked',
        'Content-Type': 'application/octet-stream',
		'Content-Length': '9',
    }
)
response1 = conn.getresponse()
print("First request:", response1.status, response1.reason)
print(response1.read().decode())

# Small delay to simulate real-world usage
time.sleep(1)

# Send second request
conn.request(
    'POST',
    '/',
    body=chunks[1] + chunks[2],
    headers={
        'Transfer-Encoding': 'chunked',
        'Content-Type': 'application/octet-stream',
		'Content-Length': '19',
    }
)
response2 = conn.getresponse()
print("Second request:", response2.status, response2.reason)
print(response2.read().decode())

# Close the connection
conn.close()
