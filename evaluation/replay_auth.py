#!/usr/bin/env python3
import socket

RESPONSE_FILE = "signed_response.bin"

with open(RESPONSE_FILE, "rb") as f:
    RESPONSE = f.read()

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.bind(("127.0.0.1", 5302))
print("Replay auth server on 127.0.0.1:5302 for signed.test")

while True:
    data, addr = sock.recvfrom(512)
    if data:
        sock.sendto(RESPONSE, addr)
