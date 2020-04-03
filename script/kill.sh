#!/bin/bash
TEST_SCRIPT_DIR="`dirname ${BASH_SOURCE[0]}`"
pkill -f bird
pkill -f boilerplate
kill `pgrep -n -f python3`
rm $TEST_SCRIPT_DIR/data.tmp

ip netns delete pc1
ip netns delete pc2
ip netns delete r1
ip netns delete r2
ip netns delete r3
