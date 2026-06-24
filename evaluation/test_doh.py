#!/usr/bin/env python3
import socket, base64, struct

# DNS query for google.com A
domain = b'google.com'
q = b''
for label in domain.split(b'.'):
    q += bytes([len(label)]) + label
q += b'\x00'
q += struct.pack('!HH', 1, 1)  # type A, class IN
query = struct.pack('!H', 0x1234) + b'\x01\x00\x00\x01\x00\x00\x00\x00\x00\x00' + q

# base64url encode
b64 = base64.urlsafe_b64encode(query).decode()
url = f'/dns-query?dns={b64}'
request = f'GET {url} HTTP/1.1\r\nHost: 127.0.0.1:8053\r\nConnection: close\r\n\r\n'

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('127.0.0.1', 8053))
sock.sendall(request.encode())
response = b''
while True:
    chunk = sock.recv(4096)
    if not chunk: break
    response += chunk
sock.close()

# extract DNS response after \r\n\r\n
body_start = response.find(b'\r\n\r\n') + 4
dns_resp = response[body_start:]
if len(dns_resp) >= 12:
    # crude IP extraction
    ip = '.'.join(str(b) for b in dns_resp[-4:])
    print(f'Resolved IP: {ip}')
else:
    print('No valid DNS response')
