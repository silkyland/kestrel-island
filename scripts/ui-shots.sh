#!/usr/bin/env bash
# Render every expanded page offscreen into /tmp/kestrel-ui/<page>.png.
set -euo pipefail
cd "$(dirname "$0")/.."
out=/tmp/kestrel-ui
work="$out/.work"
rm -rf "$work"
mkdir -p "$work"
for page in music focus system settings awake tasks notifications tools; do
    QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software \
        ./build/kestrel-island --screenshot "$work/$page" --expanded --page "$page"
    cp "$work/$page/expanded.png" "$out/$page.png"
done
rm -rf "$work"
ls -1 "$out"/*.png
