#!/bin/sh
#
# Use `test/fakesmsc' to test the bearerbox and the smsbox.

set -e
#set -x

# Cleanup function
cleanup() {
    kill -INT $bbpid $smspid 2>/dev/null || true
    wait 2>/dev/null || true
}
trap cleanup EXIT INT TERM

times=10
interval=0
loglevel=0
host=127.0.0.1

../gw/bearerbox -v $loglevel ../gw/smskannel.conf > check_fakesmsc_bb.log 2>&1 &
bbpid=$!
sleep 2

../test/fakesmsc -H $host -r 20000 -i $interval -m $times '123 234 text nop' \
    > check_fakesmsc.log 2>&1 &
sleep 1

../gw/smsbox -v $loglevel ../gw/smskannel.conf > check_fakesmsc_sms.log 2>&1 &
smspid=$!

running="yes"
while [ $running = "yes" ]
do
    sleep 2
    if grep "Got message $times" check_fakesmsc.log >/dev/null
    then
    	running="no"
    fi
done

kill -INT $bbpid $smspid
wait

if grep 'WARNING:|ERROR:|PANIC:' check_fakesmsc*.log >/dev/null
then
	echo check_fakesmsc.sh failed 1>&2
	echo See check_fakesmsc*.log for info 1>&2
	exit 1
fi

rm -f check_fakesmsc*.log

exit 0
