# Kestrel Island 0.2 — implementation record

Implemented from [PLAN.md](PLAN.md), 2026-10-31. The original 288×40 compact notch, 456×304 expanded notch, hover, automatic media selection, independent artwork, and selected DP-2 monitor are preserved.

| Plan area | Delivered | Remaining acceptance boundary |
|---|---|---|
| Correctness | Restartable message expiry; expanded click pins; checked single-instance reply / bus failure; no ineffective icon tint API; private-bus test registration | Physical keyboard focus restoration, hotplug and drag cancellation still require hands-on testing |
| Activities | Bounded persistent activities, priority/coalescing/expiry notices, compact cycling, context page on hover | No plugin framework or history store |
| Music | Signal/owner subscriptions with reconciliation; position/rate timeline; capability-aware seek; generation and unique-owner guards | Real user playback was not changed in tests; browser-specific seek support depends on advertised MPRIS capabilities |
| Sound | Opt-in pactl observer, default output/microphone confirmation, visible muted microphone icon, compact feedback, bounded restart/backoff | This is default-source mute, not a universal recording detector or privacy lock |
| Focus | Countdown/custom durations, Pomodoro work/short/long breaks, pause with millisecond precision, explicit next phase, completion held until dismiss, suspend-aware clock | Memory-only; no wake alarm or session restore across restart |
| Keep awake | Two acknowledged inhibition cookies, partial-failure rollback, unique-owner release, timeout/stop/restart handling, sleep-inclusive deadline | Actual acquisition and release passed on KDE; forced sleep/lid/thermal policy remains owned by the system |
| Task hooks | Opt-in v1 event schema, bounded/rate-limited tasks, elapsed/stale/attention/finished states, file-based CLI sender and command wrapper | No automatic Codex/Claude desktop integration, log reading, account/tokens/cost tracking |
| Notifications | KDE watcher, sender validation, opt-in application allowlist, plain-text memory inbox, replace/close/expiry, fail-closed session state | **Live reception remains gated:** `notifications/validated` defaults false until a controlled real KDE lock/DND check is completed. The UI reports this boundary and does not register for notification content |

## Implementation choices

Stateful additions are isolated in `activitycoordinator`, `focussession`, `keepawakeservice`, `taskactivityservice`, `sessionstate`, and `notificationsservice`. The existing MPRIS and pactl adapters remain in the `Backend` facade rather than being rewritten into separate `MediaService`/`AudioService` classes. This preserves the existing command queue and selection tests; media presentation moved into `qml/MediaView.qml` and all added feature pages are separate QML components. `kestrel-core` is shared by the app and tests, so the same adapters are exercised.

Transient notices currently use the compact title, not a second overlay or competing desktop popup. Incoming timer/task/audio events never force the expanded window open. The tray's existing autostart preference remains off unless explicitly selected.

## Validation completed

- Three CTest suites pass: backend/QML/MPRIS/audio, activity/focus/task/keepawake/notification fake services, and CLI/wrapper behavior.
- Replacement-player test uses a second private bus connection taking over the same player name with the same track ID: a stale drag token cannot seek the replacement; a fresh token can.
- Microphone test uses fake pactl data and verifies confirmed mute, distinct muted SVG and image readiness. User output levels and playback were not modified.
- Fake KDE inhibition verifies full acquisition, release and second-cookie policy denial cleanup. Fake notification services verify owner filtering, allowlist, replacement, lock/DND clearing, bounded inbox and the disabled validation gate.
- CLI tests verify disconnected bus failure, oversized input rejection, command stdout/exit preservation and signal preservation.
- Actual KDE Wayland diagnostic on DP-2 reported `exposed=true`, compact mask height 40, keyboard interactivity 0, `keepAwakeAcquired=true`, `keepAwakeReleased=true`. Power inhibition was acquired briefly and released; no power policy was edited.
- Six offline empty/default views were rendered with KDE controls and visually reviewed. No private song title or notification data was used in these previews.
- Independent finish review scored its stale-seek and microphone-visibility findings resolved after regression tests. It did not certify physical drag, keyboard restoration, or live lock/DND behavior.

## Manual gates still open

1. Physical pointer drag, Escape cancellation, hotplug and typing restoration from an expanded control on the user's actual desktop.
2. Live notification privacy check covering AboutToLock, ActiveChanged, DND transitions, daemon restart, payload clearing and no historical replay. Do not set `notifications/validated` merely because mocks passed; this flag is a release gate, not an ordinary feature preference.

