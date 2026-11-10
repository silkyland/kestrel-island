---
name: Kestrel Island
description: A quiet Dynamic Island attached to the top edge of a KDE desktop
colors:
  surface: "#080809"
  raised: "#202023"
  text: "#f5f5f7"
  secondary: "#a7a7af"
  accent: "#b8a1ff"
  timer: "#f5a524"
typography:
  title:
    fontFamily: system sans
    fontSize: 14px
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

A near-black surface with off-white text and secondary gray. Lavender identifies sliders and focus; warm amber identifies the timer. Raised controls use tonal layering. The cover tint drives only the player's track-scoped parts — compact bars, the live bars beside the title and the seek timeline — while transport glyphs stay white; neutral covers fall back to lavender. Selection is a 12% white plate, never an accent ring: the tint ring means keyboard focus only. Primary text actions invert to dark text on accent.

## Typography

Use the desktop's system sans. Body and tab text are 12 px, the player's title 14 px semibold over a 13 px secondary artist line, focus headings 20 px, the Focus setup clock 34 px and the running clock 46 px, all tabular. Timeline times are 10 px tabular. Elide long titles rather than resizing the notch.

## Layout

The transparent window is 588×594 logical pixels. The visible island stays 288×40 when compact. Expanded width is 456, and 424 for the Music player so its 356 px of content keep the header's two groups within 72 px of each other; the Music player measures 254 with a timeline and 216 when no track offers one, Settings 400, Focus 216 (240 in Pomodoro), Tasks/Inbox 384, Keep awake 336 and Tools 288; System expands to 520×432 for its metric-card grid and to 560×568 while the Details disclosure is open, showing CPU, available RAM, both GPUs and disk capacity at a glance. Content has 34 px side insets. Four icon tabs (note, timer, chip, grid) widen into labelled pills at the leading edge; Pin, Settings and Collapse stay on the trailing edge, so the header reads as two anchored groups rather than one right-heavy strip. The Music page is one player block: a 56 px cover beside the title and artist, a 38 px timeline band (elapsed left, remaining right), a 54 px transport band of five equal places (shuffle, previous, play, next, repeat, with play kept centred), and the 28 px audio footer 10 px below it. The footer therefore lands 14 px above the lower edge instead of floating behind a full-width divider. The Focus page is an amber timer: a Countdown/Pomodoro switch with a 2 px underline and a 28 px Start capsule in the header, a draggable minute ruler (4 px amber ticks, every fifth minute labelled, a pointer under the selection) beside the 34 px setup clock, and one quiet break row in Pomodoro. Running sessions replace the setup with the 46 px clock and a Pause capsule. The microphone action is also available in Settings when feedback is enabled. Tools makes Keep awake, Tasks and Inbox directly discoverable. Hover reopening remembers the chosen page. System keeps flat divided rows with explicit units and real CPU history inside Details, never placeholder readings. Extra devices scroll.

## Elevation & Depth

No decorative shadow or blur. Black remains the defining material; control states are tonal.

## Shapes

14 px concave shoulders attach the notch to the top edge. Lower corners use a radius of 26 px, clamped to half the height when compact. Pointer region and visible geometry must remain identical. Artwork uses a 0.19×size radius with a 0.5 px inner hairline; utility buttons are 10 px rounded-rects and transport buttons stay circular.

## Components

