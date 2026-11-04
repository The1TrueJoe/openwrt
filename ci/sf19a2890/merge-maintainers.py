#!/usr/bin/env python3
# Resolve git merge-file conflicts in MAINTAINERS where two series add
# entries at the same place: keep every entry from both sides verbatim and
# put the entries in title order, as a maintainer merging them would.
import sys

path = sys.argv[1]
out, ours, theirs, side = [], [], [], None
for line in open(path):
    if line.startswith("<<<<<<< "):
        side = ours
    elif line.startswith("=======") and side is ours:
        side = theirs
    elif line.startswith(">>>>>>> ") and side is theirs:
        entries = {e.strip("\n") for s in (ours, theirs)
                   for e in "".join(s).split("\n\n") if e.strip()}
        entries = sorted(entries, key=str.lower)
        tail = "\n\n" if (ours + theirs)[-1] == "\n" else "\n"
        out.append("\n\n".join(entries) + tail)
        ours, theirs, side = [], [], None
    elif side is not None:
        side.append(line)
    else:
        out.append(line)
if side is not None:
    sys.exit("unterminated conflict in " + path)
open(path, "w").write("".join(out))
