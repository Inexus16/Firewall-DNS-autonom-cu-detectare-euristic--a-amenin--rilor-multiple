#!/usr/bin/env python3
import socket, struct, sys

ZONE_FILE = "signed.test.zone"

#  signed zone and store all RRs by name+type
zone_rrs = {}
current_name = None
with open(ZONE_FILE) as f:
    for line in f:
        line = line.strip()
        if not line or line.startswith(';'):
            continue
        parts = line.split()
        if parts[0] == '@':
            name = "signed.test."
        else:
            name = parts[0] if parts[0].endswith('.') else parts[0] + '.'
        ttl = int(parts[1])
        rtype = parts[2]
        rdata = ' '.join(parts[3:])

