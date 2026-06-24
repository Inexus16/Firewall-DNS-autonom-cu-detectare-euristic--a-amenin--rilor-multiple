#!/usr/bin/env python3
import sys, math, struct

# 4‑gram model start
def load_ngram_model(path="4grams.bin"):
    table = {}
    try:
        with open(path, "rb") as f:
            raw = f.read(4)
            if len(raw) < 4: return table
            count = struct.unpack("<I", raw)[0]
            for _ in range(count):
                gram = f.read(4).decode("ascii", errors="replace")
                logp = struct.unpack("<f", f.read(4))[0]
                table[gram] = logp
    except FileNotFoundError:
        print(f"Warning: ngram file {path} not found", file=sys.stderr)
    return table

ngram_model = load_ngram_model()
MIN_LOGPROB = -20.0
LOG_LO = -20.0
LOG_HI = -10.34

def ngram_score_domain(domain):
    if not ngram_model: return 0.0
    sld = domain.strip().split('.')[0].lower()
    letters = [c for c in sld if 'a' <= c <= 'z']
    if len(letters) < 4: return 0.0
    grams = [''.join(letters[i:i+4]) for i in range(len(letters)-3)]
    sum_log = 0.0
    for g in grams:
        sum_log += ngram_model.get(g, MIN_LOGPROB)
    avg = sum_log / len(grams)
    norm = (avg - LOG_LO) / (LOG_HI - LOG_LO)
    norm = max(0.0, min(1.0, norm))
    return 1.0 - norm

# vowel/consonant score
def vowel_cons_score(domain):
    sld = domain.strip().split('.')[0].lower()
    vowels = 0; cons = 0; alt = 0; prev = -1
    for c in sld:
        if 'a' <= c <= 'z':
            if c in 'aeiou':
                if prev == 1: alt += 1
                prev = 0; vowels += 1
            else:
                if prev == 0: alt += 1
                prev = 1; cons += 1
    total = vowels + cons
    if total < 2: return 0.0
    ratio = vowels / total
    ratio_score = 1.0 - abs(ratio - 0.45) * 2.0
    if ratio_score < 0.0: ratio_score = 0.0
    alt_score = alt / (total - 1)
    return ratio_score * 0.5 + alt_score * 0.5

# Logistic regression
lr_coeffs = None
try:
    with open("lr_coeffs.bin", "rb") as f:
        data = f.read()
        import struct, array
        lr_coeffs = array.array('f')
        lr_coeffs.frombytes(data[:24])  # 6 floats
except:
    pass

def lr_score(features):
    if lr_coeffs is None: return None
    s = lr_coeffs[5]  # intercept
    for i in range(5): s += lr_coeffs[i] * features[i]
    return 1.0 / (1.0 + math.exp(-s))

# main scoring function 
def dga_score(domain):
    domain = domain.rstrip('.')
    sld = domain.split('.')[0].lower()
    length = len(sld)
    if length < 5: return 0.0

    letters = 0; digits = 0
    counts = [0]*26; total = 0
    transitions = 0; prev_type = 0
    for c in sld:
        if 'a' <= c <= 'z':
            idx = ord(c)-ord('a'); counts[idx]+=1; letters+=1; total+=1
            if prev_type == 2: transitions+=1
            prev_type = 1
        elif '0' <= c <= '9':
            digits+=1; total+=1
            if prev_type == 1: transitions+=1
            prev_type = 2
        else:
            prev_type = 0
    if total < 2: return 0.0

    entropy = 0.0
    if letters > 0:
        for cnt in counts:
            if cnt: p = cnt/letters; entropy -= p*math.log2(p)
    entropy_norm = entropy / 4.7
    digit_ratio = digits / length
    transition_score = transitions / (length-1) if length>1 else 0
    len_score = 1.0 if length>20 else (0.5 if length>12 else 0.0)
    ng = ngram_score_domain(domain)
    vc = vowel_cons_score(domain)

    if lr_coeffs is not None:
        feats = [entropy_norm, digit_ratio, transition_score, len_score, ng]
        prob = lr_score(feats)
        if prob is not None: return prob

    # weighted combination
    score = entropy_norm * 0.20 + digit_ratio * 0.15 + transition_score * 0.15 + len_score * 0.10 + ng * 0.25 + vc * 0.15
    return score

if __name__ == "__main__":
    for line in sys.stdin:
        domain = line.strip()
        if not domain: continue
        s = dga_score(domain)
        print(f"{domain},{s:.4f}")
