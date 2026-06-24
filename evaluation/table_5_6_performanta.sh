#!/bin/bash
# Table 5.6 – Performance benchmarks (dnsperf)
# Requires: firewall binary, dnsperf installed, legit_200.txt

FIREWALL_DIR="/home/kali/new_dns/dns-firewall"
EVAL_DIR="$FIREWALL_DIR/evaluation"

cd "$FIREWALL_DIR"

# Generam legit_200.txt daca nu exista
if [ ! -f "$EVAL_DIR/legit_200.txt" ]; then
    echo "Creating legit_200.txt..."
    head -200 "$EVAL_DIR/benign_tranco.csv" | sed 's/$/ A/' > "$EVAL_DIR/legit_200.txt"
fi

# Opreste in caz de rulare
sudo pkill dns-firewall 2>/dev/null || true
sudo systemctl stop systemd-resolved 2>/dev/null || true
sleep 1

export DNS_FORWARDER=8.8.8.8
export DNS_FORWARDER_TCP=1
DGA_THRESHOLD=0.5 ./dns-firewall &
sleep 3

# Verifica daca firewall-ul merge
if ! pgrep -f dns-firewall > /dev/null; then
    echo "ERROR: Firewall failed to start!"
    exit 1
fi

echo ""
echo "  Cold Cache Tests"

echo ""
echo "50 QPS"
dnsperf -s 127.0.0.1 -d "$EVAL_DIR/legit_200.txt" -c 8 -Q 50 -l 30 -t 10 2>/dev/null | grep -E "(Queries sent|Queries completed|Queries lost|per second|Average Latency)"

echo ""
echo "100 QPS"
dnsperf -s 127.0.0.1 -d "$EVAL_DIR/legit_200.txt" -c 8 -Q 100 -l 30 -t 10 2>/dev/null | grep -E "(Queries sent|Queries completed|Queries lost|per second|Average Latency)"

echo ""
echo "200 QPS"
dnsperf -s 127.0.0.1 -d "$EVAL_DIR/legit_200.txt" -c 8 -Q 200 -l 30 -t 10 2>/dev/null | grep -E "(Queries sent|Queries completed|Queries lost|per second|Average Latency)"

echo ""
echo "500 QPS"
dnsperf -s 127.0.0.1 -d "$EVAL_DIR/legit_200.txt" -c 8 -Q 500 -l 30 -t 10 2>/dev/null | grep -E "(Queries sent|Queries completed|Queries lost|per second|Average Latency)"

echo ""
echo "  Warm Cache Test (2000 QPS)"
echo "Pre-warming cache..."
dnsperf -s 127.0.0.1 -d "$EVAL_DIR/legit_200.txt" -c 1 -Q 10 -l 10 -t 5 2>/dev/null > /dev/null
sleep 1

echo ""
echo "2000 QPS"
dnsperf -s 127.0.0.1 -d "$EVAL_DIR/legit_200.txt" -c 8 -Q 2000 -l 20 -t 5 2>/dev/null | grep -E "(Queries sent|Queries completed|Queries lost|per second|Average Latency)"

echo ""
echo "Cleaning"
sudo pkill dns-firewall 2>/dev/null || true
