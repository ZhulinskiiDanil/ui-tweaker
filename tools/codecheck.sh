#!/usr/bin/env bash
# The generated code (every sample of tests/compile in both styles, the layouts', "Copy all" with its hooks) compiled
# against the real Geode headers, in a mod that's built and never installed (tests/geode). The first run fetches
# Geode's dependencies into build-geode (a few minutes). Git Bash on Windows, clang as `geode build --ninja` uses
set -eu
root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"
cmake --build build-tests --target generate-samples >/dev/null
mkdir -p build-geode
real="$(pwd -W)/build-geode/real.inc"
./build-tests/generate-samples build-geode/samples.inc build-geode/hooks.inc "$real"
if [ ! -f build-geode/build.ninja ]; then
  cmake -S tests/geode -B build-geode -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ -DREAL_INC="$real" >/dev/null
fi
if cmake --build build-geode 2>&1 | grep -E "error|FAILED"; then
  echo "codecheck: the generated code doesn't compile against Geode"
  exit 1
fi
echo "codecheck: the generated code compiles against Geode $(cd "$GEODE_SDK" 2>/dev/null && git describe --tags 2>/dev/null || echo)"
