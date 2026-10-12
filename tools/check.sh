#!/usr/bin/env bash
# The stage 1 check: the game started with UI_TWEAKER_SELFTEST=1, the self test's result from Geode's log,
# a screenshot of the game with the edit mode on, the game closed.
#   tools/check.sh [screenshot.png] [seconds to wait] [UI_TWEAKER_SELFTEST's value, 1 by default]
# The mod has to be installed (geode build). Git Bash on Windows
set -u

shot="${1:-}"
timeout="${2:-150}"
mode="${3:-1}"
here="$(cd "$(dirname "$0")" && pwd)"

# The game of Geode CLI's current profile
exe="$(geode profile list | grep '^\*' | sed -E 's/.*path = (.*) \]$/\1/')"
game="$(cygpath -u "$(dirname "$exe")")"
logs="$game/geode/logs"

newest() { ls -t "$1" 2>/dev/null | head -1; }
running() { tasklist //FI "IMAGENAME eq GeometryDash.exe" 2>/dev/null | grep -q GeometryDash; }
stop() {
  taskkill //IM GeometryDash.exe //F >/dev/null 2>&1
  for _ in $(seq 1 30); do running || return 0; sleep 1; done
}
wait_for() {
  for _ in $(seq 1 "$2"); do eval "$1" && return 0; sleep 1; done
  return 1
}

stop
before="$(newest "$logs")"
UI_TWEAKER_SELFTEST="$mode" geode run --background >/dev/null 2>&1
if ! wait_for '[ "$(newest "$logs")" != "$before" ]' 60; then
  echo "FAIL: no new Geode log in 60 s"
  stop
  exit 1
fi
log="$logs/$(newest "$logs")"
echo "log: $log"

if ! wait_for 'grep -q "Self test PASS\|Self test FAIL: [0-9]" "$log"' "$timeout"; then
  echo "FAIL: no self test result in $timeout s"
  running || echo "the game isn't running: crashed?"
  tail -20 "$log"
  stop
  exit 1
fi
# A few frames for the overlay to draw the selection
sleep 3
if [ -n "$shot" ]; then
  powershell -NoProfile -ExecutionPolicy Bypass -File "$(cygpath -w "$here/screenshot.ps1")" -out "$(cygpath -w "$shot")"
fi
running && echo "the game is running" || echo "FAIL: the game isn't running"

echo "== the self test's lines"
grep -E "zhulis.ui-tweaker|ui-tweaker|UI Tweaker" "$log" | grep -E "Self test|Edit mode|ERROR|error" | cut -c1-200
echo "== the generated code"
awk '/Self test: the code for/{p=1} /Self test: [0-9]+ steps undone/{p=0} p' "$log" | head -80
echo "== errors from any mod"
grep -E "\[ERROR\]|\] ERROR" "$log" | head -10
stop
grep -q "Self test PASS" "$log" && echo "RESULT: PASS" || echo "RESULT: FAIL"
