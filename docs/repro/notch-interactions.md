# Notch interaction regressions — 2026-10-31

User reports: cannot drag to another display, choosing either display does not move the island, settings are hard to find, island expands before clicking, and button icons are missing on KDE.

## Reproduction and ledger

1. Existing application builds and the original suite runs. Current preferences contain only `screen=DP-1`; there is no explicit hover preference. Thus saved opt-in cannot explain expansion.
2. Two isolated regression cases fail before changes:

```
./build/test-backend -platform offscreen clickOnlyByDefault settingsDiscoverableWhileCollapsed
FAIL! BackendTest::clickOnlyByDefault() '!backend.hoverExpand()' returned FALSE.
FAIL! BackendTest::settingsDiscoverableWhileCollapsed() 'settings' returned FALSE.
```

3. GDB breakpoint in Backend::hoverExpand shows it returning `m_settings.value("hoverExpand", true)`. QML enterDelay expands after 220 ms. No click is needed. This falsifies the alternative that expansion is caused solely by the launcher or focus.
4. A screenshot using `QT_QUICK_CONTROLS_STYLE=org.kde.desktop QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software` reproduces missing transport/settings/collapse icons. The same bundled SVGs display with the ordinary style, falsifying missing asset files as the sole explanation. The platform Button content delegate is the failing path.
5. Display selection calls setScreen on an existing layer surface. The layer-shell protocol chooses its output in get_layer_surface; changing Qt's QScreen does not retarget that role. The user's visible failure contradicts the earlier check that only reported QWindow::screen().
6. Corrected Wayland protocol capture: output#30 is DP-2, output#28 is DP-1. The cycle check creates get_layer_surface on #30, then #28, then #30. Final exposure is true, input mask height32, keyboard interactivity0, screen DP-2. This verifies native surface recreation rather than relying on the Qt screen name alone.
7. New actual-QML tests verify a collapsed settings button, direct SVG Image.Ready, settings click activation, no expansion after 350 ms of hover, expansion after a click, bounded offsets, invalid display rejection, and loading of native drag-target QML. Full suite passes in both ordinary and KDE desktop control styles.

## Corrections

Hover is opt-in. Compact size is244×32. Settings are visible in compact form and have an in-island page. Project SVGs render directly through Image. Display changes destroy and remap the native layer surface. Native QDrag temporarily shows per-display drop surfaces and commits the display only after a valid drop.

## Limits

Actual monitor remapping is verified from Wayland protocol. Native drag gesture and input across physical displays still need hands-on validation; automated tests establish the drop UI loads and selection path works, not a physical mouse drag. No real media playback or audio levels are changed by tests.
