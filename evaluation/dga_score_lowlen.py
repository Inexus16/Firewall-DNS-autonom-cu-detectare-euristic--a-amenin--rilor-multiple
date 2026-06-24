#!/usr/bin/env python3

import sys, math
from collections import Counter

def dga_score(domain):
    domain = domain.rstrip('.')
    sld = domain.split('.')[0]
    sld = sld.lower()
    length = len(sld)
    if length < 5:
        return 0.0
    letters = 0
    digits = 0
    counts = [0]*26
    total = 0
    transitions = 0
    prev_type = 0
    for c in sld:
        if 'a' <= c <= 'z':
            idx = ord(c) - ord('a')
            counts[idx] += 1
            letters += 1
            total += 1
            if prev_type == 2:
                transitions += 1
            prev_type = 1
        elif '0' <= c <= '9':
            digits += 1
            total += 1
            if prev_type == 1:
                transitions += 1
            prev_type = 2
        else:
            prev_type = 0
    if total < 2:
        return 0.0
    entropy = 0.0
    if letters > 0:
        for c in counts:
            if c > 0:
                p = c / letters
                entropy -= p * math.log2(p)
    digit_ratio = digits / length
    transition_score = transitions / (length - 1) if length > 1 else 0

    # MODIFIED: lower length thresholds
    # Original: 1.0 if >20, 0.5 if >12, else 0.0
    # Modified: 1.0 if >14, 0.5 if >8,  else 0.0
    len_score = 1.0 if length > 14 else (0.5 if length > 8 else 0.0)

    score = (entropy / 4.7) * 0.35 + digit_ratio * 0.30 + transition_score * 0.25 + len_score * 0.10
    return score

if __name__ == "__main__":
    for line in sys.stdin:
        domain = line.strip()
        if not domain:
            continue
        score = dga_score(domain)
        print(f"{domain},{score:.4f}")
