#!/bin/bash
# checkpatch --strict from linux-next on one series' patches. Run from the
# top of a kernel tree: checkpatch.pl <series-dir> [checkpatch options]
# Fails on any ERROR, and on any WARNING or CHECK not in the allow-list.
set -e
dir=$1; shift
series=$(basename "$dir")
cp=$(mktemp -d)
url=https://git.kernel.org/pub/scm/linux/kernel/git/next/linux-next.git/plain
for f in scripts/checkpatch.pl scripts/spelling.txt scripts/const_structs.checkpatch localversion-next; do
	curl -fsSL "$url/$f" -o "$cp/$(basename $f)"
done
echo "checkpatch from linux$(cat $cp/localversion-next)"

# series/patch/type/count. FILE_PATH_CHANGES: the new file is covered by
# an existing entry (drivers/clk/siflower/ from patch 2; LED SUBSYSTEM's
# Documentation/leds/). ENOTSUPP: the pinconf core requires it.
allowed="
02-sysm/0004/WARNING:FILE_PATH_CHANGES/1
03-pinctrl/0002/WARNING:ENOTSUPP/2
09-leds/0008/WARNING:FILE_PATH_CHANGES/1
"
fail=0
for p in "$dir"/*.patch; do
	n=$(basename "$p" | cut -c1-4)
	perl "$cp/checkpatch.pl" --strict --show-types "$@" "$p" > "$cp/out" || true
	cat "$cp/out"
	while read -r c t; do
		if grep -qx "$series/$n/$t/$c" <<< "$allowed"; then
			echo "allowed: $series patch $n $t x$c"
		else
			echo "::error::$series patch $n: $t x$c"; fail=1
		fi
	done < <(grep -oE '^(ERROR|WARNING|CHECK):[A-Z0-9_]+' "$cp/out" | sort | uniq -c)
done
rm -rf "$cp"
exit $fail
