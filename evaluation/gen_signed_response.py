#!/usr/bin/env python3
import subprocess, struct, socket, time, os, tempfile

ZONE = "signed.test."
DOMAIN = "www.signed.test."
IP = "192.0.2.1"
RESPONSE_FILE = "signed_response.bin"
TRUSTED_KEY_FILE = "trusted_key.pem"
PRIV_KEY_FILE = "temp_private.pem"

print("[*] Generating RSA key pair...")
subprocess.run(
    ["openssl", "genpkey", "-algorithm", "RSA", "-out", PRIV_KEY_FILE,
     "-pkeyopt", "rsa_keygen_bits:2048"],
    check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
)
subprocess.run(
    ["openssl", "rsa", "-in", PRIV_KEY_FILE, "-pubout", "-out", TRUSTED_KEY_FILE],
    check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
)
print("[+] Trust anchor saved to trusted_key.pem")

# extract modulus and exponent
modulus_hex = subprocess.check_output(
    ["openssl", "rsa", "-in", PRIV_KEY_FILE, "-noout", "-modulus"]
).decode().strip().split("=")[1]

# openssl to print public key info and grab exponent
text = subprocess.check_output(
    ["openssl", "rsa", "-in", PRIV_KEY_FILE, "-noout", "-text"]
).decode()
# Find line with "Exponent:"
for line in text.splitlines():
    if "Exponent:" in line:
        # "Exponent: 65537 (0x10001)" or "Exponent: 65537"
        parts = line.split(":")[1].strip().split()
        exp_str = parts[0]  # decimal string
        break
exp_val = int(exp_str)  # decimal integer

# convert to bytes of required length
exp_bytes = exp_val.to_bytes((exp_val.bit_length() + 7) // 8, byteorder='big')
if len(exp_bytes) <= 255:
    exp_blob = bytes([len(exp_bytes)]) + exp_bytes
else:
    exp_blob = b'\x00' + struct.pack("!H", len(exp_bytes)) + exp_bytes
modulus = bytes.fromhex(modulus_hex)

# DNSKEY RDATA
flags = struct.pack("!H", 257)   # KSK
protocol = b'\x03'	# 3 = DNSSEC
algorithm = b'\x08'    # RSA/SHA-256
pubkey = exp_blob + modulus #concatenate
dnskey_rdata = flags + protocol + algorithm + pubkey

# key tag
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

# record RRset
name_wire = b'\x03www\x06signed\x04test\x00'
a_rdata = socket.inet_aton(IP)
rrset_a = name_wire + struct.pack("!HHIH", 1, 1, 300, 4) + a_rdata

# RRSIG header
now = int(time.time())
sig_incept = now - 86400
sig_expire = now + 86400
signer_wire = b'\x06signed\x04test\x00'
rrsig_header = struct.pack("!HBBIIIH",
    1, 8, 3, 300, sig_expire, sig_incept, kt
) + signer_wire

# sign the RRset
to_sign = rrsig_header + rrset_a
tmp = tempfile.NamedTemporaryFile(delete=False)
tmp.write(to_sign)
tmp.close()
signature = subprocess.check_output(
    ["openssl", "dgst", "-sha256", "-sign", PRIV_KEY_FILE, tmp.name]
)
os.unlink(tmp.name)

rrsig_rdata = rrsig_header + signature
rrsig_rr = name_wire + struct.pack("!HHIH", 46, 1, 300, len(rrsig_rdata)) + rrsig_rdata

# DNSKEY RR
dnskey_name = b'\x06signed\x04test\x00'
dnskey_rr = dnskey_name + struct.pack("!HHIH", 48, 1, 3600, len(dnskey_rdata)) + dnskey_rdata

# response packet
tid = struct.pack("!H", 0x1234)
flags_resp = 0x8580				#KSK
counts = struct.pack("!HHHH", 1, 2, 0, 1)
question = name_wire + struct.pack("!HH", 1, 1)
answer = rrset_a + rrsig_rr
additional = dnskey_rr

response = tid + struct.pack("!H", flags_resp) + counts + question + answer + additional

with open(RESPONSE_FILE, "wb") as f:
    f.write(response)
print(f"[+] Signed response saved to {RESPONSE_FILE}")

os.unlink(PRIV_KEY_FILE)
print("[*] Done.")
