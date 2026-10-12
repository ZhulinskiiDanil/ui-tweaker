#!/usr/bin/env bash
# The edit mode with real input, as a user's: the mouse and the keys through SendInput into the game's window.
# Starts the game with the self test (which leaves the edit mode on), then hovers, drags, scrolls, nudges and
# undoes the Geode button, and checks the log's lines for each. Screenshots into the folder given.
#   tools/real-input.sh <screenshot folder>
set -u
shots="$1"
here="$(cd "$(dirname "$0")" && pwd)"
exe="$(geode profile list | grep '^\*' | sed -E 's/.*path = (.*) \]$/\1/')"
logs="$(cygpath -u "$(dirname "$exe")")/geode/logs"
input() { powershell -NoProfile -ExecutionPolicy Bypass -File "$(cygpath -w "$here/input.ps1")" "$@"; }
shot() { powershell -NoProfile -ExecutionPolicy Bypass -File "$(cygpath -w "$here/screenshot.ps1")" -out "$(cygpath -w "$shots/$1")"; }
running() { tasklist //FI "IMAGENAME eq GeometryDash.exe" 2>/dev/null | grep -q GeometryDash; }
stop() { taskkill //IM GeometryDash.exe //F >/dev/null 2>&1; for _ in $(seq 1 30); do running || return 0; sleep 1; done; }
newest() { ls -t "$1" 2>/dev/null | head -1; }

stop
before="$(newest "$logs")"
UI_TWEAKER_SELFTEST=1 geode run --background >/dev/null 2>&1
for _ in $(seq 1 60); do [ "$(newest "$logs")" != "$before" ] && break; sleep 1; done
log="$logs/$(newest "$logs")"
for _ in $(seq 1 150); do grep -q "Self test PASS\|Self test FAIL: [0-9]" "$log" && break; sleep 1; done
grep -E "Self test (PASS|FAIL)" "$log" | cut -c1-120
# Only what comes after the self test counts: it drags and scales the same button itself
start=$(wc -l < "$log")
fresh() { tail -n +"$((start + 1))" "$log"; }
# Where the Geode button is, from the panel's line in the self test: its middle on the screen
read -r gx gy < <(grep -o "the code for geode.loader/geode-button" "$log" >/dev/null && echo "448.5 20.5")
sleep 2

passed=0; failed=0
expect() {
  if fresh | grep -q "$1"; then echo "ok: $2"; passed=$((passed + 1)); else echo "FAIL: $2"; failed=$((failed + 1)); fi
}

input move 300 200; input move "$gx" "$gy"; sleep 0.5; shot hover.png
input drag "$gx" "$gy" 400 140; sleep 1.5
expect "Dragged geode.loader/geode-button" "a real drag moved the Geode button"
input wheel 2; sleep 1.5
expect "Scaled geode.loader/geode-button" "the real wheel scaled it"
fresh | grep "Scaled geode.loader/geode-button" | tail -1 | cut -c1-160
input key right shift; sleep 1.5
expect "Nudged geode.loader/geode-button" "Shift+Right nudged it"
fresh | grep "Nudged geode.loader/geode-button" | cut -c1-160
shot edited.png
input key a; sleep 0.5; shot shapes.png; input key a
input key z ctrl; input key z ctrl; input key z ctrl; sleep 1.5
undos=$(fresh | grep -c "Undid a step of geode.loader/geode-button")
[ "$undos" -ge 3 ] && { echo "ok: 3 real Ctrl+Z"; passed=$((passed + 1)); } || { echo "FAIL: $undos Ctrl+Z seen"; failed=$((failed + 1)); }
fresh | grep "Undid a step of geode.loader/geode-button" | tail -1 | cut -c1-160
# The tree: T, / and a search, Enter selects the first match
input key esc; input key t; sleep 0.5; shot tree.png
input key slash; input type "geode-button"; sleep 0.3; shot tree-search.png; input key enter; sleep 1.5
expect "Selected geode.loader/geode-button" "the tree's search and Enter selected the Geode button"
input key t
# A click in the edit mode doesn't reach the game: the Geode button doesn't open the mods list
input move "$gx" "$gy"; input down; input up; sleep 1.5; shot click-in-edit-mode.png
input key esc; input key f8; sleep 1.5
expect "Edit mode off" "F8 left the edit mode"
running && echo "the game is running" || echo "FAIL: the game isn't running"
echo "== the log after the self test"
fresh | grep -E "UI Tweaker" | cut -c1-170
echo "passed $passed, failed $failed"
stop
