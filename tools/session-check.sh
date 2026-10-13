#!/usr/bin/env bash
# The edits kept across a restart, as a mod developer has it: the game started as usual (no self test), F8, the
# title dragged with the real mouse, F8; the game closed and started again with "Put edits back" on, F8: the title
# is where it was dragged. The mod's settings and session file are put back as they were afterwards.
#   tools/session-check.sh <screenshot folder>
# In a vanilla main menu (tools/vanilla.sh tools/session-check.sh ...). Git Bash on Windows
set -u

shots="${1:-.}"
here="$(cd "$(dirname "$0")" && pwd)"
exe="$(geode profile list | grep '^\*' | sed -E 's/.*path = (.*) \]$/\1/')"
logs="$(cygpath -u "$(dirname "$exe")")/geode/logs"
saves="$LOCALAPPDATA/GeometryDash/geode/mods/zhulis.ui-tweaker"
input() { powershell -NoProfile -ExecutionPolicy Bypass -File "$(cygpath -w "$here/input.ps1")" "$@"; }
shot() { powershell -NoProfile -ExecutionPolicy Bypass -File "$(cygpath -w "$here/screenshot.ps1")" -out "$(cygpath -w "$shots/$1")"; }
running() { tasklist //FI "IMAGENAME eq GeometryDash.exe" 2>/dev/null | grep -q GeometryDash; }
stop() { taskkill //IM GeometryDash.exe //F >/dev/null 2>&1; for _ in $(seq 1 30); do running || return 0; sleep 1; done; }
newest() { ls -t "$1" 2>/dev/null | head -1; }

# The settings and the session as they are, back afterwards
stop
keep="$(mktemp -d)"
cp "$saves/settings.json" "$keep/settings.json"
# The session file and the ones the mod makes next to it (a backup, a temporary one, the forgotten edits)
names="session.txt session.txt.bak session.txt.tmp session.txt.forgotten"
for name in $names; do [ -f "$saves/$name" ] && cp "$saves/$name" "$keep/$name"; done
restore() {
  stop
  cp "$keep/settings.json" "$saves/settings.json"
  for name in $names; do
    if [ -f "$keep/$name" ]; then cp "$keep/$name" "$saves/$name"; else rm -f "$saves/$name"; fi
  done
  rm -rf "$keep"
}
trap restore EXIT
rm -f "$saves/session.txt"

# The game started as usual, the main menu given time to come
start() {
  local before log
  before="$(newest "$logs")"
  geode run --background >/dev/null 2>&1
  for _ in $(seq 1 60); do [ "$(newest "$logs")" != "$before" ] && break; sleep 1; done
  log="$logs/$(newest "$logs")"
  sleep 12
  echo "$log"
}

passed=0; failed=0
expect() {
  if grep -q "$2" "$1"; then echo "ok: $3"; passed=$((passed + 1)); else echo "FAIL: $3"; failed=$((failed + 1)); fi
}

# The title in a vanilla main menu, a sprite outside layouts
tx=284.5; ty=270

log="$(start)"
input key f8; sleep 1
input drag "$tx" "$ty" 250 230; sleep 1
input key f8; sleep 1.5
expect "$log" "Dragged main-title" "a real drag moved the title"
[ -s "$saves/session.txt" ] && { echo "ok: the session was written when the edit mode was left"; passed=$((passed + 1)); } ||
  { echo "FAIL: no session written"; failed=$((failed + 1)); }
moved="$(grep "Dragged main-title" "$log" | tail -1 | sed -E 's/.*position \(([^)]*)\).*/\1/')"
echo "moved to ($moved)"
stop

# "Put edits back" on, the game started again
python - "$saves/settings.json" <<'PY'
import json, sys
with open(sys.argv[1], encoding="utf-8") as f:
    data = json.load(f)
data["put-back"] = True
with open(sys.argv[1], "w", encoding="utf-8") as f:
    json.dump(data, f, indent=4)
PY
log="$(start)"
expect "$log" "Kept edits of 1 nodes from the last time" "the session was read when the game started"
input key f8; sleep 2
expect "$log" "Put back the kept edits of 1 nodes" "F8 put the title's edit back"
expect "$log" "Put back main-title (CCSprite): position ($moved)" "the title is where it was dragged before the restart"
shot put-back.png
input key z ctrl; sleep 1
expect "$log" "Undid a step of main-title" "Ctrl+Z took the put back edit off"
input key f8; sleep 1
echo "passed $passed, failed $failed"
