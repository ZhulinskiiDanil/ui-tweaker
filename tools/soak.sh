#!/usr/bin/env bash
# The edit mode used for a while with real input: drags, the wheel, undo, the tree and the code view open, the
# game's memory before and after. tools/soak.sh [rounds]
set -u
rounds="${1:-60}"
here="$(cd "$(dirname "$0")" && pwd)"
exe="$(geode profile list | grep '^\*' | sed -E 's/.*path = (.*) \]$/\1/')"
logs="$(cygpath -u "$(dirname "$exe")")/geode/logs"
input() { powershell -NoProfile -ExecutionPolicy Bypass -File "$(cygpath -w "$here/input.ps1")" "$@"; }
memory() { powershell -NoProfile -Command "(Get-Process GeometryDash).PrivateMemorySize64 / 1MB" | tr -d '\r'; }
taskkill //IM GeometryDash.exe //F >/dev/null 2>&1; sleep 2
before="$(ls -t "$logs" | head -1)"
UI_TWEAKER_SELFTEST=1 geode run --background >/dev/null 2>&1
for _ in $(seq 1 60); do [ "$(ls -t "$logs" | head -1)" != "$before" ] && break; sleep 1; done
log="$logs/$(ls -t "$logs" | head -1)"
for _ in $(seq 1 150); do grep -q "Self test PASS\|Self test FAIL: [0-9]" "$log" && break; sleep 1; done
sleep 3
input key esc; input key t; input key c
start=$(memory)
echo "memory at the start: $start MB"
for i in $(seq 1 "$rounds"); do
  input drag 284.5 270 230 200
  input wheel 1
  input key z ctrl; input key z ctrl
  # A title like it made and taken out again (SOAK_MAKE=0: not)
  if [ "${SOAK_MAKE:-1}" = 1 ]; then input key d ctrl; input key z ctrl; fi
  input move 300 150
  if [ $((i % 20)) -eq 0 ]; then echo "round $i: $(memory) MB"; fi
done
end=$(memory)
echo "memory at the end: $end MB"
input key t; input key c
grep -c "Dragged " "$log" | xargs echo "drags logged:"
tasklist //FI "IMAGENAME eq GeometryDash.exe" | grep -q GeometryDash && echo "the game is running" || echo "FAIL: the game isn't running"
taskkill //IM GeometryDash.exe //F >/dev/null 2>&1
