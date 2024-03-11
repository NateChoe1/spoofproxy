#!/bin/sh
set -e

OUT_LOC=$(mktemp)

TEST_COUNT=0
SUCCESS_COUNT=0

make -C .. > /dev/null 2>&1

for test in $(ls *.test) ; do
	PAYLOAD="$(echo -en "$(head -n1 $test)")"
	IP="$(head -n2 $test | tail -n1)"
	RESULT="$(head -n3 $test | tail -n1)"
	printf "Testing %s: " $test
	LD_PRELOAD=../out/spoofproxy.so nc -lvp 8000 > $OUT_LOC 2>&1 &
	sleep 0.1
	printf "%s" "$PAYLOAD" | nc localhost 8000 > /dev/null 2>&1 &
	sleep 0.1
	killall nc > /dev/null 2>&1 || true
	PEER=$(head -n2 $OUT_LOC | tail -n1 | grep -oP '\[.*?\]' | tail -n1 | sed 's/\[\(.*\)\]/\1/')
	RECEIVED="$(tail -n+3 $OUT_LOC | tr -d '[\r]')"
	if ( [ "$IP" = "$PEER" ] && [ "$RESULT" = "$RECEIVED" ] ) || ( [ "$RESULT" = "_bad_" ]  && grep -q 'no connection' $OUT_LOC ) ; then
		echo "SUCCESS"
		SUCCESS_COUNT=$(expr $SUCCESS_COUNT + 1)
	else
		echo "FAILURE"
	fi
	TEST_COUNT=$(expr $TEST_COUNT + 1)
done

rm $OUT_LOC

printf "%s\n" "$SUCCESS_COUNT/$TEST_COUNT tests passed"
