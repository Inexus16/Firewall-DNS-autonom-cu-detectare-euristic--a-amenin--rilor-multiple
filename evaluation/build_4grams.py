#!/usr/bin/env python3
import sys, math, struct
from collections import Counter

def extract_4grams(domain):
    sld = domain.strip().split('.')[0].lower()
    letters = [c for c in sld if 'a' <= c <= 'z']
    if len(letters) < 4:
        return []
    return [''.join(letters[i:i+4]) for i in range(len(letters)-3)]

def build_model(input_file, output_file, top_n=500000):
    counter = Counter()
    with open(input_file, 'r') as f:
        for i, line in enumerate(f):
            if i >= top_n:
                break
            domain = line.strip()
            grams = extract_4grams(domain)
            counter.update(grams)
    total = sum(counter.values())
    # Laplace smoothing: add 1 to all observed grams + total
    with open(output_file, 'wb') as out:
        out.write(struct.pack('<I', len(counter)))
        for gram, count in counter.most_common():
            prob = (count + 1) / (total + len(counter))
            out.write(gram.encode('ascii'))
            out.write(struct.pack('<f', math.log2(prob)))
    print(f"Saved {len(counter)} 4-grams to {output_file}")

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: build_4grams.py <input_domains.txt> <output.bin>")
        sys.exit(1)
    build_model(sys.argv[1], sys.argv[2])
