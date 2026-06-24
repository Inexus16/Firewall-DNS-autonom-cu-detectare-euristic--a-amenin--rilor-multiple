#!/usr/bin/env python3
import requests
import time
import sys

URL = "http://127.0.0.1:8053/dns-query"
QUERY = b'\x00\x01\x01\x00\x00\x01\x00\x00\x00\x00\x00\x00\x06google\x03com\x00\x00\x01\x00\x01'
HEADERS = {"Content-Type": "application/dns-message"}

N = 500
start = time.time()
for i in range(N):
    resp = requests.post(URL, data=QUERY, headers=HEADERS)
    if resp.status_code != 200:
        print(f"Error at {i}: {resp.status_code}")
        sys.exit(1)
elapsed = time.time() - start
print(f"DoH: {N} requests in {elapsed:.2f}s, {N/elapsed:.2f} QPS")
