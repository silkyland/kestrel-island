# Working on Kestrel Island

The user narrowed this project to **only the Dynamic Island / notch**, inspired by Vorssaint. Do not grow it into a general toolkit or dashboard without an explicit request.

- Native KDE-first C++20 + Qt Quick + LayerShellQt; no web wrapper.
- Build: `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build -j 4`.
- Tests: `ctest --test-dir build --output-on-failure`. Tests use a private D-Bus session when available.
- Runtime: `./build/kestrel-island`; read-only session check: `./build/kestrel-island --check-session` (quit any running instance first).
- Screenshots: `QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software ./build/kestrel-island --screenshot /tmp/island-preview --expanded`. Media preview is offline; `--page system` reads aggregate hardware counters.
- Preserve shape/input-mask parity, nonexclusive keyboard focus, no workspace reservation, and independent name/artwork.
- Do not change real media playback or audio settings for tests. Use an isolated MPRIS fake.
- Settings and autostart are user opt-in. No root helper, shell commands assembled from metadata, telemetry, or remote art downloads.
- See PRODUCT.md, DESIGN.md and docs/ARCHITECTURE.md for scope and implementation evidence.

Implementation status is recorded in docs/IMPLEMENTATION.md. The six-feature plan is authorized, still notch-only. New stateful services are in src/; media/audio adapters remain in Backend. Do not bypass notifications/validated or enable task/audio features silently. Tests must use dbus-run-session; CMake no longer falls back to the user bus. The optional CLI wrapper requires Python 3.

The main notch uses LayerShellQt LayerTop, not LayerOverlay, so active fullscreen games/video are stacked above it by KWin. Only temporary user-initiated drag targets use Overlay. Do not reintroduce a permanently game-covering overlay.
