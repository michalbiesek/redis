#!/bin/sh

while :
do
date
echo "size: 4096"
./src/redis-benchmark -h 127.0.0.1 -p 6379 -t set -n 50000000 -r 1000000000 -d 4096 -u 4096 -c 8 -e -q >benchmark.log
date
echo "size: 64"
./src/redis-benchmark -h 127.0.0.1 -p 6379 -t set -n 50000000 -r 1000000000 -d 64 -u 64 -c 8 -e -q >benchmark.log
date
echo "size: 64,4096"
./src/redis-benchmark -h 127.0.0.1 -p 6379 -t set -n 50000000 -r 1000000000 -d 64 -u 4096 -c 8 -e -q >benchmark.log
date
echo "size: 1024,2048"
./src/redis-benchmark -h 127.0.0.1 -p 6379 -t set -n 50000000 -r 1000000000 -d 1024 -u 2048 -c 8 -e -q >benchmark.log
done

