#!/usr/bin/env bash
# The edit mode with real input, as a user's: the mouse and the keys through SendInput into the game's window.
# Starts the game with the self test (which leaves the edit mode on), then hovers, drags, scrolls, nudges and
# undoes the Geode button, and checks the log's lines for each. Screenshots into the folder given.
#   tools/real-input.sh <screenshot folder>
set -u
shots="$1"
mkdir -p "$shots"
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
# In a vanilla main menu: the Geode button's middle, and the title, a node outside layouts (a layout guard of
# another mod could put a menu's buttons back every frame, and a drag of them would leave no step). Other mods
# changing the main menu move these
gx=330.75; gy=45
tx=284.5; ty=270
# The play button's middle and its right side's handle (109.5 wide): the title is a sprite, with no handles
px=284.5; py=170; hx=339.25; hy=170
sleep 2

passed=0; failed=0
expect() {
  if fresh | grep -q "$1"; then echo "ok: $2"; passed=$((passed + 1)); else echo "FAIL: $2"; failed=$((failed + 1)); fi
}

input move 300 200; input move "$gx" "$gy"; sleep 0.5; shot hover.png
# Nothing selected: the panel is short and doesn't cover the title
input key esc
input drag "$tx" "$ty" 260 200; sleep 1.5
expect "Dragged " "a real drag moved the title"
fresh | grep "Dragged " | tail -1 | cut -c1-160
input wheel 2; sleep 1.5
expect "Scaled " "the real wheel scaled it"
fresh | grep "Scaled " | tail -1 | cut -c1-160
input key right shift; sleep 1.5
expect "Nudged " "Shift+Right nudged it"
fresh | grep "Nudged " | cut -c1-160
shot edited.png
input key a; sleep 0.5; shot shapes.png; input key a
input key z ctrl; input key z ctrl; input key z ctrl; sleep 1.5
undos=$(fresh | grep -c "Undid a step of ")
[ "$undos" -ge 3 ] && { echo "ok: 3 real Ctrl+Z"; passed=$((passed + 1)); } || { echo "FAIL: $undos Ctrl+Z seen"; failed=$((failed + 1)); }
fresh | grep "Undid a step of " | tail -1 | cut -c1-160
# Values typed for the selected title: Enter, w/2 Tab h/2 Enter
input key enter; input key w; input key slash; input key 2; input key tab; input key h; input key slash; input key 2
sleep 0.3; shot typing.png; input key enter; sleep 1.5
expect "Typed " "real keys typed the values w/2, h/2"
fresh | grep "Typed " | tail -1 | cut -c1-160
input key z ctrl
# Ctrl+wheel turns it
input ctrlwheel 1; sleep 1.5
expect "Turned main-title" "Ctrl+wheel turned the title"
input key z ctrl
# Ctrl+D makes one like it
input key d ctrl; sleep 1.5
expect "Made zhulis.ui-tweaker/new-" "Ctrl+D made a title like it"
input key z ctrl
# The play button selected, its right side dragged by its handle
input move "$px" "$py"; input down; input up; sleep 0.5
input drag "$hx" "$hy" "$(python -c "print($hx + 20)")" "$hy"; sleep 1.5
expect "Resized " "a real drag of a handle resized the play button"
fresh | grep "Resized " | tail -1 | cut -c1-160
input key z ctrl
# Shift+click: the Geode button added to the selected play button
input shiftclick "$gx" "$gy"; sleep 1.5
expect "Selection: 2 nodes" "Shift+click selected a second node"
shot several.png
# Alt+A lines the two up by their left edges, as in Figma (Alt comes as a system key on Windows)
input key a alt; sleep 1.5
expect "Aligned " "Alt+A aligned the two selected nodes"
fresh | grep "Aligned " | tail -1 | cut -c1-160
shot aligned.png
input key z ctrl
# The tree: T, / and a search, Enter selects the first match
input key esc; input key t; sleep 0.5; shot tree.png
input key slash; input type "geode-button"; sleep 0.3; shot tree-search.png; input key enter; sleep 1.5
expect "Selected geode.loader/geode-button" "the tree's search and Enter selected the Geode button"
# By what a node shows: the play button's frame
input key slash; input type "playbtn"; sleep 0.3; input key enter; sleep 1.5
expect "Selected play-button" "the tree found the play button by its frame's name"
input key t
# A click in the edit mode doesn't reach the game: the Geode button doesn't open the mods list
input move "$gx" "$gy"; input down; input up; sleep 1.5; shot click-in-edit-mode.png
input key esc; input key f8; sleep 1.5
expect "Edit mode off" "F8 left the edit mode"
running && echo "the game is running" || echo "FAIL: the game isn't running"
echo "== the log after the self test"
fresh | grep -E "UI Tweaker" | cut -c1-170
stop

# F8 while a text field has the keyboard (the self test's =ime leaves one focused, the edit mode off): the keybind
# allows text inputs, and the edit mode takes the keyboard from the field
before="$(newest "$logs")"
UI_TWEAKER_SELFTEST=ime geode run --background >/dev/null 2>&1
for _ in $(seq 1 60); do [ "$(newest "$logs")" != "$before" ] && break; sleep 1; done
log="$logs/$(newest "$logs")"
for _ in $(seq 1 150); do grep -q "Self test PASS\|Self test FAIL: [0-9]" "$log" && break; sleep 1; done
grep -E "Self test (PASS|FAIL)" "$log" | cut -c1-120
start=$(wc -l < "$log")
sleep 1
input key f8; sleep 1.5
expect "Edit mode on" "F8 in a text field turned the edit mode on"
expect "A text field had the keyboard: let go" "the text field let go of the keyboard"
shot text-field.png
echo "== the log after the self test"
fresh | grep -E "UI Tweaker" | cut -c1-170
echo "passed $passed, failed $failed"
stop
