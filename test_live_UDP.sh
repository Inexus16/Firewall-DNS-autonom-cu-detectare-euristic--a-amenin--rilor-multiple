#!/bin/bash
# test_live_recursive.sh – All 11 live tests in recursive mode, no forwarder

echo "Cleaning up previous instances"
sudo pkill dns-firewall 2>/dev/null
pkill -f auth_fastflux.py 2>/dev/null
pkill -f auth_tunnel.py 2>/dev/null
pkill -f replay_auth.py 2>/dev/null
sudo fuser -k 80/tcp 2>/dev/null
sleep 2

echo "Starting servers..."
cd ~/new_dns/dns-firewall/evaluation
python3 auth_fastflux.py &
python3 auth_tunnel.py &
python3 replay_auth.py &
sleep 1
cd ~/new_dns/dns-firewall

echo "[*] Starting firewall in recursive mode (DGA_THRESHOLD=0.4, FLUX_IP_MAX=3)"
sudo ./dns-firewall &
sleep 3   # load root hints 

echo ""
echo " LIVE TEST RESULTS "
echo ""

echo "1. UDP resolution:"
dig @127.0.0.1 +time=5 google.com A +short
sleep 1

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
sleep 2

echo ""
echo "5. DNSSEC:"
dig @127.0.0.1 +time=10 www.signed.test A +short
sleep 2

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
python3 -c "import sys; d=sys.stdin.buffer.read(); print('DoH POST resolved:', '.'.join(str(b) for b in d[-4:]) if len(d)>40 else 'FAILED')"

echo ""
echo "8. DoH GET (dns=):"
curl -s "http://127.0.0.1:8053/dns-query?dns=AAEBAAABAAAAAAAABmdvb2dsZQNjb20AAAEAAQ" | \
python3 -c "import sys; d=sys.stdin.buffer.read(); print('DoH GET dns= resolved:', '.'.join(str(b) for b in d[-4:]) if len(d)>40 else 'FAILED')"

echo ""
echo "9. DoH GET (name=):"
curl -s --max-time 15 "http://127.0.0.1:8053/dns-query?name=google.com&type=A" | \
python3 -c "import sys; d=sys.stdin.buffer.read(); print('DoH GET name= resolved:', '.'.join(str(b) for b in d[-4:]) if len(d)>40 else 'FAILED')"

echo ""
echo "10. RPZ export:"
sudo kill -HUP $(pgrep dns-firewall)
sleep 1
head -8 rpz.zone

echo ""
echo "11. Reputation:"
cat reputation.csv 2>/dev/null

echo ""
echo "12. Fast-flux:"
for i in $(seq 1 6); do
    sleep 1.5
    echo -n "Query $i: "
    dig @127.0.0.1 +time=5 fastflux.test A +short 2>/dev/null
done


echo ""
echo "[*] Cleaning up..."
sudo pkill dns-firewall
pkill -f auth_fastflux.py
pkill -f auth_tunnel.py
pkill -f replay_auth.py
sudo fuser -k 80/tcp 2>/dev/null
echo "All tests completed."
