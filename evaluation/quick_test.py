#!/usr/bin/env python3
import socket, struct, time, random

DNS = ("127.0.0.1", 53)
TIMEOUT = 2

def query(domain, qtype=1):
    tid = random.randint(0, 65535)
    header = struct.pack("!HHHHHH", tid, 0x0100, 1, 0, 0, 0)
    q = b""
    for label in domain.encode().split(b"."):
        q += bytes([len(label)]) + label
    q += b"\x00" + struct.pack("!HH", qtype, 1)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(TIMEOUT)
    sock.sendto(header + q, DNS)
    try:
        data, _ = sock.recvfrom(512)
    except socket.timeout:
        return None
    finally:
        sock.close()
    return data

def get_ip(response):
    if not response or len(response) < 12:
        return None
    # Search for A record (type=1, class=1) with rdlength=4
    for i in range(12, len(response)-10):
        if response[i:i+2] == b'\x00\x01' and response[i+2:i+4] == b'\x00\x01':
            rdlen = struct.unpack("!H", response[i+8:i+10])[0]
            if rdlen == 4:
                return ".".join(str(b) for b in response[i+10:i+14])
    return None

# Quick tests
tests = [
    ("google.com", "A", "benign", False),
    ("malware.test.com", "A", "blacklist", True),
    ("xyzzy12345abc.com", "A", "DGA", True),   # high entropy
]

print("Testing DNS firewall...\n")
for domain, rtype, category, expect_block in tests:
    time.sleep(0.1)
    resp = query(domain, 1 if rtype=="A" else 16)
    ip = get_ip(resp)
    blocked = (ip == "0.0.0.0")
    status = "BLOCKED" if blocked else f"IP: {ip}"
    correct = (blocked == expect_block)
    print(f"{category:12} {domain:25} → {status:15} | {'✓' if correct else '✗ expected block'}")
