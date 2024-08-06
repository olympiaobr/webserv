import http.client
import time

# Connect to localhost on port 8080
conn = http.client.HTTPConnection('localhost', 8080)

# Define chunks
chunks = [
    b'6\r\nchunk1 \r\n',
    b'6\r\nchunk2 skipped_data\r\n',
    b'0\r\n\r\n'
]

# Send first request
conn.request(
    'POST',
    '/upload/',
    body=chunks[0],
    headers={
        'Transfer-Encoding': 'chunked',
        'Content-Type': 'application/octet-stream',
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
    '/upload/',
    body=chunks[1] + chunks[2],
    headers={
        'Transfer-Encoding': 'chunked',
        'Content-Type': 'application/octet-stream',
    }
)
response2 = conn.getresponse()
print("Second request:", response2.status, response2.reason)
print(response2.read().decode())

# time.sleep(1)

lorem = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nunc maximus sollicitudin ex, non scelerisque turpis finibus maximus. Donec congue gravida turpis in semper. Mauris suscipit viverra mauris, quis tincidunt elit vestibulum ac. Cras ut semper lacus. Vivamus eget scelerisque libero. Integer gravida libero a feugiat interdum. Maecenas vitae efficitur nulla. Fusce consectetur eros tempus rutrum imperdiet. \
Suspendisse sit amet ipsum iaculis, lacinia diam nec, facilisis nisl. Donec nec ante quis est fermentum faucibus. Donec vitae est id magna feugiat varius. Cras sed quam ac massa ornare luctus non et quam. Sed sed turpis dictum, consequat leo vitae, convallis mauris. Aliquam pellentesque quis ligula nec mollis. Fusce tellus nibh, euismod in metus ut, rutrum venenatis tellus. Lorem ipsum dolor sit amet, consectetur adipiscing elit. \
Vestibulum a condimentum elit. Suspendisse semper finibus ante, vel aliquet ante suscipit id. Ut aliquet lectus aliquet, vulputate diam ut, viverra lectus. Sed eu eros dolor. Vivamus bibendum placerat arcu, vel suscipit nisl mollis nec. Morbi gravida tortor sed tristique hendrerit. Sed vel odio sodales, eleifend ipsum nec, malesuada erat. Fusce eget rutrum lacus, non suscipit diam.\
Praesent volutpat accumsan massa, nec fermentum metus euismod eu. Donec dignissim auctor lorem consectetur faucibus. Donec dignissim leo vel neque imperdiet faucibus. Vivamus finibus ante et lectus volutpat, in tempus enim congue. Pellentesque gravida cursus mi sed laoreet. Quisque aliquam at nunc sit amet fringilla. Duis sit amet velit vel ligula commodo dictum. Nunc ornare velit sit amet velit dapibus, sit amet interdum lorem iaculis. Curabitur quis eros bibendum, rhoncus nisl sit amet, facilisis dolor. Sed hendrerit placerat mollis. Etiam ex velit, pretium in risus ac, rhoncus sodales tortor. Cras et sagittis lacus. Vivamus dictum finibus quam eget rhoncus. Vivamus turpis augue, tincidunt non tempor vitae, rhoncus eget risus. Curabitur faucibus lacus id augue."

conn.request(
	    'POST',
    '/upload/',
    body= lorem,
    headers={
        'Content-Type': 'application/octet-stream',
    }
)

response3 = conn.getresponse()
print("Third request:", response3.status, response3.reason)
print(response3.read().decode())


# Close the connection
conn.close()
