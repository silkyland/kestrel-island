---
name: Kestrel Island
description: A quiet Dynamic Island attached to the top edge of a KDE desktop
colors:
  surface: "#080809"
  raised: "#202023"
  text: "#f5f5f7"
  secondary: "#a7a7af"
  accent: "#b8a1ff"
  timer: "#ffc078"
typography:
  title:
    fontFamily: system sans
    fontSize: 18px
    fontWeight: 600
  body:
    fontFamily: system sans
    fontSize: 12px
    fontWeight: 400
  timer:
    fontFamily: system sans
    fontSize: 44px
    fontWeight: 500
rounded:
  artwork: 0.19x size
  lower-corner: 26px
spacing:
  content: 34px
  small: 8px
  medium: 16px
---

# Kestrel Island

## Overview

Operate mode, native desktop overlay. The user's explicit reference is a Dynamic Island: black, attached to the top center, with concave shoulders and rounded lower corners. Compact status expands into controls without a dashboard.

## Colors

A near-black surface with off-white text and secondary gray. Lavender identifies playback and sliders; warm amber identifies the timer. Raised controls use tonal layering. The cover tint drives track-scoped parts (compact bars, timeline fill, selected-tab ring, play fill) and is darkened so white icons keep >=4.5:1 contrast; neutral covers fall back to lavender. Primary text actions invert to dark text on accent.

## Typography

Use the desktop's system sans. Body and tab text are 12 px, metadata titles 18 px, focus headings 20 px, and countdown 44 px with tabular numerals. Elide long titles rather than resizing the notch.

## Layout

The transparent window is 588×594 logical pixels. The visible island stays 288×40 when compact. Expanded width is 456, with Music height 304, Settings height 360, Focus/Tasks/Inbox height 384, Keep awake height 336 and Tools height 288; System expands to 560×568 to show CPU, available RAM, both GPUs and disk capacity. Content has 34 px side insets. Four icon tabs (note, timer, chip, grid) widen into labelled pills when selected and share a volume footer. Tools makes Keep awake, Tasks and Inbox directly discoverable. Hover reopening remembers the chosen page. System uses flat divided rows with explicit units and real CPU history, never placeholder readings. Extra devices scroll.

## Elevation & Depth

No decorative shadow or blur. Black remains the defining material; control states are tonal.

## Shapes

14 px concave shoulders attach the notch to the top edge. Lower corners use a radius of 26 px, clamped to half the height when compact. Pointer region and visible geometry must remain identical. Artwork uses a 0.19×size radius with a 0.5 px inner hairline; utility buttons are 10 px rounded-rects and transport buttons stay circular.

## Components

The compact notch shows the selected activity or highest-priority expiring notice and playback-status bars. Selected media uses a 24 px center-cropped local artwork thumbnail, decoded at 48 px; missing, unreadable or non-media activity uses the 20 px island icon. Notices do not borrow unrelated album art. Activity cycling preserves the single context view. Bars indicate playback; they are not a spectrum analyzer. The user prefers a click-driven model: hovering the compact notch plays a small peek animation (the island nods to 54 px and recentres its content, after ~120 ms) but never opens the window. Clicking expands and implicitly pins, so pointer leave alone never closes it — only the collapse button, Escape, tray toggle or an explicit unpin followed by pointer leave does. Pin is in-memory and clears on explicit collapse. The hover animation is optional and honours the app's motion setting. A compact settings icon opens an in-island settings page. Dragging the compact title starts native per-display drop targets. Expansion uses 280 ms OutCubic and opacity uses 100–140 ms. The "Animate transitions" setting disables all motion; KDE's AnimationDurationFactor scales durations (Qt 6 exposes no reduced-motion hint): the factor is read once at startup and not watched live — changing it takes effect on the next launch, while the app flag applies immediately. 0 disables motion and other values are clamped to [0.25, 1]. Countdown digits roll per digit. Activity notices never force expansion. Music adds a capability-aware seek timeline; muted microphone artwork remains visually distinct.

Music controls respect MPRIS capabilities. Empty state says how to start playback. Focus offers a custom countdown and Pomodoro work/short-break/long-break phases. Completion stays visible until dismissal; the next phase starts explicitly. System shows real CPU and memory. The visible volume footer is disabled when audio is unavailable. Icon controls have names, tooltips, focus rings, and disabled states.

The in-island settings page controls display, top offset and hover expansion. The tray menu additionally controls animations, startup and exit. Render bundled button SVGs directly with Image so the KDE platform delegate cannot hide them. Collapsing disables keyboard interactivity; an active native surface briefly unmaps/remaps to release KWin focus. Explicit collapse suppresses hover reopening until the pointer leaves. Expanded controls use on-demand focus.

## Do's and Don'ts

- Preserve the user's black-notch form and compact footprint.
- Show live data or an honest empty state.
- Keep transparent pixels click-through and reserve no workspace area.
- Keep metadata bounded and local artwork optional.
- Do not add a dashboard, invented metrics, remote artwork requests or upstream branding.
