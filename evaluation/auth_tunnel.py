#!/usr/bin/env python3
import socket, struct

DOMAIN = "tunnel.test"
TXT_DATA = b"X" * 400

# split TXT_DATA into chunk≤ 255 bytes
def encode_txt(data):
    encoded = b""
    while data:
        chunk = data[:255]
        data = data[255:]
        encoded += bytes([len(chunk)]) + chunk
    return encoded

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.bind(("127.0.0.1", 5301))
print(f"Tunnel auth server on 127.0.0.1:5301 for {DOMAIN}")

while True:
    data, addr = sock.recvfrom(2048)
    if len(data) < 12:
        continue
    tid = data[:2]
    flags = 0x8580
    q = data[12:]
    txt_rdata = encode_txt(TXT_DATA)
    rdlength = len(txt_rdata)
    answer = b"\xc0\x0c" + struct.pack("!HHIH", 16, 1, 300, rdlength) + txt_rdata
    response = tid + struct.pack("!H", flags) + b"\x00\x01\x00\x01\x00\x00\x00\x00" + q + answer
    sock.sendto(response, addr)
