# Send LED v2 from the other Mac

The bundle `ktd202x-et6326.bundle` holds the b4 branch `b4/ktd202x-et6326`
for v2: the cover letter with its To/Cc and the 11 patches, on Lee's
for-leds-next 59cf91cf8 (still its tip on 2026-11-03). b4 tracks it as
revision 2, with v1's Message-Id in its history, so v2 threads under
"Link to v1".

NOT READY TO SEND YET. The cover's "Tested on" block has one PENDING marker,
for the owner's look at the breathing on the Opal. This bundle is rebuilt
when the marker comes out. Send from ONE Mac only, around 2026-11-09.

Use the linux-leds clone that sent v1.

1. Keep the old branch under another name, then load v2 from the bundle:

       cd linux-leds
       git branch -m b4/ktd202x-et6326 b4-ktd202x-et6326-before-v2
       curl -fLO https://github.com/The1TrueJoe/openwrt/raw/leds-ktd202x-send/ktd202x-et6326.bundle
       git fetch ./ktd202x-et6326.bundle b4/ktd202x-et6326:b4/ktd202x-et6326
       git checkout b4/ktd202x-et6326
       rm ktd202x-et6326.bundle

   `git rev-parse --short HEAD` must print 2bbfff93d.

2. Check, then send once:

       b4 send --use-web-endpoint --dry-run
       b4 send --use-web-endpoint

The dry run should list 12 messages, From joseph@telaak.dev, with subjects
[PATCH v2 00/11] to [PATCH v2 11/11].

If b4 reports "uncommitted changes", run `git status`. Remove any stray
files. What's left should be only paths that macOS can't check out (files
whose names differ only in case, or that a sparse clone left out). Hide
them from b4 with
`git status --porcelain | awk '{print $2}' | xargs git update-index --skip-worktree`.
A rehearsal of these steps on 2026-11-03 rendered the same 12 emails as
the preview.
