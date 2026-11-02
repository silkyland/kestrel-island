# Architecture

`src/main.cpp` starts QApplication, registers the single-instance D-Bus service `io.github.kestrel.Island`, loads embedded QML and creates a system tray icon. A second launch invokes `/Island` → `io.github.kestrel.Island.Toggle`.

`src/backend.cpp` owns desktop integration and session state:

- Asynchronous session D-Bus discovery of `org.mpris.MediaPlayer2.*`, followed by `org.freedesktop.DBus.Properties.GetAll`. Metadata is decoded with `qdbus_cast`. Selection is automatic: playing sources outrank paused ones, recent playback/track changes outrank older activity, Plasma Browser Integration breaks ties ahead of duplicate browser sources, and the current source remains stable otherwise. Candidates are ranked only after all asynchronous replies finish. There is no manual player selector. Polls are bounded and do not overlap.
- MPRIS Previous, Next and PlayPause only when the advertised capabilities permit them. A player disappearing is handled by the next poll.
- CPU deltas from `/proc/stat`, excluding duplicate guest counters, and memory from MemAvailable in `/proc/meminfo`.
- Non-shell QProcess calls to `pactl`, with timeouts, for audio queries and user-requested volume/mute changes.
- FocusSession uses BOOTTIME deadlines, retains millisecond pause state, and emits a bounded compact completion notice without opening a page. It does not replace Plasma's notification daemon.
- QSettings preferences, opt-in autostart through QSaveFile, and per-screen placement.
- LayerShellQt top anchor, Top layer (below active fullscreen), exclusion zone -1 (ignore panels, reserve no space), on-demand keyboard interactivity. The top-gap setting is the remedy for a top panel occupying the same position. Input QRegion follows the same concave-shoulder geometry as the QML Shape; pixels outside the island pass through.

`qml/Main.qml` contains one transparent 588×594 window. Its compact island is 288×40; expanded height follows the current page (304–568 logical pixels); the window itself does not repeatedly resize during animations. `qml/IslandButton.qml` provides keyboard-accessible, labelled icon controls with project-owned SVGs. No remote artwork is loaded.

Tests run on a private session bus to prevent MPRIS mocks from appearing in the user's desktop. The ordinary Wayland session check performs read-only audio/media queries. Offscreen screenshots show an offline empty state.

## Sources checked during implementation

