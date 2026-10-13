#!/usr/bin/env bash
# The end-to-end check of what the mod is for: an edit in the game, its "Copy all" pasted into a mod, one build, and
# the nodes where the edit left them. In a vanilla game:
#   1. the self test's =export edits the main menu and writes the code and where the nodes are (e2e-code.cpp,
#      e2e-expected.txt in UI Tweaker's save folder)
#   2. tests/e2e builds that code into a mod of its own (zhulis.ui-tweaker-e2e), with a check
#   3. the game runs once with it (UI Tweaker there, its edit mode never on), the check's result read from the log
#   4. that mod, its save folder and its unzipped copy are taken out of the game again, whatever happened
#   tools/e2e.sh
set -u
root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"
exe="$(geode profile list | grep '^\*' | sed -E 's/.*path = (.*) \]$/\1/')"
game="$(cygpath -u "$(dirname "$exe")")"
logs="$game/geode/logs"
saves="$LOCALAPPDATA/GeometryDash/geode/mods"
id="zhulis.ui-tweaker-e2e"

echo "== 1. the edit, exported"
code="$saves/zhulis.ui-tweaker/e2e-code.cpp"
expected="$saves/zhulis.ui-tweaker/e2e-expected.txt"
# An earlier run's files would check old code: only this run's count
rm -f "$code" "$expected"
out="$(tools/vanilla.sh tools/check.sh "" 300 export)"
echo "$out" | grep -E "RESULT|FAIL"
echo "$out" | grep -q "RESULT: PASS" || { echo "FAIL: the export didn't pass"; exit 1; }
[ -s "$code" ] && [ -s "$expected" ] || { echo "FAIL: nothing exported"; exit 1; }

echo "== 2. built into a mod"
win() { cygpath -m "$1"; }
if [ ! -f build-e2e/build.ninja ]; then
  cmake -S tests/e2e -B build-e2e -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ -DE2E_CODE="$(win "$code")" -DE2E_EXPECTED="$(win "$expected")" >/dev/null
fi
# The code changes from run to run: built again
touch tests/e2e/main.cpp
if cmake --build build-e2e 2>&1 | grep -E "error|FAILED"; then
  echo "FAIL: the exported code doesn't build"
  exit 1
fi

echo "== 3. the game with it"
running() { tasklist //FI "IMAGENAME eq GeometryDash.exe" 2>/dev/null | grep -q GeometryDash; }
run="$(mktemp)"
cleanup() {
  taskkill //IM GeometryDash.exe //F >/dev/null 2>&1
  for _ in $(seq 1 30); do running || break; sleep 1; done
  rm -f "$game/geode/mods/$id.geode" "$run"
  rm -rf "$saves/$id" "$game/geode/unzipped/$id"
  # Left in the game, it would load with the next normal start: said loudly
  for left in "$game/geode/mods/$id.geode" "$game/geode/unzipped/$id"; do
    [ -e "$left" ] && echo "WARNING: $left is still there, take it out by hand"
  done
}
trap cleanup EXIT
cp "build-e2e/$id.geode" "$game/geode/mods/"
cat > "$run" <<EOF
before="\$(ls -t "$logs" | head -1)"
geode run --background >/dev/null 2>&1
for _ in \$(seq 1 120); do [ "\$(ls -t "$logs" | head -1)" != "\$before" ] && break; sleep 1; done
log="$logs/\$(ls -t "$logs" | head -1)"
for _ in \$(seq 1 120); do grep -q "E2E PASS\|E2E FAIL" "\$log" && break; sleep 1; done
grep -E "E2E" "\$log" | cut -c1-200
taskkill //IM GeometryDash.exe //F >/dev/null 2>&1
EOF
result="$(VANILLA_ALSO="$id" tools/vanilla.sh bash "$run")"
echo "$result" | grep -v "^vanilla:"
echo "$result" | grep -q "E2E PASS" && echo "RESULT: PASS" || echo "RESULT: FAIL"
