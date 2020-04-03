#!/bin/bash
TEST_SCRIPT_DIR="`dirname ${BASH_SOURCE[0]}`"
echo "Set up environment..."
bash $TEST_SCRIPT_DIR/setup_environment.sh
#start router
ip netns exec r1 sysctl -w net.ipv4.ip_forward=1
ip netns exec r2 sysctl -w net.ipv4.ip_forward=0
ip netns exec r3 sysctl -w net.ipv4.ip_forward=1
ip netns exec r2 sysctl -w net.ipv4.icmp_echo_ignore_all=1
ip netns exec r1 bird -c $TEST_SCRIPT_DIR/r1.conf -s /dev/ttyS90
ip netns exec r2 /home/wcz/router/router-lab/Homework/boilerplate/boilerplate > $TEST_SCRIPT_DIR/r2.log 2>&1 &
ip netns exec r3 bird -c  $TEST_SCRIPT_DIR/r3.conf -s /dev/ttyS92
sleep 20s
bash $TEST_SCRIPT_DIR/do_test.sh
bash $TEST_SCRIPT_DIR/kill.sh
