#!/bin/bash
# Table 5.3 – DGA detection by family (UMUDGA, 100 domains/family)
# Assumes dga_score.py and UMUDGA directory structure

cd ~/new_dns/dns-firewall/evaluation

BASE="UMUDGA/UMUDGA - University of Murcia Domain Generation Algorithm Dataset/Fully Qualified Domain Names"

echo "Familie | Detectate la 0.4 | Categorie"
TP_TOTAL=0

for family_dir in "$BASE"/*/; do
    family_name=$(basename "$family_dir")
    [ "$family_name" = "legit" ] && continue
    list_file="$family_dir/list/1000.txt"
    
    if [ ! -f "$list_file" ]; then
        echo "WARNING: $list_file not found, skipping $family_name"
        continue
    fi
    
    head -100 "$list_file" | python3 dga_score.py > "/tmp/${family_name}_scores.csv"
    tp=$(awk -F, '$2>0.4 {c++} END{print c+0}' "/tmp/${family_name}_scores.csv")
    
    if [ "$tp" -ge 80 ]; then
        cat="Ridicata"
    elif [ "$tp" -ge 1 ]; then
        cat="Medie"
    else
        cat="Zero"
    fi
    
    printf "%-20s %3d  %s\n" "$family_name" $tp "$cat"
    TP_TOTAL=$((TP_TOTAL + tp))
done

echo ""
echo "Total TP (familii): $TP_TOTAL"
