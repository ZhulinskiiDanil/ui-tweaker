#!/usr/bin/env bash
# Runs a check in a vanilla game: every mod but Geode, node-ids (the IDs the code finds nodes by) and UI Tweaker
# turned off for the run, then the mods put back as they were, byte for byte, whatever the check did.
#   tools/vanilla.sh tools/check.sh shot.png 150 tour
# Geode keeps what's turned on in its loader's saved.json; the game is closed before and after. Git Bash on Windows
set -u

keep="geode.loader geode.node-ids zhulis.ui-tweaker"
saved="$LOCALAPPDATA/GeometryDash/geode/mods/geode.loader/saved.json"
exe="$(geode profile list | grep '^\*' | sed -E 's/.*path = (.*) \]$/\1/')"
mods="$(cygpath -u "$(dirname "$exe")")/geode/mods"

running() { tasklist //FI "IMAGENAME eq GeometryDash.exe" 2>/dev/null | grep -q GeometryDash; }
stop() {
  taskkill //IM GeometryDash.exe //F >/dev/null 2>&1
  for _ in $(seq 1 30); do running || return 0; sleep 1; done
}

stop
backup="$(mktemp)"
cp "$saved" "$backup"
restore() {
  stop
  cp "$backup" "$saved"
  rm -f "$backup"
}
trap restore EXIT

ids="$(ls "$mods" | sed -n 's/\.geode$//p')"
python - "$saved" "$keep" $ids <<'EOF'
import json, sys
path, keep, ids = sys.argv[1], sys.argv[2].split(), sys.argv[3:]
with open(path, encoding="utf-8") as f:
    data = json.load(f)
for mod in ids:
    data[f"should-load-{mod}"] = mod in keep
with open(path, "w", encoding="utf-8") as f:
    json.dump(data, f, indent=4)
print(f"vanilla: {len(keep)} of {len(ids)} mods on")
EOF

"$@"
