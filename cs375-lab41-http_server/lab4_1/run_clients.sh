#!/bin/bash
PORT=${1:-8080}
COUNT=${2:-10}
for i in $(seq 1 $COUNT); do
    python3 slow_client.py $PORT $i &
done
wait
