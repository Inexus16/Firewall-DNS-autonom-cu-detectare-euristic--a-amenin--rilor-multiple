#!/bin/bash
# Table 5.4 – Low-length threshold experiment (6‑feature version)
# Compares ENHANCED scorer vs ENHANCED+LOWLEN scorer

cd ~/new_dns/dns-firewall/evaluation

echo "Generating scores..."

# Enhanced scorer (standard 6 features)
python3 dga_score.py < dga_umudga_5k.txt > dga_scores_enhanced.csv
python3 dga_score.py < benign_10k_sample.txt > benign_scores_enhanced.csv

# Enhanced + lowlen scorer (6 features with relaxed length)
python3 dga_score_lowlen_6.py < dga_umudga_5k.txt > dga_scores_lowlen_6.csv
python3 dga_score_lowlen_6.py < benign_10k_sample.txt > benign_scores_lowlen_6.csv

echo ""
echo "Prag | DR enhanced | DR lowlen | FPR enhanced | FPR lowlen"
for TH in 0.3 0.4 0.48 0.5 0.6; do
    TP_enh=$(awk -F, -v t=$TH '$2>t {c++} END{print c+0}' dga_scores_enhanced.csv)
    TP_low=$(awk -F, -v t=$TH '$2>t {c++} END{print c+0}' dga_scores_lowlen_6.csv)
    FP_enh=$(awk -F, -v t=$TH '$2>t {c++} END{print c+0}' benign_scores_enhanced.csv)
    FP_low=$(awk -F, -v t=$TH '$2>t {c++} END{print c+0}' benign_scores_lowlen_6.csv)
    
    DR_enh=$(echo "scale=4; $TP_enh/5000*100" | bc)
    DR_low=$(echo "scale=4; $TP_low/5000*100" | bc)
    FPR_enh=$(echo "scale=4; $FP_enh/10000*100" | bc)
    FPR_low=$(echo "scale=4; $FP_low/10000*100" | bc)
    
    printf "%.2f  | %.2f%%       | %.2f%%     | %.2f%%        | %.2f%%\n" $TH $DR_enh $DR_low $FPR_enh $FPR_low
done
