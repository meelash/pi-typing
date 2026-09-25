#!/bin/bash
# Runs the simulator scripts in order against a fresh save file.
# Screenshots go to build/shots/. Exits non-zero if any screen check fails.
set -e
cd "$(dirname "$0")/../.."
make -s -C src/sim
out=build/shots
rm -rf "$out" && mkdir -p "$out"
status=0
for t in tests/sim/[0-9]*.txt; do
	echo "== $t"
	./build/sim/sim "$t" "$out" "$out" >/dev/null || status=1
done
exit $status