No actual notification watcher registration was enabled in the user's session. Task hooks and audio/microphone feedback are optional and remain controlled by the user's switches.

## Baseline and rollback

The pre-change source and installed binary were copied to `/tmp/kestrel-baseline-20261031/` before editing. This is a temporary local rollback aid, not a durable backup or git history. Existing settings keys are preserved. To roll back, quit Kestrel, ensure keep awake is off, restore that binary if the temporary backup still exists, and launch it. No notification daemon or audio configuration needs restoration.

## Fullscreen games

The main Wayland surface now uses LayerTop rather than LayerOverlay. KWin v6.8.1 maps layer-shell Top to AboveLayer and places active fullscreen windows in ActiveLayer above it. This keeps the notch behind fullscreen games/video, including XWayland games handled by KWin, without per-process game detection. Temporary move targets remain Overlay only during explicit dragging. Ordinary windowed/borderless windows that do not declare fullscreen are still ordinary windows; no universal game-process detection is claimed. An actual Dota 2 session has not been launched for testing.

Sources checked: https://github.com/KDE/kwin/blob/v6.8.1/src/layershellv1window.cpp#L216-L226 and installed `/usr/include/kwin/effect/globals.h:164–176`.

## System detail and navigation (0.3)

Added real CPU hardware/load/history, RAM used/available/total/swap, GPU load/VRAM/temperature/power where supported and disk used/available/total. Verified device discovery on this host: Ryzen 9 9900X3D, NVIDIA RTX 5070 Ti and AMD Radeon 610M. Unknown counters render unavailable. Memory availability includes reclaimable cache; AMD integrated VRAM is driver-reported reservation, not the complete shared-memory budget.

System has a larger readable expanded layout while preserving the original compact size and hover. A Tools tab exposes the existing optional services. Explicit navigation is remembered on reopen. Notifications still retain the original validation gate. Added fixture tests for MemAvailable accounting, unsupported NVIDIA metrics, unit conversion and page-memory/expanded geometry.

Release validation: all three CTest suites pass. Actual Wayland check on DP-2 returned exposed=true, page=2, System input-region height=568, LayerTop=2 and both GPU snapshots. Installed 0.3.0 under ~/.local and restarted the user service on System. This does not certify an actual Dota 2 session or unlock the notification gate.

## UI and desktop verification (0.3.1)

Music has larger artwork, long-title tooltips, timeline capability messaging and keyboard focus feedback. Focus has visibly selected modes, editable duration, presets and a clear primary action. Keep awake shows confirmed state and remaining seconds; child tools provide Back navigation. Tasks includes a copyable command and explicit hook state. Notifications shows the unavailable gate honestly and disables unvalidated configuration. StackLayout minimum sizing no longer pushes navigation/footer out of smaller pages.

All three private-bus CTest suites pass. Added real D-Bus lock/DND signal-route tests, page geometry checks, Pomodoro layout bounds and explicit-collapse/hover regression coverage. Live KWin checks found and fixed an actual focus-return bug. Active collapse now unmaps/remaps with keyboard None; fullscreen remains above the island. Native fullscreen/focus checks pass; actual Dota 2 and physical hotplug are not certified.

With explicit user consent, real KDE watcher checks passed synthetic receive, DND clear/suppression, no replay after resume, receive after resume, disabled inbox clearing and preservation of the notification daemon owner. Four silent transient test notifications were closed; only synthetic content was kept in RAM and no real message content was saved. The quiet-mode test restored its inhibition token. The lock/unlock observation is a separate remaining gate until the user actually performs it; notifications/validated must stay false meanwhile. See LIVE_CHECKS.md and .impeccable/review/0.3.1.

Final 0.3.1 validation: all three suites pass (7.99 seconds in the final full run); added Pomodoro bounds test also passes. The final native probe reports fullscreenAboveIsland=true, islandAcceptedFocus=true, focusReturnedAfterCollapse=true, expandedAfterCollapse=false and islandStillActiveAfterCollapse=false. Installed binary reports 0.3.1; user service restarted on DP-2, Focus. Notification inhibition reads false after test cleanup. Lock observation timed out with lockObserved=false and unlockedAndSafe=false; the user confirmed no lock/unlock had yet been performed. The validation flag was not changed.

## Slider repair (0.3.2)