The compact notch shows the selected activity or highest-priority expiring notice and playback-status bars. Selected media uses a 24 px center-cropped local artwork thumbnail, decoded at 48 px; missing, unreadable or non-media activity uses the 20 px island icon. Notices do not borrow unrelated album art. Activity cycling preserves the single context view. Bars indicate playback; they are not a spectrum analyzer. The user prefers a click-driven model: hovering the compact notch plays a small peek animation (the island nods to 54 px and recentres its content, after ~120 ms) but never opens the window. Clicking expands and implicitly pins, so pointer leave alone never closes it — only the collapse button, Escape, tray toggle or an explicit unpin followed by pointer leave does. Pin is in-memory and clears on explicit collapse. The hover animation is optional and honours the app's motion setting. A compact settings icon opens an in-island settings page. Dragging the compact title starts native per-display drop targets. Expansion uses 280 ms OutCubic and opacity uses 100–140 ms. The "Animate transitions" setting disables all motion; KDE's AnimationDurationFactor scales durations (Qt 6 exposes no reduced-motion hint): the factor is read once at startup and not watched live — changing it takes effect on the next launch, while the app flag applies immediately. 0 disables motion and other values are clamped to [0.25, 1]. Countdown digits roll per digit. Keyboard-layout switches and newly received notifications arrive as card notices: the 288×40 frame never grows — the compact controls fade out and only the event icon and title cross-fade in for ~1.6 s before the compact row returns; the title then stays as a compact notice until expiry. Other fresh notices nod the compact surface once (316×54, ~800 ms). All of it runs through the same motion gate, and activity notices never force the real expanded state. Music adds a capability-aware seek timeline; muted microphone artwork remains visually distinct.

Music controls respect MPRIS capabilities, including Shuffle and LoopStatus when the player publishes them; otherwise those buttons stay out of their slots. The empty state keeps the transport dimmed and says how to start playback. Focus sets the countdown on the amber ruler (1–180 minutes, also wheel/arrow keys) and offers Pomodoro work/short-break/long-break phases with two break steppers. Completion stays visible until dismissal; the next phase starts explicitly. System shows real CPU and memory as glanceable metric cards — title, symbol, large value, a 3 px meter and a subtitle that folds in the device temperature; a card turns its value and meter amber at ≥85% load or <10% free disk, and the detailed divided rows stay behind a Details disclosure. The audio footer (mute, level, percentage and, when enabled, microphone mute) is disabled when audio is unavailable. Icon controls have names, tooltips, focus rings, and disabled states.

The player's cover is 56 px with a 0.19× radius and a 0.5 px hairline; a missing cover shows the note glyph at 45% opacity and scales to 0.94 while paused. Transport is glyph-only: previous/next 19 px, play/pause 26 px, shuffle/repeat 16 px on a 36 px circular plate only while active (12% white). A control the player cannot drive is 40% opaque and never re-coloured to look live. The timeline bar is 4 px with its times outside; the seek handle is 10 px. Nothing in the player uses a filled accent circle, so the cover tint never competes with the artwork.

Every colour, radius and duration comes from the `Theme` singleton (`qml/Theme.qml`); there are no per-page hex literals. Value controls are island-owned dark templates — IslandStepper (bounded −/+/value, Up/Down keys), IslandCheck (check and toggles) and IslandSelect (closed face + delegate + popup) — so no light KDE control face appears on the black surface. Each keeps Accessible names, a Theme.tint focus ring and strict bounds. Scrolling pages fade their edges only while more content lies beyond them (EdgeFade, 16 px surface-to-transparent, no shaders). Escape closes the inner layer first: the Keep awake/Tasks/Inbox pages step back to Tools and Escape on any other page collapses the island. The compact title cross-fades for 140 ms on activity or notice changes and uses tabular figures.

The in-island settings page controls display, top offset and hover expansion. The tray menu additionally controls animations, startup and exit. Render bundled button SVGs directly with Image so the KDE platform delegate cannot hide them. Collapsing disables keyboard interactivity; an active native surface briefly unmaps/remaps to release KWin focus. Explicit collapse suppresses hover reopening until the pointer leaves. Expanded controls use on-demand focus.

## Do's and Don'ts

- Preserve the user's black-notch form and compact footprint.
- Show live data or an honest empty state.
- Keep transparent pixels click-through and reserve no workspace area.
- Keep metadata bounded and local artwork optional.
- Keep the player's four bands exact (56 + 38 + 54 + 38) so the footer never floats.
- Do not add a dashboard, invented metrics, remote artwork requests or upstream branding.
- Do not re-introduce a filled accent play circle or a selected-tab accent ring.
