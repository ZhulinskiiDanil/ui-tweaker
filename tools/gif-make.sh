#!/usr/bin/env bash
# The README's third GIF: in a vanilla main menu, the title selected with a click (a sprite outside layouts: in a
# menu with a layout the new one would join its row), the code view on,
# Ctrl+D making one like it, the new one dragged where it goes while the panel shows the code that makes it,
# undone; frames taken meanwhile, with real mouse and keys.
#   tools/gif-make.sh <work folder> <out.gif>
# Run it in a vanilla game (tools/vanilla.sh tools/gif-make.sh ...). SETTLE as in gif.sh
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
UI_TWEAKER_SELFTEST=code UI_TWEAKER_LANG="${LANG_OVERRIDE:-en}" geode run --background >/dev/null 2>&1
for _ in $(seq 1 60); do [ "$(ls -t "$logs" | head -1)" != "$before" ] && break; sleep 1; done
log="$logs/$(ls -t "$logs" | head -1)"
for _ in $(seq 1 150); do grep -q "Self test PASS\|Self test FAIL: [0-9]" "$log" && break; sleep 1; done
start=$(wc -l < "$log")
sleep "${SETTLE:-8}"
ps input.ps1 key esc
ps input.ps1 move 400 200
ps frames.ps1 -out "$(cygpath -w "$work/frames")" -count 120 -every 50 &
frames=$!
sleep 0.5
# The title selected, its code shown (the self test's =code leaves the code view on)
ps input.ps1 move 284.5 270; ps input.ps1 down; ps input.ps1 up
sleep 0.8
# One like it, made next to it, dragged where it goes
ps input.ps1 key d ctrl
sleep 0.8
ps input.ps1 drag 292.5 262 300 190
sleep 2.0
ps input.ps1 key z ctrl
sleep 0.4
ps input.ps1 key z ctrl
sleep 0.8
wait $frames
tail -n +"$((start + 1))" "$log" | grep -E "Selected|Made|Dragged|Undid" | cut -c1-140
stop
python "$(cygpath -w "$here/make_gif.py")" "$(cygpath -w "$work/frames")" "$(cygpath -w "$out")" 760 "${CUT:-0.04}"
