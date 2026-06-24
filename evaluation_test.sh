#!/bin/bash
# evaluation_test.sh – Cele 10 teste live, mod recursiv (fără forwarder)
#use this for eval 
# curatenie
sudo pkill dns-firewall 2>/dev/null
sudo pkill -f auth_fastflux.py 2>/dev/null
sudo pkill -f auth_tunnel.py 2>/dev/null
sudo pkill -f replay_auth.py 2>/dev/null
sudo fuser -k 80/tcp 2>/dev/null
sleep 2

# Pornire servere auxiliare
cd evaluation
python3 gen_signed_response.py 2>/dev/null
cp trusted_key.pem ../trusted_key.pem
python3 replay_auth.py &
python3 auth_tunnel.py &
python3 auth_fastflux.py &
sleep 1
cd ..

# Pornire firewall – mod recursiv, FLUX_IP_MAX=3 pentru a vedea alerta
sudo DGA_THRESHOLD=0.48 FLUX_IP_MAX=3 ./dns-firewall &
sleep 4

echo "=================== LIVE TEST RESULTS ==================="

echo "1. UDP resolution for google.com:"
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
" | curl -s -X POST -H "Content-Type: application/dns-message" --data-binary @- http://127.0.0.1:8053/dns-query 2>/dev/null | \
python3 -c "import sys; d=sys.stdin.buffer.read(); print('DoH POST resolved:', '.'.join(str(b) for b in d[-4:]) if len(d)>40 else 'FAILED')"
echo ""
echo "   DoH GET (dns=):"
curl -s "http://127.0.0.1:8053/dns-query?dns=AAEBAAABAAAAAAAABmdvb2dsZQNjb20AAAEAAQ" 2>/dev/null | \
python3 -c "import sys; d=sys.stdin.buffer.read(); print('DoH GET dns= resolved:', '.'.join(str(b) for b in d[-4:]) if len(d)>40 else 'FAILED')"
echo ""
echo "   DoH GET (name=):"
curl -s --max-time 15 "http://127.0.0.1:8053/dns-query?name=google.com&type=A" 2>/dev/null | \
python3 -c "import sys; d=sys.stdin.buffer.read(); print('DoH GET name= resolved:', '.'.join(str(b) for b in d[-4:]) if len(d)>40 else 'FAILED')"
echo ""
sleep 2

echo "8. Fast-flux (threshold=3, ALERT expected):"
for i in $(seq 1 6); do
    sleep 1.5
    echo -n "Query $i: "
    dig @127.0.0.1 +time=5 fastflux.test A +short 2>/dev/null
done
echo ""
sleep 2

echo "Setting up veth pair for rate limiting and quarantine tests..."
sudo ip netns add testns 2>/dev/null
sudo ip link add veth0 type veth peer name veth1 2>/dev/null
sudo ip link set veth1 netns testns
sudo ip addr add 10.0.0.1/24 dev veth0
sudo ip link set veth0 up
sudo ip netns exec testns ip addr add 10.0.0.2/24 dev veth1
sudo ip netns exec testns ip link set veth1 up
sleep 1

echo "9. Rate limiting:"
sudo ip netns exec testns bash -c '
for i in $(seq 1 25); do
    result=$(dig @10.0.0.1 +time=3 +short google.com A 2>&1)
    echo "Query $i: $result"
done
'
echo ""
sleep 2

echo "10. Client quarantine:"
sudo ip netns exec testns bash -c '
for i in $(seq 1 10); do
    dig @10.0.0.1 +time=3 +short malware.test A 2>/dev/null
    sleep 0.1
done
'
echo "   Legitimate query after quarantine (should return 0.0.0.0):"
sudo ip netns exec testns dig @10.0.0.1 +time=3 google.com A +short 2>&1
echo ""
sleep 2

# curatenie
sudo ip netns delete testns 2>/dev/null
sudo ip link delete veth0 2>/dev/null
sudo pkill dns-firewall
sudo pkill -f auth_fastflux.py
sudo pkill -f auth_tunnel.py
sudo pkill -f replay_auth.py
sudo fuser -k 80/tcp 2>/dev/null

echo "=============== ALL 10 TESTS COMPLETED ==============="
