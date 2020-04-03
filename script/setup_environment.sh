#!/bin/bash
ip netns add pc1
ip netns add pc2
ip netns add r1
ip netns add r2
ip netns add r3
# r1 r2
ip link add eth1 type veth peer name veth1
ip link set eth1 netns r2
ip link set veth1 netns r1
ip netns exec r2 ip addr add 10.0.0.1/24 dev eth1 && ip netns exec r2 ip link set eth1 up
ip netns exec r1 ip addr add 10.0.0.2/24 dev veth1 && sudo ip netns exec r1 ip link set veth1 up
#r1 pc1
ip link add veth3 type veth peer name veth4
ip link set veth3 netns r1
ip link set veth4 netns pc1
ip netns exec r1 ip addr add 192.168.0.1/24 dev veth3 && ip netns exec r1 ip link set veth3 up
ip netns exec pc1 ip addr add 192.168.0.2/24 dev veth4 && sudo ip netns exec pc1 ip link set veth4 up
#pc1
ip netns exec pc1 route add default gw 192.168.0.1
#r2 r3
ip link add eth2 type veth peer name veth2
ip link set eth2 netns r2
ip link set veth2 netns r3
ip netns exec r2 ip addr add 10.0.1.1/24 dev eth2 && ip netns exec r2 ip link set eth2 up
ip netns exec r3 ip addr add 10.0.1.2/24 dev veth2 && sudo ip netns exec r3 ip link set veth2 up
#r3 pc2
ip link add veth5 type veth peer name veth6
ip link set veth5 netns r3
ip link set veth6 netns pc2
ip netns exec r3 ip addr add 192.168.1.1/24 dev veth5 && ip netns exec r3 ip link set veth5 up
ip netns exec pc2 ip addr add 192.168.1.2/24 dev veth6 && sudo ip netns exec pc2 ip link set veth6 up
#pc2
ip netns exec pc2 route add default gw 192.168.1.1

ip netns exec pc1 ethtool -K veth4 tx off rx off
ip netns exec r1  ethtool -K veth3 tx off rx off
ip netns exec r1  ethtool -K veth1 tx off rx off
ip netns exec r2  ethtool -K eth1  tx off rx off
ip netns exec r2  ethtool -K eth2  tx off rx off
ip netns exec r3  ethtool -K veth2 tx off rx off
ip netns exec r3  ethtool -K veth3 tx off rx off
ip netns exec pc2 ethtool -K veth6 tx off rx off
