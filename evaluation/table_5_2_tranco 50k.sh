#!/bin/bash
# Table 5.2 – FPR on Tranco 50k (ordered by popularity)
# Assumes dga_score.py, benign_tranco.csv exists

cd ~/new_dns/dns-firewall/evaluation

echo "Generating scores for Tranco 50k..."
python3 dga_score.py < benign_tranco.csv > benign_scores_50k.csv

echo ""
echo "Prag | Benigne blocate | FPR (%)"
for TH in 0.3 0.4 0.5; do
    FP=$(awk -F, -v t=$TH '$2>t {c++} END{print c+0}' benign_scores_50k.csv)
    TOTAL=$(wc -l < benign_scores_50k.csv)
    FPR=$(echo "scale=4; $FP/$TOTAL*100" | bc)
    echo "$TH  | $FP            | $FPR"
done
