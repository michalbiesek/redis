#!/bin/sh

mkdir -p /mnt/pmem
sudo mount -o dax /dev/pmem0 /mnt/pmem
export MEMKIND_HOG_MEMORY=1 && nohup ./src/redis-server redis.conf >server.log 2>&1 &
