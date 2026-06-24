#!/usr/bin/env python3
import struct, socket, time, os
from cryptography.hazmat.primitives.asymmetric import rsa, padding
from cryptography.hazmat.primitives import hashes, serialization

ZONE = "signed.test."
DOMAIN = "www.signed.test."
IP = "192.0.2.1"
RESPONSE_FILE = "signed_response.bin"
TRUSTED_KEY_FILE = "trusted_key.pem"

#  RSA key pair
private_key = rsa.generate_private_key(public_exponent=65537, key_size=2048)
public_key = private_key.public_key()

# public key as PEM 
with open(TRUSTED_KEY_FILE, "wb") as f:
    f.write(public_key.public_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PublicFormat.SubjectPublicKeyInfo
    ))
print("[+] Trust anchor saved to trusted_key.pem")

# DNSKEY RDATA RFC 4034
# flags: 257 KSK, Secure Entry Point
# protocol: 3 DNSSEC
# algorithm: 8 RSA/SHA256
# public key: exponent length (1 byte if <256) + exponent + modulus
pub_numbers = public_key.public_numbers()
exponent = pub_numbers.e.to_bytes((pub_numbers.e.bit_length()+7)//8, 'big')
modulus = pub_numbers.n.to_bytes(256, 'big')  # 2048 bits = 256 bytes

if len(exponent) <= 255:
    key_blob = bytes([len(exponent)]) + exponent + modulus
else:
    key_blob = b'\x00' + struct.pack("!H", len(exponent)) + exponent + modulus

dnskey_rdata = struct.pack("!HBB", 257, 3, 8) + key_blob

# key tag (RFC 4034 appendix B)
def key_tag(rdata):
    ac = 0
    for i in range(0, len(rdata), 2):
        if i+1 < len(rdata):
            ac += (rdata[i] << 8) | rdata[i+1]
        else:
            ac += rdata[i] << 8
    ac += (ac >> 16) & 0xFFFF
    return ac & 0xFFFF
kt = key_tag(dnskey_rdata)

# wire format domain names
name_www = b'\x03www\x06signed\x04test\x00'        # www.signed.test.
name_zone = b'\x06signed\x04test\x00'              # signed.test.

# record RRset
a_rdata = socket.inet_aton(IP)
rrset_a = name_www + struct.pack("!HHIH", 1, 1, 300, 4) + a_rdata

#RRSIG RDATA (without signature)
now = int(time.time())
sig_incept = now - 86400
sig_expire = now + 86400

rrsig_hdr = struct.pack("!HBBIIIH",
    1,      # type covered = A
    8,      # algorithm = RSA/SHA256
    3,      # labels = 3 (www.signed.test.)
    300,    # original TTL
    sig_expire,
    sig_incept,
    kt       # key tag
) + name_zone

# sign the RRset
to_sign = rrsig_hdr + rrset_a
signature = private_key.sign(to_sign, padding.PKCS1v15(), hashes.SHA256())

rrsig_rdata = rrsig_hdr + signature
rrsig_rr = name_www + struct.pack("!HHIH", 46, 1, 300, len(rrsig_rdata)) + rrsig_rdata

# DNSKEY RR
dnskey_rr = name_zone + struct.pack("!HHIH", 48, 1, 3600, len(dnskey_rdata)) + dnskey_rdata

# full response packet
tid = struct.pack("!H", 0x1234)
flags = 0x8580   # QR=1, AA=1, RA=1
counts = struct.pack("!HHHH", 1, 2, 0, 1)   # QD=1, AN=2, NS=0, AR=1
question = name_www + struct.pack("!HH", 1, 1)
answer = rrset_a + rrsig_rr
additional = dnskey_rr

response = tid + struct.pack("!H", flags) + counts + question + answer + additional

with open(RESPONSE_FILE, "wb") as f:
    f.write(response)
print(f"[+] Signed response saved to {RESPONSE_FILE}")
