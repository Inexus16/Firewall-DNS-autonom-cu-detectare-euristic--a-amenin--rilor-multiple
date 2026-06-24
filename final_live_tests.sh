#!/bin/bash
# final_live_tests.sh – run from ~/dns-firewall
sudo pkill dns-firewall;sudo pkill -f auth_fastflux.py; sudo pkill -f auth_tunnel.py;sudo pkill -f replay_auth.py
sudo fuser -k 80/tcp 2>/dev/null
sleep 2

# Start supporting servers
cd evaluation
python3 gen_signed_response.py 2>/dev/null
cp trusted_key.pem ../trusted_key.pem
python3 replay_auth.py &
python3 auth_tunnel.py &
python3 auth_fastflux.py &
sleep 1
cd ..

# Start firewall
sudo DNS_FORWARDER=8.8.8.8 DNS_FORWARDER_TCP=1 DGA_THRESHOLD=0.4 ./dns-firewall &
sleep 4

echo "=== LIVE TEST RESULTS ==="

echo "1. UDP resolution:"
dig @127.0.0.1 +time=5 google.com A +short
echo ""
sleep 2
echo "2. Blacklist:"
dig @127.0.0.1 +time=5 malware.test A +short
echo ""
sleep 2
echo "3. DGA live:"
dig @127.0.0.1 +time=5 xvzqktrmnpwjhfsdabcdef.com A +short
echo ""
sleep 2
echo "4. Tunnelling:"
dig @127.0.0.1 +time=10 TXT $(python3 -c "print('A'*60)").tunnel.test +short
echo ""
sleep 2
echo "5. DNSSEC:"
dig @127.0.0.1 +time=10 www.signed.test A +short
echo ""
sleep 2
echo "6. Sinkhole HTTP:"
curl -s http://127.0.0.1/ | head -1
echo ""
sleep 2
echo "7. DoH POST:"
python3 -c "
import sys
q = b'\x00\x01\x01\x00\x00\x01\x00\x00\x00\x00\x00\x00\x06google\x03com\x00\x00\x01\x00\x01'
sys.stdout.buffer.write(q)
" | curl -s -X POST -H "Content-Type: application/dns-message" --data-binary @- http://127.0.0.1:8053/dns-query | \
python3 -c "import sys; d=sys.stdin.buffer.read(); print('DoH POST resolved:', '.'.join(str(b) for b in d[-4:]) if len(d)>40 else 'FAILED')"
echo ""
sleep 2
echo "8. DoH GET (dns=):"
curl -s "http://127.0.0.1:8053/dns-query?dns=AAEBAAABAAAAAAAABmdvb2dsZQNjb20AAAEAAQ" | \
python3 -c "import sys; d=sys.stdin.buffer.read(); print('DoH GET dns= resolved:', '.'.join(str(b) for b in d[-4:]) if len(d)>40 else 'FAILED')"
echo ""
sleep 2
echo "9. DoH GET (name=):"
curl -s --max-time 15 "http://127.0.0.1:8053/dns-query?name=google.com&type=A" | \
python3 -c "import sys; d=sys.stdin.buffer.read(); print('DoH GET name= resolved:', '.'.join(str(b) for b in d[-4:]) if len(d)>40 else 'FAILED')"
echo ""
sleep 2
echo "10. RPZ export:"
sudo kill -HUP $(pgrep dns-firewall)
sleep 1
head -8 rpz.zone
echo ""
sleep 2
echo "11. Reputation:"
cat reputation.csv 2>/dev/null
echo ""
sleep 2
echo "12. Fast-flux (production threshold=500):"
for i in $(seq 1 6); do
    sleep 1.5
    echo -n "Query $i: "
    dig @127.0.0.1 +time=5 fastflux.test A +short 2>/dev/null
done
#echo "(No ALERT expected with threshold 500)"
echo ""
sleep 2
# Cleanup
sudo pkill dns-firewall;sudo pkill -f auth_fastflux.py; sudo pkill -f auth_tunnel.py;sudo pkill -f replay_auth.py
sudo fuser -k 80/tcp 2>/dev/null
