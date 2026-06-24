Install the required packages:
sudo apt update
sudo apt install -y build-essential gcc make libssl-dev python3 python3-pip dnsperf
pip3 install cryptography
Optional for performance test
sudo apt install -y dnsperf curl bind9-dnsutils

System preparation
Stop systemd‑resolved so port 53 is free:
sudo systemctl stop systemd-resolved
sudo systemctl disable systemd-resolved

Allow UDP outbound (if your network blocks UDP 53, skip this; the firewall can use TCP forwarder instead):
sudo iptables -F
sudo iptables -P OUTPUT ACCEPT

Increase file descriptors and buffers (for high‑performance tests):
ulimit -n 65535
sudo sysctl -w net.ipv4.ip_local_port_range="1024 65535"
sudo sysctl -w net.core.rmem_max=8388608


Build the firewall
cd dns-firewall
make clean && make


Run the firewall
Recursive mode (no external forwarder, uses root hints):
sudo DGA_THRESHOLD=0.48 ./dns-firewall &
Forwarder mode (use 8.8.8.8 via TCP, good if UDP is blocked):
sudo DNS_FORWARDER=8.8.8.8 DNS_FORWARDER_TCP=1 DGA_THRESHOLD=0.48 ./dns-firewall &
Fast‑flux test mode (low threshold to trigger alerts):
sudo FLUX_IP_MAX=3 ./dns-firewall &
All options can be combined. Example for a quick live demonstration:
sudo DNS_FORWARDER=8.8.8.8 DNS_FORWARDER_TCP=1 DGA_THRESHOLD=0.48 FLUX_IP_MAX=3 ./dns-firewall &

Run the automated live tests
cd dns-firewall
./evaluation_test.sh


Run the offline evaluation (generate tables)
cd evaluation
./table_5_1_variatia_pragurilor.sh
./table_5_2_tranco_50k.sh
./table_5_3_analiza_familii.sh
./table_5_4_prag_de_lungime.sh    
./table_5_5_wilson.sh

# Performance test (requires dnsperf)
cd ..
sudo ./table_5_8_performanta.sh   # adjust if necessary
