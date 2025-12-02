#!/bin/sh
#
# Run a simple HTTP benchmark.
#
# Lars Wirzenius

set -e

case "$1" in
--fast) times=1000; shift ;;
*) times=100000 ;;
esac

port=8080

. benchmarks/functions.inc

rm -f bench_http.log
test/test_http_server -v 4 -l bench_http.log -p $port &
sleep 1
test/test_http -q -v 2 -r $times http://localhost:$port/foo
test/test_http -q -v 2 http://localhost:$port/quit
wait

awk '/INFO: Request for/ { print $1, $2 }' bench_http.log  |
test/timestamp | uniq -c |
awk '
    NR == 1 { first = $2 }
    { print $2 - first, $1 }
' > bench_http.dat

# Calculate stats
duration=$(awk 'END { print $1 }' bench_http.dat)
avg_rps=$(awk '{ sum += $2; count++ } END { printf "%.0f", sum/count }' bench_http.dat)

plot benchmarks/bench_http "time (s)" "requests/s (Hz)" "bench_http.dat" ""

sed -e "s/#TIMES#/$times/g" \
    -e "s/#AVG_RPS#/$avg_rps/g" \
    -e "s/#DURATION#/$duration/g" \
    benchmarks/bench_http.txt

rm -f bench_http.log
rm -f bench_http.dat
