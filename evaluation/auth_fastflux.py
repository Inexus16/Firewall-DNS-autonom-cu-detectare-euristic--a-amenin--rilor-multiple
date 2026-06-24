#!/usr/bin/env python3
import socket
import struct

POOL = ["192.0.2.1", "192.0.2.2", "192.0.2.3", "192.0.2.4", "192.0.2.5",
        "192.0.2.6", "192.0.2.7", "192.0.2.8", "192.0.2.9", "192.0.2.10"]
counter = 0

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.bind(("127.0.0.1", 5300))
print("Fast-flux auth server on 127.0.0.1:5300 for fastflux.test")

while True:
    data, addr = sock.recvfrom(512)
    if len(data) < 12:
        continue
    tid = data[0:2]

    question = data[12:]
    flags = 0x8580
    qdcount = 1
    ancount = 1
    nscount = 0
    arcount = 0
    header = struct.pack("!HHHHHH", int.from_bytes(tid, 'big'), flags, qdcount, ancount, nscount, arcount)
    answer_name = b"\xc0\x0c"
    answer_type = struct.pack("!H", 1)
    answer_class = struct.pack("!H", 1)
    answer_ttl = struct.pack("!I", 1)
    answer_rdlength = struct.pack("!H", 4)
    ip = POOL[counter % len(POOL)]
    counter += 1
    answer_rdata = socket.inet_aton(ip)
    answer = answer_name + answer_type + answer_class + answer_ttl + answer_rdlength + answer_rdata
    response = header + question + answer
    sock.sendto(response, addr)
