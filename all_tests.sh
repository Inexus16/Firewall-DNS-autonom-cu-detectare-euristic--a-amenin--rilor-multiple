#!/bin/bash
cd /home/kali/new_dns/dns-firewall

# ── Curățenie totală ──────────────────────────────────────
echo "[*] Cleaning up previous processes..."
sudo pkill dns-firewall 2>/dev/null || true
sudo pkill -f auth_fastflux.py 2>/dev/null || true
sudo pkill -f auth_tunnel.py 2>/dev/null || true
sudo pkill -f replay_auth.py 2>/dev/null || true
sudo fuser -k 80/tcp 8053/tcp 2>/dev/null || true
sudo systemctl stop systemd-resolved 2>/dev/null || true
sleep 2

# ── Pornește serverele auxiliare ───────────────────────────
cd evaluation
python3 gen_signed_response.py 2>/dev/null
cp trusted_key.pem ../trusted_key.pem
python3 replay_auth.py &
python3 auth_tunnel.py &
python3 auth_fastflux.py &
sleep 2
cd ..

# ── Pornește firewall-ul (fără debug) ─────────────────────
sudo ./dns-firewall
sleep 4

echo ""
echo "==================== LIVE TESTS ===================="

echo "1. UDP resolution:"
dig @127.0.0.1 +time=5 google.com A +short
sleep 2

echo ""
echo "2. Blacklist:"
dig @127.0.0.1 +time=5 malware.test A +short
sleep 1

echo ""
echo "3. DGA live:"
dig @127.0.0.1 +time=5 xvzqktrmnpwjhfsdabcdef.com A +short
sleep 1

echo ""
echo "4. Tunnelling:"
dig @127.0.0.1 +time=10 TXT $(python3 -c "print('A'*60)").tunnel.test +short
sleep 3

echo ""
echo "5. DNSSEC:"
dig @127.0.0.1 +time=10 www.signed.test A +short
sleep 3

echo ""
echo "6. Sinkhole HTTP:"
curl -s http://127.0.0.1/ | head -1
sleep 1

echo ""
echo "7. DoH POST:"
python3 -c "
import sys
q = b'\x00\x01\x01\x00\x00\x01\x00\x00\x00\x00\x00\x00\x06google\x03com\x00\x00\x01\x00\x01'
sys.stdout.buffer.write(q)
" | curl -s -X POST -H "Content-Type: application/dns-message" --data-binary @- http://127.0.0.1:8053/dns-query | \
python3 -c "import sys; d=sys.stdin.buffer.read(); print('IP:', '.'.join(str(b) for b in d[-4:]) if len(d)>40 else 'FAILED')"

echo ""
echo "8. DoH GET (dns=):"
curl -s "http://127.0.0.1:8053/dns-query?dns=AAEBAAABAAAAAAAABmdvb2dsZQNjb20AAAEAAQ" | \
python3 -c "import sys; d=sys.stdin.buffer.read(); print('IP:', '.'.join(str(b) for b in d[-4:]) if len(d)>40 else 'FAILED')"

echo ""
echo "9. DoH GET (name=):"
curl -s --max-time 15 "http://127.0.0.1:8053/dns-query?name=google.com&type=A" | \
python3 -c "import sys; d=sys.stdin.buffer.read(); print('IP:', '.'.join(str(b) for b in d[-4:]) if len(d)>40 else 'FAILED')"

echo ""
echo "10. RPZ export:"
sudo kill -HUP $(pgrep dns-firewall)
sleep 1
head -8 rpz.zone

echo ""
echo "11. Reputation:"
cat reputation.csv 2>/dev/null

echo ""
echo "12. Fast-flux (production threshold=500):"
for i in $(seq 1 6); do
    sleep 1.5
    echo -n "Query $i: "
    dig @127.0.0.1 +time=5 fastflux.test A +short 2>/dev/null
done
echo "(No ALERT expected with threshold 500)"

echo ""
echo "================= RATE LIMIT & QUARANTINE TESTS ================="
# Pregătește interfața virtuală
sudo ip netns add testns 2>/dev/null || true
sudo ip link add veth0 type veth peer name veth1 2>/dev/null || true
sudo ip link set veth1 netns testns 2>/dev/null || true
sudo ip addr add 10.0.0.1/24 dev veth0 2>/dev/null || true
sudo ip link set veth0 up 2>/dev/null || true
sudo ip netns exec testns ip addr add 10.0.0.2/24 dev veth1 2>/dev/null || true
sudo ip netns exec testns ip link set veth1 up 2>/dev/null || true

echo ""
echo "13. Rate limiting (client 10.0.0.2):"
sudo ip netns exec testns bash -c '
for i in $(seq 1 25); do
    result=$(dig @10.0.0.1 +time=3 +short google.com A 2>&1)
    echo "Query $i: $result"
done'

echo ""
echo "14. Client quarantine:"
sudo ip netns exec testns bash -c '
for i in $(seq 1 10); do
    dig @10.0.0.1 +time=3 +short malware.test A 2>/dev/null
    sleep 0.1
done'
echo "--- Legitimate query after quarantine (should return 0.0.0.0) ---"
sudo ip netns exec testns dig @10.0.0.1 +time=3 google.com A +short


sudo ip netns delete testns 2>/dev/null || true
sudo ip link delete veth0 2>/dev/null || true
sudo pkill dns-firewall 2>/dev/null || true
sudo pkill -f auth_fastflux.py 2>/dev/null || true
sudo pkill -f auth_tunnel.py 2>/dev/null || true
sudo pkill -f replay_auth.py 2>/dev/null || true
sudo fuser -k 80/tcp 8053/tcp 2>/dev/null || true

echo ""
echo "================= ALL TESTS COMPLETE ================="
