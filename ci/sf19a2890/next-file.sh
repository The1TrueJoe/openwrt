#!/bin/bash
# Print one file of linux-next's master: next-file.sh <path>. kernel.org
# sometimes refuses runner addresses, so fall back to its mirror.
set -eo pipefail
korg=https://git.kernel.org/pub/scm/linux/kernel/git/next/linux-next.git/plain
mirror=https://kernel.googlesource.com/pub/scm/linux/kernel/git/next/linux-next/+/refs/heads/master
curl -fsSL --connect-timeout 20 --retry 2 "$korg/$1" ||
	{ echo "::warning::kernel.org unreachable, $1 from its mirror" >&2
	  curl -fsSL --retry 2 "$mirror/$1?format=TEXT" | base64 -d; }
