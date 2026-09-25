#!/bin/bash
# Packages the release files after `make` and `make debug`.
# Usage: scripts/package-release.sh <version>
# Writes the downloadable files to build/release/ and the release notes to
# build/release-notes.md (kept outside the release folder so it isn't uploaded).
set -euo pipefail
cd "$(dirname "$0")/.."

version=${1:?usage: package-release.sh <version>}
out=build/release
commit=$(git rev-parse --short HEAD)

for f in build/typing-adventure.img build/sdcard/kernel8.img build/debug/kernel8.img; do
	[ -f "$f" ] || { echo "missing $f: run make and make debug first" >&2; exit 1; }
done

rm -rf "$out"
mkdir -p "$out"
xz -9 -T0 -c build/typing-adventure.img > "$out/typing-adventure-$version.img.xz"
(cd build/sdcard && zip -q -X "../../$out/typing-adventure-$version-sdcard-files.zip" ./*)
cp build/sdcard/kernel8.img "$out/kernel8.img"
cp build/debug/kernel8.img "$out/kernel8-debug.img"
(cd "$out" && sha256sum ./* | sed 's| \./| |' > SHA256SUMS.txt)

sed -e "s/{{VERSION}}/$version/g" -e "s/{{COMMIT}}/$commit/g" docs/release-notes.md > build/release-notes.md

echo "Release $version:"
ls -l "$out"
