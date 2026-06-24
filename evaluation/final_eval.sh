#!/bin/bash
DNS="127.0.0.1"
RESULTS_FILE="evaluation_results.csv"
echo "domain,category,response_ip" > "$RESULTS_FILE"

# ---------- Benign domains ----------
BENIGN_LIST="top-1k.txt"
if [ ! -f "$BENIGN_LIST" ]; then
    echo "top-1k.txt not found, generating a short list."
    cat > top-1k.txt <<EOF
google.com
youtube.com
facebook.com
wikipedia.org
reddit.com
amazon.com
github.com
stackoverflow.com
netflix.com
microsoft.com
EOF
fi

echo "=== Benign queries ==="
FP_COUNT=0
TOTAL_BENIGN=0
while read -r domain; do
    [ -z "$domain" ] && continue
    TOTAL_BENIGN=$((TOTAL_BENIGN+1))
    ip=$(dig @$DNS "$domain" A +short +timeout=2 | head -1)
    if [ "$ip" = "0.0.0.0" ]; then
        FP_COUNT=$((FP_COUNT+1))
        echo "$domain -> BLOCKED (false positive)"
        echo "$domain,benign,0.0.0.0" >> "$RESULTS_FILE"
    elif [ -z "$ip" ]; then
        echo "$domain -> TIMEOUT"
        echo "$domain,benign,TIMEOUT" >> "$RESULTS_FILE"
    else
        echo "$domain -> $ip"
        echo "$domain,benign,$ip" >> "$RESULTS_FILE"
    fi
done < "$BENIGN_LIST"

# ---------- Blacklist ----------
echo "=== Blacklist queries ==="
BLACKLIST_FILE="../blacklist.txt"
BLOCKED_BL=0
TOTAL_BL=0
while read -r domain; do
    [ -z "$domain" ] && continue
    TOTAL_BL=$((TOTAL_BL+1))
    ip=$(dig @$DNS "$domain" A +short +timeout=1 | head -1)
    if [ "$ip" = "0.0.0.0" ]; then
        BLOCKED_BL=$((BLOCKED_BL+1))
        echo "$domain -> BLOCKED"
        echo "$domain,blacklist,0.0.0.0" >> "$RESULTS_FILE"
    else
        echo "$domain -> NOT BLOCKED (got $ip)"
        echo "$domain,blacklist,$ip" >> "$RESULTS_FILE"
    fi
done < "$BLACKLIST_FILE"

# ---------- DGA ----------
echo "=== DGA queries ==="
DGA_LIST=(
    "xh7k3j9d0f2a.xyz"
    "pz8m5n2v6q4w.net"
    "t1r9b7k3w5f8.info"
    "a2s4d6f8g0h1.com"
    "9i8u7y6t5r4e.xyz"
    "c3v6b9n2m5k8.net"
    "q1w2e3r4t5y6.info"
    "l0k9j8h7g6f5.com"
    "z8x7c6v5b4n3.xyz"
    "m2n3b4v5c6x7.net"
)
BLOCKED_DGA=0
TOTAL_DGA=0
for domain in "${DGA_LIST[@]}"; do
    TOTAL_DGA=$((TOTAL_DGA+1))
    ip=$(dig @$DNS "$domain" A +short +timeout=1 | head -1)
    if [ "$ip" = "0.0.0.0" ]; then
        BLOCKED_DGA=$((BLOCKED_DGA+1))
        echo "$domain -> BLOCKED"
        echo "$domain,dga,0.0.0.0" >> "$RESULTS_FILE"
    else
        echo "$domain -> NOT BLOCKED (got $ip)"
        echo "$domain,dga,$ip" >> "$RESULTS_FILE"
    fi
done

# ---------- Summary ----------
echo ""
echo "=== RESULTS ==="
echo "Benign:   $TOTAL_BENIGN domains, false positives = $FP_COUNT ($(awk "BEGIN {printf \"%.0f\", 100*$FP_COUNT/$TOTAL_BENIGN}")%)"
echo "Blacklist: $TOTAL_BL domains, blocked = $BLOCKED_BL ($(awk "BEGIN {printf \"%.0f\", 100*$BLOCKED_BL/$TOTAL_BL}")%)"
echo "DGA:      $TOTAL_DGA domains, blocked = $BLOCKED_DGA ($(awk "BEGIN {printf \"%.0f\", 100*$BLOCKED_DGA/$TOTAL_DGA}")%)"
echo "CSV saved to $RESULTS_FILE"
