# Native verification, 0.3.1

These are explicit desktop checks, not unattended CTests. Do not change media/audio to test the island. Quit the running app before `--check-session`; keep it running for the desktop probe.

## Automated regression suite

`ctest --test-dir build --output-on-failure` runs three suites on private D-Bus sessions. Coverage includes every page's navigation/footer bounds, explicit-collapse hover suppression, remembered page, Focus mode/start, context-page return, sender/owner guards, real D-Bus lock/DND signal routes, notification clearing and unknown-state suppression.

## Fullscreen and focus

```
./build/kestrel-desktop-probe --fullscreen
```

A temporary fullscreen window appears on DP-2 (or the default display when DP-2 is absent). Temporary KWin scripts read only the island/probe state and restore the prior active window. The probe verifies actual back-to-front stacking, explicitly expands the island, lets it accept focus, collapses it and verifies the probe receives focus again. Temporary scripts unload on exit. The app's prior expanded state is restored. This tests KWin's fullscreen behavior, not a Dota 2 match.

Verified here: fullscreen above island, island accepts focus, focus returns after collapse. Machine-readable before/after evidence is in `.impeccable/review/0.3.1/`.

## Display remapping

```
./build/kestrel-island --check-session --cycle-displays --screen DP-2 --expanded --page focus
```

The island remaps across the connected outputs and restores DP-2. Actual session result: Wayland, exposed=true, page=1, mask height=400, keyboard interactivity=OnDemand, presentation layer=Top. This does not certify physical dragging, cable hotplug, or every monitor layout; do not disable the user's outputs to simulate these without an explicit request.

## Notification privacy

```
./build/kestrel-desktop-probe --quiet
./build/kestrel-desktop-probe --watch-lock
```

The first test temporarily acquires a notification inhibition cookie and releases it, leaving an already-quiet/locked/unknown session unchanged. The second waits up to five minutes for the user to lock/unlock the desktop, observing only booleans. It never locks the desktop itself or registers a notification watcher.

After explicit consent:

```
./build/kestrel-desktop-probe --notifications
```

This temporarily registers a watcher, with a unique synthetic app allowlist, and sends four silent transient notifications. KDE forwards raw notifications from all apps while registered; only the synthetic app's entries are kept in RAM. No real notification content is logged or saved. All test IDs and temporary inhibition are cleaned on exit. The app's notification preference is unchanged.

Verified on the real KDE daemon: synthetic notification received, inbox cleared during quiet mode, quiet notification rejected, no replay after quiet mode, new notifications received after resume, disabled inbox empty, notification daemon owner unchanged. These actual results are saved in `notifications.json` and `quiet.json` under the review directory.

Lock/unlock is a separate manual gate. Do not mark it passed merely because DND or mocks passed. Do not set notifications/validated until both real session-state routes and the controlled synthetic notification check pass. Enabling collection remains a separate user switch and an app allowlist is mandatory.

## Focus defect ledger

- Original implementation: two independent live runs left the island active after collapse.
- Attach attempt: kernel ptrace restrictions blocked attaching; no kernel settings were changed.
- Trace: collapse changed local state to false, but KWin still reported the island active. Waiting 1.5 seconds instead of 0.5 did not fix it; the probe was active before entering the island. This ruled out an ordinary delayed update and a failed collapse request.
- KWin 6.8.1 `handleAcceptsFocusChanged` only activates a surface that wants input; the None transition does not activate a replacement window. Source: https://github.com/KDE/kwin/blob/v6.8.1/src/layershellv1window.cpp#L293-L306
- Fix: unmap/remap the active QWindow on collapse with keyboard None and activateOnShow=false. This returns focus through KWin's normal focus chain. Explicit collapse stops pending hover timers and suppresses re-entry until pointer leave.
- The diagnostic now exercises a full explicit expand/collapse cycle even if the app started expanded. Before that setup change one run could not activate the initially expanded surface, so it was not counted as a focus-return pass. Subsequent collapsed and pre-expanded runs both passed the complete cycle.
