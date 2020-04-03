#!/bin/bash
TEST_SCRIPT_DIR="`dirname ${BASH_SOURCE[0]}`"
echo "Ping from PC1 to PC2"
ip netns exec pc1 ping 192.168.1.2 -c 4
echo "Ping from PC2 to PC1"
ip netns exec pc2 ping 192.168.0.2 -c 4
echo "Test download speed"
dd if=/dev/urandom of=$TEST_SCRIPT_DIR/data.tmp bs=1024 count=100000
cd $TEST_SCRIPT_DIR && ip netns exec pc1 python3 -m http.server > /dev/null 2>&1 &
sleep 2s
ip netns exec pc2 wget http://192.168.0.2:8000/data.tmp -O /dev/null
