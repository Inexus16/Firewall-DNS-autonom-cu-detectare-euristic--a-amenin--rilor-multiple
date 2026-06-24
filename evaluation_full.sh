#!/bin/bash
# reproduce_evaluation.sh – reproduce exact results for Thesis Chapter 5
# Run from ~/new_dns/dns-firewall/evaluation
# Assumes the following files exist:
#   benign_10k_sample.txt, dga_umudga_5k.txt, benign_tranco.csv (50k ordered),
#   UMUDGA structure, dga_score.py, dga_score_lowlen.py

set -e
cd ~/new_dns/dns-firewall/evaluation

echo "=== 1. Variația pragurilor (Tabel 5.1) ==="
# Generate scores if not already present
if [ ! -f benign_scores_10k.csv ]; then
    python3 dga_score.py < benign_10k_sample.txt > benign_scores_10k.csv
fi
if [ ! -f dga_scores_5k.csv ]; then
    python3 dga_score.py < dga_umudga_5k.txt > dga_scores_5k.csv
fi

echo "Prag | Benigne blocate (FP) | DGA blocate (TP) | FPR (%) | DR (%)"
for TH in 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9; do
    FP=$(awk -F, -v t=$TH '$2>t {c++} END{print c+0}' benign_scores_10k.csv)
    TP=$(awk -F, -v t=$TH '$2>t {c++} END{print c+0}' dga_scores_5k.csv)
    FPR=$(echo "scale=4; $FP/10000*100" | bc)
    DR=$(echo "scale=4; $TP/5000*100" | bc)
    printf "%.1f  | %d            | %d          | %.2f   | %.2f\n" $TH $FP $TP $FPR $DR
done
echo ""

echo "=== 2. FPR pe setul Tranco 50k ordonat (Tabel 5.2) ==="
# Generate scores for 50k ordered Tranco if needed
if [ ! -f benign_scores_50k.csv ]; then
    # Assumes benign_tranco.csv contains first 50000 domains (ordered by popularity)
    python3 dga_score.py < benign_tranco.csv > benign_scores_50k.csv
fi
echo "Prag | Benigne blocate | FPR (%)"
for TH in 0.3 0.4 0.5; do
    FP=$(awk -F, -v t=$TH '$2>t {c++} END{print c+0}' benign_scores_50k.csv)
    TOTAL=$(wc -l < benign_scores_50k.csv)
    FPR=$(echo "scale=4; $FP/$TOTAL*100" | bc)
    echo "$TH  | $FP            | $FPR"
done
echo ""

echo "=== 3. Analiza pe familii (Tabel 5.3) ==="
echo "Familie | Detectate la 0.4 | Categorie (indicativ)"
TP_TOTAL=0
for family_dir in "UMUDGA/UMUDGA - University of Murcia Domain Generation Algorithm Dataset/Fully Qualified Domain Names"/*/; do
    family_name=$(basename "$family_dir")
    list_file="$family_dir/list/1000.txt"
    if [ -f "$list_file" ]; then
        head -100 "$list_file" | python3 dga_score.py > "/tmp/${family_name}_scores.csv"
        tp=$(awk -F, '$2>0.4 {c++} END{print c+0}' "/tmp/${family_name}_scores.csv")
        # Assign category (adjust as needed)
        if [ "$tp" -ge 80 ]; then cat="Ridicata"
        elif [ "$tp" -ge 1 ]; then cat="Medie"
        else cat="Zero"
        fi
        printf "%-20s %3d  %s\n" "$family_name" $tp "$cat"
        TP_TOTAL=$((TP_TOTAL + tp))
    fi
done
echo "Total TP (familii): $TP_TOTAL"
echo ""

echo "=== 4. Experimentul pragului de lungime (Tabel 5.4) ==="
# Lowlen scores
python3 dga_score_lowlen.py < dga_umudga_5k.txt > dga_scores_lowlen.csv
python3 dga_score_lowlen.py < benign_10k_sample.txt > benign_scores_lowlen.csv

echo "Prag | DR originală | DR lowlen | FPR originală | FPR lowlen"
for TH in 0.3 0.4; do
    TP_orig=$(awk -F, -v t=$TH '$2>t {c++} END{print c+0}' dga_scores_5k.csv)
    TP_low=$(awk -F, -v t=$TH '$2>t {c++} END{print c+0}' dga_scores_lowlen.csv)
    FP_orig=$(awk -F, -v t=$TH '$2>t {c++} END{print c+0}' benign_scores_10k.csv)
    FP_low=$(awk -F, -v t=$TH '$2>t {c++} END{print c+0}' benign_scores_lowlen.csv)
    DR_orig=$(echo "scale=4; $TP_orig/5000*100" | bc)
    DR_low=$(echo "scale=4; $TP_low/5000*100" | bc)
    FPR_orig=$(echo "scale=4; $FP_orig/10000*100" | bc)
    FPR_low=$(echo "scale=4; $FP_low/10000*100" | bc)
    printf "%.1f  | %.2f%%       | %.2f%%     | %.2f%%        | %.2f%%\n" $TH $DR_orig $DR_low $FPR_orig $FPR_low
done
echo ""

echo "=== 5. Intervale de încredere Wilson (Tabel 5.5) ==="
python3 - <<EOF
import math
def wilson_interval(successes, trials, z=1.96):
    p = successes / trials
    denom = 1 + z**2 / trials
    centre = (p + z**2 / (2*trials)) / denom
    margin = z * math.sqrt(p*(1-p)/trials + z**2/(4*trials**2)) / denom
    return max(0, centre - margin), min(1, centre + margin)

# FPR: 26 false positives, 10000 benign
fpr_low, fpr_high = wilson_interval(26, 10000)
print(f"FPR: 0.26%  (95% CI: {fpr_low*100:.2f}% - {fpr_high*100:.2f}%)")

# DR: 702 true positives, 5000 DGA
dr_low, dr_high = wilson_interval(702, 5000)
print(f"DR: 14.04% (95% CI: {dr_low*100:.2f}% - {dr_high*100:.2f}%)")
EOF
echo ""

echo "=== 6. Performanță (Tabel 5.8) - necesită firewall pornit ==="
echo "Pentru reproducere exactă, rulează manual:"
echo "  # Pornire firewall:"
echo "  sudo DNS_FORWARDER=8.8.8.8 DNS_FORWARDER_TCP=1 DGA_THRESHOLD=0.4 FLUX_IP_MAX=500 ./dns-firewall &"
echo "  sleep 3"
echo "  # Teste dnsperf:"
echo "  dnsperf -s 127.0.0.1 -d evaluation/legit_200.txt -c 8 -Q 50 -l 30 -t 10"
echo "  dnsperf -s 127.0.0.1 -d evaluation/legit_200.txt -c 8 -Q 100 -l 30 -t 10"
echo "  dnsperf -s 127.0.0.1 -d evaluation/legit_200.txt -c 8 -Q 200 -l 30 -t 10"
echo "  dnsperf -s 127.0.0.1 -d evaluation/legit_200.txt -c 8 -Q 500 -l 30 -t 10"
echo "  # Cache cald:"
echo "  dnsperf -s 127.0.0.1 -d evaluation/legit_200.txt -c 1 -Q 10 -l 10 -t 5   # preîncălzire"
echo "  dnsperf -s 127.0.0.1 -d evaluation/legit_200.txt -c 8 -Q 2000 -l 20 -t 5"
echo "Vezi output-ul fiecărui dnsperf pentru Queries per second, pierderi, latenta."
