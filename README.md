# Send the switch reset series from the other Mac

The bundle `mxl-gsw1xx-reset.bundle` (4 KB) holds the b4 branch
`b4/mxl-gsw1xx-reset`: the cover letter with its To/Cc and the 2 patches,
on net-next main 4dd16e311 (its tip on 2026-11-02). `b4-preview/` is what
`b4 send -o` rendered from it: 3 emails, `[PATCH net-next 0/2]` to `2/2`.

Send from ONE Mac only. Nothing has been sent.

Before sending, check that net-next is open: no "net-next is CLOSED" post
on netdev, and https://netdev.bots.linux.dev/net-next.html says open. It
closes for the two weeks of each merge window.

1. Get net-next. A sparse clone checks out only the top-level files, which
   avoids the case collisions a full checkout hits on macOS. b4 works from
   the git objects.

       git clone --depth 1 --sparse https://git.kernel.org/pub/scm/linux/kernel/git/netdev/net-next.git linux-netnext
       cd linux-netnext

   If net-next has moved on since 4dd16e311, fetch enough history to
   include it:

       git cat-file -e 4dd16e311^{commit} || git fetch --shallow-since=2026-10-28 origin main

2. Download the bundle from the fork and load the series from it:

       curl -fLO https://github.com/The1TrueJoe/openwrt/raw/mxl-gsw1xx-reset-send/mxl-gsw1xx-reset.bundle
       git fetch ./mxl-gsw1xx-reset.bundle b4/mxl-gsw1xx-reset:b4/mxl-gsw1xx-reset
       git checkout b4/mxl-gsw1xx-reset
       rm mxl-gsw1xx-reset.bundle

3. Identity and relay (the same as for the LED series):

       git config user.name "Joseph Telaak"
       git config user.email joseph@telaak.dev
       git config b4.send-endpoint-web https://lkml.kernel.org/_b4_submit

   The patatt key and the web auth from the LED send are per user, not
   per repository, so there's nothing to redo. If b4 says
   patatt.signingkey is not set, run `git config patatt.signingkey` and
   `git config patatt.selector` with the values from the LED tree on that
   Mac.

4. Check, then send once:

       b4 send --use-web-endpoint --dry-run
       b4 send --use-web-endpoint

   The dry run should list 3 messages, From joseph@telaak.dev, with
   subjects [PATCH net-next 0/2] to [PATCH net-next 2/2].

If b4 reports "uncommitted changes", run `git status`. Remove any stray
files, and mark paths that macOS deleted with
`git ls-files --deleted -z | xargs -0 git update-index --skip-worktree`.
