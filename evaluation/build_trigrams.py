#!/usr/bin/env python3
import sys, math, struct
from collections import Counter

def extract_trigrams(domain):
    sld = domain.strip().split('.')[0].lower()
    letters = [c for c in sld if 'a' <= c <= 'z']
    if len(letters) < 3:
        return []
    return [''.join(letters[i:i+3]) for i in range(len(letters)-2)]

def build_model(input_file, output_file, top_n=100000):
    counter = Counter()
    with open(input_file, 'r') as f:
        for i, line in enumerate(f):
            if i >= top_n:
                break
            domain = line.strip()
            trigrams = extract_trigrams(domain)
            counter.update(trigrams)
    
    total = sum(counter.values())
    with open(output_file, 'wb') as out:
        out.write(struct.pack('<I', len(counter)))
        for trig, count in counter.most_common():
            prob = (count + 1) / (total + len(counter))  # smoothing simplu
            # trigrama (3 bytes) + log prob (float32)
            out.write(trig.encode('ascii'))
            out.write(struct.pack('<f', math.log2(prob)))
    print(f"Saved {len(counter)} trigrams to {output_file}")

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: build_trigrams.py <benign_domains.txt> <trigrams.bin>")
        sys.exit(1)
    build_model(sys.argv[1], sys.argv[2])
