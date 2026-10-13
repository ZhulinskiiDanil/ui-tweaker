#!/usr/bin/env bash
# The README's second GIF: the three big buttons of a vanilla main menu (the icon kit, play, the editor) selected
# with a click and Shift+clicks, lined up by their tops (Alt+W), undone, by their bottoms (Alt+S), undone, copied as
# a layout (L); frames taken meanwhile, with real mouse and keys.
#   tools/gif-align.sh <work folder> <out.gif>
# Run it in a vanilla game (tools/vanilla.sh tools/gif-align.sh ...). SETTLE as in gif.sh
set -u
work="$1"; out="$2"
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
start=$(wc -l < "$log")
sleep "${SETTLE:-8}"
ps input.ps1 key esc
# The panel out of the way: it covers the icon kit's button where the self test left it
ps input.ps1 key h
ps input.ps1 move 300 250
ps frames.ps1 -out "$(cygpath -w "$work/frames")" -count 120 -every 50 &
frames=$!
sleep 0.5
# The icon kit, then play and the editor with Shift
ps input.ps1 move 175 170; ps input.ps1 down; ps input.ps1 up
sleep 0.4
ps input.ps1 shiftclick 284.5 170
sleep 0.3
ps input.ps1 shiftclick 394 170
sleep 0.6
ps input.ps1 key w alt
sleep 1.0
ps input.ps1 key z ctrl
sleep 0.5
ps input.ps1 key s alt
sleep 1.0
ps input.ps1 key z ctrl
sleep 0.5
ps input.ps1 key l
sleep 1.2
wait $frames
ps input.ps1 key h
tail -n +"$((start + 1))" "$log" | grep -E "Selection|Aligned|Layout for" | cut -c1-140
stop
python "$(cygpath -w "$here/make_gif.py")" "$(cygpath -w "$work/frames")" "$(cygpath -w "$out")" 760 "${CUT:-0.04}"
