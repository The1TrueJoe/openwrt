# Send the LED series from another Mac

The bundle `ktd202x-et6326.bundle` (16 KB) holds the b4 branch
`b4/ktd202x-et6326`: the cover letter with its To/Cc, and the 10 patches,
on Lee's for-leds-next 59cf91cf8 (still its tip on 2026-11-01). A rehearsal
from a fresh clone gave the same 11 emails as the reviewed preview.

Send from ONE Mac only. Nothing has been sent from the other one.

1. Get Lee's tree (depth 1, about 300 MB):

       git clone --depth 1 --branch for-leds-next https://git.kernel.org/pub/scm/linux/kernel/git/lee/leds.git linux-leds
       cd linux-leds

2. Load the series from the bundle (adjust the path to where you saved it):

       git fetch ~/Downloads/ktd202x-et6326.bundle b4/ktd202x-et6326:b4/ktd202x-et6326
       git checkout b4/ktd202x-et6326

3. Identity and relay:

       git config user.name "Joseph Telaak"
       git config user.email joseph@telaak.dev
       git config b4.send-endpoint-web https://lkml.kernel.org/_b4_submit

4. If this Mac has never sent through the B4 relay as joseph@telaak.dev:

       patatt genkey          # then run the two git config lines it prints
       b4 send --web-auth-new
       b4 send --web-auth-verify <token from the email>

5. Check, then send once:

       b4 send --use-web-endpoint --dry-run
       b4 send --use-web-endpoint

The dry run should list 11 messages, From joseph@telaak.dev, subjects
[PATCH 00/10] to [PATCH 10/10].