Fixed zero-height pointer hit areas for timeline and volume with KDE's native control style. A new mouse-driven regression fails on 0.3.1 and passes after the two implicitHeight fixes. All four CTest suites pass, including the dedicated KDE-style test. See SLIDER_FIX.md. No actual playback or output level was changed for testing.

## Explicit pin interaction (0.3.3)

Mouse leave collapses after 180 ms unless the top Pin button is checked. Removed implicit pinning from clicks, navigation, Settings, activity cycling and D-Bus expansion. The button has a checked visual state, accessible action label and bundled SVG. Unpinning outside schedules collapse; explicit collapse clears pin. Regression uses actual mouse clicks/movement to verify unpinned leave, pinned leave and leave after unpin. All four suites pass.

## Compact artwork (0.3.4)

Replaced the compact leading logo with the selected music/video local MPRIS artwork at 24×24, retaining the logo as loading/error/missing-art fallback and for non-media activities/notices. Compact geometry stays 288×40; hover and explicit pin behavior are unchanged. Images use the same local-only source policy as the expanded player. Read-only inspection confirmed the current Plasma Browser Integration artwork points to an existing local file.

Native compact check on DP-2: exposed=true, mediaAvailable=true, compactArtworkReady=true, inputRegionHeight=40. Installed and restarted 0.3.4 in compact mode; all four existing test suites passed.

## Temperatures (0.3.5)

Added a temperature strip to the top of the System page: CPU, each GPU and each NVMe controller render as labelled readings with a large value and device-name tooltip. CPU reads the package sensor from hwmon `k10temp`/`coretemp`/`zenpower` `temp1_input`; GPUs read `hwmon/temp1_input` under each `/sys/class/drm/cardN/device` (NVIDIA values still come from nvidia-smi where it wins the merge); NVMe reads each controller's composite `temp1_input` under `/sys/class/nvme/nvmeN`, skips namespace entries and falls back to the controller id when no model string exists. Missing or unparsable sensors render an em dash and an unavailable tooltip rather than a fake zero. Readings refresh with the system page cadence (1s sysfs, NVIDIA merge still 2s throttled); disk polling remains 10s.

The strip is a new `TemperatureReading` component with icon, accessible name and plain-text value; three bundled SVG icons (cpu/gpu/nvme) were added. A fixture test covers composite-channel selection, namespace exclusion, missing-model fallback and unreadable/absent sensors; the existing NVIDIA fixture now also asserts unavailable temperature. All four CTest suites pass at 0.3.5. Offscreen render on this host showed live values: CPU 48°C, NVIDIA 47°C, AMD 610M 42°C, NVMe 41°C (`.impeccable/review/system-temperatures/`). No sensor limits, fan controls or other settings were modified.

Deployment note: binaries built on this host inherit `x86 ISA needed: …x86-64-v4` in `.note.gnu.property`, propagated at link time from this machine's v4-optimized glibc (`libc.so.6`/`libm.so.6`/`libresolv.so.2`), not from our objects (all `x86-64-baseline`). On hosts that support only up to x86-64-v3 the loader rejects the binary with "CPU ISA level is lower than required". Fix for foreign deployment: `objcopy --remove-section=.note.gnu.property` on the installed binary; each shared dependency's own marker is still checked at load time. Deployed 0.3.5 this way to `bundit@192.168.1.198:~/.local/bin`, added a matching `~/.config/autostart/io.github.kestrel.Island.desktop`, and started it on its `wayland-1` session.

## Icon-first UI refresh (0.3.6)

Replaced the four text navigation pills with icon tabs (note, timer, chip, grid) using new bundled SVGs in the existing 24 px stroke style; the selected tab gets the lavender tonal state and unselected icons dim to 55%. Tool rows, settings chips and Focus mode chips now lead with semantic icons (coffee, terminal, bell, timer, cycle); System section headers reuse the cpu/gpu/nvme/ram icon set. `IslandButton` gained `selected`/`dim` states and `IslandAction` an optional leading `symbol`.

The Music page was rebuilt around a larger 96 px artwork with a small-caps status eyebrow, title, artist and transport controls tucked under the metadata, matching the Dynamic Island reference layout; the timeline is a slimmer 3 px track. Expanded heights dropped to Music/Settings 304, Focus/Tasks/Inbox 384, Keep awake 336 and Tools 288 while compact stays 288×40; layout spacing normalised to 8 px. All four CTest suites pass at 0.3.6, including updated page-geometry expectations.

