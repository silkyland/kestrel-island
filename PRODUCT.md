# Kestrel Island

<!-- impeccable:product-schema 1 -->

## Platform

Native Linux desktop, KDE Plasma first, Wayland layer shell; X11 fallback.

## Users and purpose

The user wants only the Dynamic Island / simulated notch from Vorssaint, not a full desktop toolkit. A small black notch at the top center expands into useful controls.

## Stack

Implementation choice: C++20, Qt 6 Quick/QML, KDE LayerShellQt and session D-Bus. All are available on this CachyOS KDE Wayland machine.

## Scope

Music via MPRIS, focus timer, live CPU and memory, quick audio volume. The first surface is the island itself, with a tray menu for screen selection, positioning, startup and quitting. No accounts or telemetry. No general dashboard, clipboard manager, window manager or scratchpad.

## Brand

Working independent name: Kestrel Island. Preserve the user's explicit black notch form. Do not reuse upstream names, logos or artwork. Inspired by https://github.com/vorssaint/vorssaint-utils, not an official Linux port.

## Confirmed interaction preference

The user prefers the original 288×40 compact notch and hover-to-expand behavior, enabled by default. Only the explicit top Pin button keeps the island open; mouse leave collapses otherwise. Content clicks and navigation never pin. Preserve the later settings, icon-rendering and display-remapping fixes.

Media selection is automatic. The user requested removal of the source dropdown and browser identifiers from the music UI; display the current track and artist, with no manual source choice.

## Approved expansion (0.2)

The user approved docs/PLAN.md and requested implementation: activity arbitration, media timeline, audio/microphone feedback, Pomodoro, keep awake, explicit task hooks and gated Plasma notifications. This supersedes the initial feature list while preserving the notch-only boundary. Notifications remain unavailable pending real-session privacy validation. Other opt-in features are controlled by user settings.

## Requested system detail (0.3)

User explicitly requested real CPU/GPU use, RAM remaining and disk used/free, plus clearer navigation. These live in the notch System page, not a separate monitoring app. Keep the compact shape and hover behavior. Tools provides direct access to the existing optional features.
