#!/usr/bin/env bash
# The README's GIF: the game started with the self test (the edit mode on, English), the code view on, the node
# at (x, y) in GD's points selected, its handle at (hx, hy) dragged 30 points right if given (a sprite or a label
# has none), the node dragged from (x, y) to (x2, y2) with snapping, scaled, the steps undone; frames taken
# meanwhile. In the vanilla main menu the title (a sprite):
#   tools/gif.sh <work folder> <out.gif> 284.5 270 286 228 [hx hy]
# SETTLE: seconds to wait first for the screen's notifications (a GPU overlay's toast) to go, 8 by default
set -u
work="$1"; out="$2"; x="$3"; y="$4"; x2="$5"; y2="$6"; hx="${7:-}"; hy="${8:-}"
here="$(cd "$(dirname "$0")" && pwd)"
exe="$(geode profile list | grep '^\*' | sed -E 's/.*path = (.*) \]$/\1/')"
logs="$(cygpath -u "$(dirname "$exe")")/geode/logs"
ps() { powershell -NoProfile -ExecutionPolicy Bypass -File "$(cygpath -w "$here/$1")" "${@:2}"; }
running() { tasklist //FI "IMAGENAME eq GeometryDash.exe" 2>/dev/null | grep -q GeometryDash; }
stop() { taskkill //IM GeometryDash.exe //F >/dev/null 2>&1; for _ in $(seq 1 30); do running || return 0; sleep 1; done; }

stop
rm -rf "$work"; mkdir -p "$work"
before="$(ls -t "$logs" | head -1)"
UI_TWEAKER_SELFTEST=1 UI_TWEAKER_LANG="${LANG_OVERRIDE:-en}" geode run --background >/dev/null 2>&1
for _ in $(seq 1 60); do [ "$(ls -t "$logs" | head -1)" != "$before" ] && break; sleep 1; done
log="$logs/$(ls -t "$logs" | head -1)"
for _ in $(seq 1 150); do grep -q "Self test PASS\|Self test FAIL: [0-9]" "$log" && break; sleep 1; done
sleep "${SETTLE:-8}"
ps input.ps1 key esc
# The code view on, the mouse away
ps input.ps1 key c
ps input.ps1 move 300 150
ps frames.ps1 -out "$(cygpath -w "$work/frames")" -count 110 -every 50 &
frames=$!
sleep 0.4
# Hover the title and select it: the panel moves out of its way
for i in 0 1 2 3 4 5; do ps input.ps1 move "$(python -c "print(300 + ($x - 300) * $i / 5)")" "$(python -c "print(150 + ($y - 150) * $i / 5)")"; done
ps input.ps1 down; ps input.ps1 up
sleep 0.4
# Its handle dragged: wider
if [ -n "$hx" ]; then
  ps input.ps1 drag "$hx" "$hy" "$(python -c "print($hx + 30)")" "$hy"
  sleep 0.3
fi
# Then the whole of it, past the guides
ps input.ps1 drag "$x" "$y" "$x2" "$y2"
sleep 0.4
ps input.ps1 wheel 2
sleep 0.6
ps input.ps1 key z ctrl
sleep 0.4
ps input.ps1 key z ctrl
if [ -n "$hx" ]; then
  sleep 0.3
  ps input.ps1 key z ctrl
fi
wait $frames
stop
python "$(cygpath -w "$here/make_gif.py")" "$(cygpath -w "$work/frames")" "$(cygpath -w "$out")" 760 "${CUT:-0.04}"