## Click-driven interaction (0.3.7)

Hover no longer opens the island. Compact hover now plays a peek animation — the notch nods to 54 px tall after ~120 ms and recentres its row — gated by the renamed "Peek on hover" preference (same `hoverExpand` key) and reduced motion. Clicking expands and implicitly pins, so pointer leave alone never closes the window; only the collapse button, Escape, tray toggle or an explicit unpin plus pointer leave does. The compact drag handle tracks the peeked height so the whole nudged surface still expands on click. Peek keeps shape/input-mask parity through the existing geometry-timer path. Tests renamed to `compactPeeksOnHoverAndExpandsOnClick` and `expandedStaysUntilExplicitClose`; all four CTest suites pass.

## Vorssaint-style player and outside dismissal (0.3.8)

Expanded dismissal now follows window activation: an outside click (losing `active`) collapses the island unless the pin is checked, replacing auto-pin on expand; pointer leave alone never closes it. The unpin-while-outside path still schedules collapse. `dismissOutside()` is the shared path so tests can exercise it without real focus events.

The Music page now follows Vorssaint's notch player recipe (read from `vorssaint/vorssaint-utils`, GPL-3.0): 68 px cover (scales to 0.94 while paused) with semibold title and secondary artist beside it, live equalizer bars at the row's far end, a full-width timeline showing elapsed and *remaining* time, and a five-slot transport row keeping play centered in the middle three (slot 1 hosts the activity switcher, slot 5 kept empty for balance). All four CTest suites pass.

### Cover tint (0.3.8, follow-up)

`media.tint` reports the dominant colour of the local artwork (8×8 downsample, HSV-clamped for legibility on black, cached per path). Neutral or unreadable covers return empty and the player keeps the lavender accent — Vorssaint's "neutral covers keep them white" rule. Timeline progress and the live equalizer bars consume it.

## UI polish (0.4)

Visual polish only, adopting Vorssaint's island UI strengths per `docs/PLAN-UI.md`; no features, adapters or data flows changed.

- One token source: `qml/Theme.qml` singleton owns all colours, cover-tint computation with contrast clamps, and `dur(ms)` — the single motion gate (`animations` flag × KDE `AnimationDurationFactor`, read once at startup, clamped [0.25, 1]). All hard-coded hex literals and `island.animations ?` duration gates are gone from shipped QML.
- Dark island controls: `IslandStepper` (restyled bounded SpinBox, −/+ faces, Up/Down keys), `IslandCheck` (restyled CheckBox; also replaces the Switches) and `IslandSelect` (restyled ComboBox closed face, delegate and popup) remove every light KDE control face while keeping `focusMinutes`, `displaySelector` and all other `objectName`s, Accessible names, Theme.tint focus rings and strict min/max bounds. The notifications text field uses a dark raised face.
- `EdgeFade` draws 16 px surface-to-transparent gradients at the top/bottom of the Settings, Tasks, Notifications and System scrolling areas, visible only while more content lies beyond that edge; verified seamless on the solid surface (U3).
- System page (V5): 2-column `MetricCard` grid — CPU, RAM, each GPU, each disk — with title, symbol, value, 3 px meter and subtitle folding in temperatures; value and meter turn `Theme.warn` at ≥85% load or <10% disk free. The previous detailed rows (CPU history, VRAM/power, swap, device/filesystem) stay behind an in-memory Details disclosure. Collapsed geometry is 520×480; details expand to 560×568 with the input mask following as usual. `systemTemperatures` moved onto the card grid; `TemperatureReading.qml` was removed.
- Escape now closes the inner layer first (V10): pages 4–6 step back to Tools before a second Escape collapses. The compact title cross-fades 140 ms on activity/notice changes with tabular figures (V8/V9 transition, without layout change).
- Tests: `escapeStepsBackToToolsBeforeCollapsing` covers pages 4–6 and the collapse; the System details geometry (560×568) and footer bound are regression-checked; all four CTest suites pass. All eight `scripts/ui-shots.sh` pages were rendered and reviewed: no clipped footers, no light control faces, no hard-cut scroll edges.

Acceptance boundary: no live-session check yet (M4-T1/T2), no attention-state screenshot fixture (logic covered by bound/threshold review), and the known `MediaView` recursive-rearrange warning is unchanged. The 288×40 compact footprint, peek geometry, LayerTop stacking and all opt-in gates (notifications/validated, tasks, audio feedback) are untouched.