- [Upstream feature inventory](https://github.com/vorssaint/vorssaint-utils/blob/main/README.md), consulted 2026-10-31. Only the Dynamic Island feature is in scope after the user's clarification.
- [MPRIS 2.2 Player interface](https://specifications.freedesktop.org/mpris/latest/Player_Interface.html), metadata, transport controls and capability flags.
- [LayerShellQt Window API](https://api.kde.org/legacy/plasma/layer-shell-qt/html/classLayerShellQt_1_1Window.html). The installed `/usr/include/LayerShellQt/window.h` was checked for actual anchors, screen, margins, exclusion and keyboard APIs.
- [Qt system tray support](https://doc.qt.io/qt-6/qsystemtrayicon.html).
- [pactl command documentation](https://man.archlinux.org/man/pactl.1), with JSON output verified against the installed PipeWire PulseAudio server.

Verified environment: CachyOS, KDE Plasma Wayland, Qt 6.12.0, LayerShellQt 6.8.2, PipeWire 1.6.10. Other compositor/distribution combinations need their own validation.

## Validation, 2026-10-31

- Clean CMake/Ninja build and all eight QtTest cases pass. Real MPRIS discovery/metadata/control is tested on a private D-Bus session. Slow-command and quick-slider-release regressions use a fake pactl; real playback and output levels are untouched.
- Actual KDE Wayland session checks expose the island and read live audio, media, CPU and memory. Compact mask height is 40 logical pixels with keyboard interactivity 0 (None). Expanded mask height is 304 with keyboard interactivity 2 (OnDemand).
- Wayland protocol capture confirms layer 3 (overlay), scope kestrel-island, and exclusive zone -1.
- Offscreen compact, music empty-state and focus empty-state renders were opened for visual inspection. An independent finish reviewer scored the audio-queue and collapse-focus findings resolved after corrections, and checked the design record against QML.
- A nonempty mask and protocol settings establish configuration; restoration of typing into an arbitrary previous application and pointer pass-through were reviewed in code, not independently verified by native input automation. Hotplug and X11 behavior still need physical-session validation.

Interaction corrections: see [notch interaction regression ledger](repro/notch-interactions.md). Display changes now recreate the native layer surface. Buttons render bundled SVGs directly to bypass the KDE style icon delegate. Settings are available while compact; hover expansion is opt-in. QDrag and temporary per-display layer windows provide drag-to-display selection. Twelve regression cases now pass, including native-style QML control tests.

User preference update, 2026-10-31: restore the original 288×40 compact footprint and enable hover expansion by default. Keep the compact Settings button and monitor/icon repairs. Regression expectations now verify hover expansion rather than click-only behavior. The earlier interaction ledger records the previous request, not the current preference.

## Version 0.2 implementation

See [IMPLEMENTATION.md](IMPLEMENTATION.md) for current delivery and manual gates; the dated eight/twelve-case notes above are historical. The executable and tests share kestrel-core. New QObject services isolate activity arbitration, focus state, inhibition leases, task validation and notification/session privacy. Backend retains tested media/audio adapters. Media now resolves unique owners asynchronously, uses signal-triggered snapshots with reconciliation, and guards seek by generation and track context. Task transport is versioned and explicit, not a provider-log parser. The notification watcher is disabled behind a validation gate; the user session has not registered to receive notification content.

Fullscreen stacking correction, 2026-10-31: the main notch now requests layer-shell Top (2), mapped by KWin to AboveLayer, below ActiveLayer. The older layer-3 protocol result above is historical. Temporary drag targets retain Overlay only while the user explicitly moves the notch. See installed /usr/include/kwin/effect/globals.h:164–176 and KWin v6.8.1 src/layershellv1window.cpp:216–226.

## System readings (0.3)

`SystemMonitor` owns the snapshot exposed as `Backend.statistics`. CPU uses `/proc/stat` deltas (guest counters excluded), model/frequency/core counts from `/proc/cpuinfo`, and supported hwmon CPU temperatures. Memory uses `MemAvailable`, including reclaimable cache. QStorageInfo reports disk used and bytes available to the current user; matching root/home devices are merged. GPU devices are discovered through DRM PCI sysfs; AMD busy/VRAM counters come from its driver. NVIDIA uses a bounded asynchronous `nvidia-smi` process with fixed arguments, 1.5-second timeout and no shell. Missing counters remain unavailable. No media/audio mutation is involved.

CPU/RAM update every second while expanded, NVIDIA at most every two seconds while System is visible, disks every ten seconds. The main window is 588×594; visible System geometry is 560×568, other expanded pages use width 456 and heights 304–400, compact 288×40. Input region follows the animated shape. `ui/lastPage` remembers explicit page navigation; Settings does not replace that preference.

## Focus release and live verification (0.3.1)

`Backend::setExpanded(false)` sets KeyboardInteractivityNone and, if the native window is active, hides/shows that same QWindow. KWin then releases focus through its own focus chain; activateOnShow remains false. Explicit collapse stops hover timers and blocks reopening until pointer leave. `/Island` exposes State and SetExpanded for deterministic same-user live checks.

`kestrel-desktop-probe` is built only with BUILD_TESTING and is never registered with CTest. It explicitly opens a temporary fullscreen window and loads temporary KWin scripts to verify actual stacking and focus return. Its privacy modes observe SessionState or test synthetic notifications after explicit consent, preserving daemon ownership and cleaning temporary inhibitors/notifications. See LIVE_CHECKS.md.
