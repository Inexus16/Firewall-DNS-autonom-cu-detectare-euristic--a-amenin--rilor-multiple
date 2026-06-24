#!/bin/bash
# Table 5.1 – DGA threshold variation
# Assumes dga_score.py, benign_10k_sample.txt, dga_umudga_5k.txt exist

cd ~/new_dns/dns-firewall/evaluation

echo "Generating scores..."
python3 dga_score.py < benign_10k_sample.txt > benign_scores_10k.csv
python3 dga_score.py < dga_umudga_5k.txt > dga_scores_5k.csv

echo ""
echo "Prag | Benigne blocate (FP) | DGA blocate (TP) | FPR (%) | DR (%)"
for TH in 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9; do
    FP=$(awk -F, -v t=$TH '$2>t {c++} END{print c+0}' benign_scores_10k.csv)
    TP=$(awk -F, -v t=$TH '$2>t {c++} END{print c+0}' dga_scores_5k.csv)
    FPR=$(echo "scale=4; $FP/10000*100" | bc)
    DR=$(echo "scale=4; $TP/5000*100" | bc)
    printf "%.1f  | %d            | %d          | %.2f   | %.2f\n" $TH $FP $TP $FPR $DR
done
