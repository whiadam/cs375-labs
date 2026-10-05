#!/bin/bash
set -u

ROOT="$(cd "$(dirname "$0")" && pwd)"
CLIENTS=${CLIENTS:-10}
RUNS=${RUNS:-3}
WORK="$ROOT/experiment"
SRV_DIR="$WORK/server_files"
CLI_DIR="$WORK/client_files"
RESULTS="$WORK/results.txt"

make -C "$ROOT" -s || exit 1

rm -rf "$WORK"
mkdir -p "$SRV_DIR" "$CLI_DIR"
head -c 1000000 /dev/urandom > "$SRV_DIR/small.bin"
head -c 50000000 /dev/urandom > "$SRV_DIR/big.bin"
: > "$RESULTS"

cpu_seconds() {
    local pid=$1
    if [ -r "/proc/$pid/stat" ]; then
        local tck
        tck=$(getconf CLK_TCK)
        awk -v t="$tck" '{ sub(/.*\) /, ""); printf "%.3f", ($12 + $13) / t }' "/proc/$pid/stat"
    else
        ps -o time= -p "$pid" | awk -F'[:.]' '{ printf "%.3f", $1 * 60 + $2 + ($3 ? $3 / 100 : 0) }'
    fi
}

peak_mem_mb() {
    local pid=$1
    if [ -r "/proc/$pid/status" ]; then
        awk '/VmHWM/ { printf "%.1f", $2 / 1024 }' "/proc/$pid/status"
    else
        ps -o rss= -p "$pid" | awk '{ printf "%.1f", $1 / 1024 }'
    fi
}

run_test() {
    local version=$1 file=$2 idle=$3 label=$4
    local log="$WORK/${version}_${file}_${idle}.log"

    (cd "$SRV_DIR" && exec "$ROOT/$version/server") >> "$log" 2>&1 &
    local srv=$!
    sleep 0.5

    local idle_pid=""
    if [ "$idle" = "idle" ]; then
        (exec 3<>/dev/tcp/127.0.0.1/9000; sleep 15) &
        idle_pid=$!
        sleep 0.3
    fi

    rm -f "$CLI_DIR"/out_* "$CLI_DIR"/received_*
    local pids=()
    for i in $(seq 1 "$CLIENTS"); do
        (cd "$CLI_DIR" && exec "$ROOT/$version/client" "$file") > "$CLI_DIR/out_$i" 2>&1 &
        pids+=($!)
    done
    wait "${pids[@]}"

    local cpu mem
    cpu=$(cpu_seconds "$srv")
    mem=$(peak_mem_mb "$srv")

    kill "$srv" 2>/dev/null
    wait "$srv" 2>/dev/null
    if [ -n "$idle_pid" ]; then
        kill "$idle_pid" 2>/dev/null
        wait "$idle_pid" 2>/dev/null
    fi

    local ok
    ok=$(grep -l "verified" "$CLI_DIR"/out_* | wc -l | tr -d ' ')
    cat "$CLI_DIR"/out_* | awk -v label="$label" -v cpu="$cpu" -v mem="$mem" -v ok="$ok" '
        /Total time/ { t = $(NF - 1) + 0; sum += t; n++; if (n == 1 || t > max) max = t; if (n == 1 || t < min) min = t }
        END { printf "%s|%.3f|%.3f|%.3f|%.3f|%s|%s|%s\n", label, max, sum / n, max, min, cpu, mem, ok }
    ' >> "$RESULTS"
    sleep 0.5
}

echo "Running $RUNS runs of each test with $CLIENTS clients. This takes a minute or two."

for r in $(seq 1 "$RUNS"); do
    echo "Run $r"
    run_test version1 small.bin none "$CLIENTS clients, 1 MB file|Single thread"
    run_test version2_threadpool small.bin none "$CLIENTS clients, 1 MB file|Pool, 4 threads"
    run_test version1 big.bin none "$CLIENTS clients, 50 MB file|Single thread"
    run_test version2_threadpool big.bin none "$CLIENTS clients, 50 MB file|Pool, 4 threads"
    run_test version1 small.bin idle "1 idle client + $CLIENTS clients, 1 MB|Single thread"
    run_test version2_threadpool small.bin idle "1 idle client + $CLIENTS clients, 1 MB|Pool, 4 threads"
done

echo
echo "Machine: $(uname -s), $(getconf _NPROCESSORS_ONLN 2>/dev/null || echo '?') CPU cores"
echo
echo "| Test | Server | Total time | Avg client time | Slowest client | Fastest client | Server CPU | Server memory | Checksums ok |"
echo "|---|---|---|---|---|---|---|---|---|"
awk -F'|' '
    { key = $1 "|" $2; if (!(key in n)) order[++k] = key; n[key]++
      for (i = 3; i <= 8; i++) s[key, i] += $i; ok[key] += $9; runs[key]++ }
    END {
        for (j = 1; j <= k; j++) {
            key = order[j]; c = n[key]; split(key, p, "|")
            printf "| %s | %s | %.2f s | %.2f s | %.2f s | %.2f s | %.2f s | %.1f MB | %d/%d |\n",
                p[1], p[2], s[key,3]/c, s[key,4]/c, s[key,5]/c, s[key,6]/c, s[key,7]/c, s[key,8]/c, ok[key], c * '"$CLIENTS"'
        }
    }' "$RESULTS"

echo
echo "Pool log showing more than one worker busy at once (good for the screenshot):"
grep -h "active workers [2-9]" "$WORK"/version2_threadpool_big.bin_none.log | head -8
