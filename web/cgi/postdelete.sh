#!/bin/bash

read method
if [ "$REQUEST_METHOD" == "POST" ]; then
    boundary=$(echo "$CONTENT_TYPE" | sed 's/.*boundary=//')
    file=$(awk -v boundary="$boundary" 'BEGIN {RS="--"boundary} NR==2 {print $0}' | awk 'NR>1' | sed 's/\r$//' > /tmp/uploaded_file)
    echo -e "Content-type: text/html\r\n\r\n"
    echo "<html><body><h1>File uploaded successfully!</h1></body></html>"
elif [ "$REQUEST_METHOD" == "DELETE" ]; then
    query=$(echo "$QUERY_STRING" | sed 's/.*file=//')
    if [ -f "/tmp/$query" ]; then
        rm "/tmp/$query"
        echo -e "Content-type: text/html\r\n\r\n"
        echo "<html><body><h1>File $query deleted successfully!</h1></body></html>"
    else
        echo -e "Content-type: text/html\r\n\r\n"
        echo "<html><body><h1>File not found</h1></body></html>"
    fi
else
    echo -e "Content-type: text/html\r\n\r\n"
    echo "<html><body><h1>Method not supported</h1></body></html>"
fi
