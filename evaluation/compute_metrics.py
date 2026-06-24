#!/usr/bin/env python3
import sys

def load_scores(filename):
    scores = []
    with open(filename) as f:
        for line in f:
            parts = line.strip().split(',')
            if len(parts) == 2:
                scores.append(float(parts[1]))
    print(f"Loaded {len(scores)} scores from {filename}", file=sys.stderr)
    return scores

if len(sys.argv) != 3:
    print("Usage: compute_metrics.py benign_scores.csv dga_scores.csv")
    sys.exit(1)

benign = load_scores(sys.argv[1])
dga = load_scores(sys.argv[2])

thresholds = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9]

print("Threshold,Benign_Blocked,Benign_Total,FPR(%),DGA_Blocked,DGA_Total,DR(%)")
for th in thresholds:
    ben_blocked = sum(1 for s in benign if s > th)
    fpr = (ben_blocked / len(benign)) * 100 if benign else 0
    dga_blocked = sum(1 for s in dga if s > th)
    dr = (dga_blocked / len(dga)) * 100 if dga else 0
    print(f"{th},{ben_blocked},{len(benign)},{fpr:.2f},{dga_blocked},{len(dga)},{dr:.2f}")
