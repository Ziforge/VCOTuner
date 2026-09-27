#!/usr/bin/env bash
#
# Full debug harness for VCOTuner.
#
# Gates, in order: static checks -> configure -> build -> unit tests -> UI smoke.
# Fail-closed: any gate failing fails the run. Nothing is skipped silently; a
# gate that cannot run reports SKIP and still counts as not-passed.
#
# Usage: tools/debug_harness.sh [--quick] [--no-ui]
#   --quick   reuse an existing build directory instead of configuring fresh
#   --no-ui   skip the launch/screenshot gate (for CI or a headless session)
#   --static-only  run only the static gates (used by tools/harness_selftest.sh)

set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$ROOT/build-harness"
ARTIFACTS="$ROOT/build-harness/artifacts"
QUICK=0
RUN_UI=1
STATIC_ONLY=0

for arg in "$@"; do
  case "$arg" in
    --quick) QUICK=1 ;;
    --no-ui) RUN_UI=0 ;;
    --static-only) STATIC_ONLY=1 ;;
    *) echo "unknown option: $arg" >&2; exit 2 ;;
  esac
done

PASS=0; FAIL=0; SKIP=0
declare -a RESULTS

gate() {  # gate <name> <status> [detail]
  local name="$1" status="$2" detail="${3:-}"
  case "$status" in
    PASS) PASS=$((PASS+1)); printf '  \033[32mPASS\033[0m  %s %s\n' "$name" "$detail" ;;
    FAIL) FAIL=$((FAIL+1)); printf '  \033[31mFAIL\033[0m  %s %s\n' "$name" "$detail" ;;
    SKIP) SKIP=$((SKIP+1)); printf '  \033[33mSKIP\033[0m  %s %s\n' "$name" "$detail" ;;
  esac
  RESULTS+=("$status|$name|$detail")
}

hdr() { printf '\n\033[1m== %s ==\033[0m\n' "$1"; }

###############################################################################
hdr "Static checks"

# Unresolved merge conflicts anywhere in the tracked tree.
if git -C "$ROOT" grep -qIn -e '^<<<<<<< ' -e '^>>>>>>> ' -- Source tests CMakeLists.txt 2>/dev/null; then
  gate "no-conflict-markers" FAIL "$(git -C "$ROOT" grep -lIn -e '^<<<<<<< ' -- Source tests | tr '\n' ' ')"
else
  gate "no-conflict-markers" PASS
fi

# Raw non-ASCII in narrow string literals reaches JUCE as Latin-1 bytes and
# renders as mojibake. Comments are fine; literals are not.
# (BSD grep has no \xNN escapes, so this is a byte scan in python instead.)
if BAD_UTF8=$(python3 "$ROOT/tools/check_literals.py" "$ROOT/Source" 2>&1); then
  gate "no-raw-utf8-literals" PASS
else
  gate "no-raw-utf8-literals" FAIL "$(echo "$BAD_UTF8" | head -2 | cut -c1-110)"
fi

# Font::getStringWidth was removed in JUCE 8; GlyphArrangement replaces it.
if grep -rn 'getCurrentFont()\.getStringWidth\|Font([^)]*)\.getStringWidth' "$ROOT/Source" >/dev/null 2>&1; then
  gate "no-juce7-font-api" FAIL "Font::getStringWidth is gone in JUCE 8"
else
  gate "no-juce7-font-api" PASS
fi

# Regression guard: drawTabAreaBehindFrontButton paints into a full-size child
# that sits in front of every inactive tab, so an opaque fill there hides them.
if awk '/drawTabAreaBehindFrontButton/,/^    \}/' "$ROOT/Source/ModernLookAndFeel.h" | grep -q 'fillRect(0, 0, w, h)'; then
  gate "tabbar-overlay-not-opaque" FAIL "opaque fill hides inactive tabs"
else
  gate "tabbar-overlay-not-opaque" PASS
fi

# Regression guard: CV output must be serviced before the audio callback's
# early returns, or the CV drops to 0 V between notes and after a sweep.
CB=$(awk '/void VCOTuner::audioDeviceIOCallbackWithContext/,/^\}/' "$ROOT/Source/VCOTuner.cpp")
CV_LINE=$(echo "$CB" | grep -n 'fillOutputBuffer' | head -1 | cut -d: -f1)
RET_LINE=$(echo "$CB" | grep -n 'return;' | head -1 | cut -d: -f1)
if [ -n "$CV_LINE" ] && [ -n "$RET_LINE" ] && [ "$CV_LINE" -lt "$RET_LINE" ]; then
  gate "cv-before-early-returns" PASS "fill@$CV_LINE < return@$RET_LINE"
elif [ -z "$CV_LINE" ]; then
  gate "cv-before-early-returns" FAIL "no CV fill in the audio callback"
else
  gate "cv-before-early-returns" FAIL "CV fill@$CV_LINE is after return@$RET_LINE"
fi

# Every conversion from a period in samples to a frequency in Hz must go
# through the measured clock, not the rate the device claims. Reverting one of
# these reintroduces a systematic error no amount of averaging removes.
NOMINAL_USE=$(grep -n 'fitFrequency(detector, sampleRate' "$ROOT/Source/VCOTuner.cpp" || true)
NOMINAL_CM=$(awk '/computeMeasurement\(/,/\);/' "$ROOT/Source/VCOTuner.cpp" | grep -c '^ *sampleRate,' || true)
if [ -n "$NOMINAL_USE" ] || [ "${NOMINAL_CM:-0}" -gt 0 ]; then
  gate "frequency-uses-corrected-clock" FAIL "a conversion still divides by the nominal rate"
else
  gate "frequency-uses-corrected-clock" PASS
fi

# The JUCE submodule must match what the tree pins, or the build is not the
# build the gates think they are testing.
PINNED=$(git -C "$ROOT" ls-files -s deps/JUCE | awk '{print $2}')
ACTUAL=$(git -C "$ROOT/deps/JUCE" rev-parse HEAD 2>/dev/null || echo none)
if [ "$PINNED" = "$ACTUAL" ]; then
  gate "juce-submodule-in-sync" PASS "$(git -C "$ROOT/deps/JUCE" describe --tags 2>/dev/null || echo "$ACTUAL" | cut -c1-8)"
else
  gate "juce-submodule-in-sync" FAIL "pinned ${PINNED:0:8} != checked out ${ACTUAL:0:8}"
fi

if [ "$STATIC_ONLY" = "1" ]; then
  hdr "Summary"
  printf '  %d passed, %d failed, %d skipped (static only)\n' "$PASS" "$FAIL" "$SKIP"
  [ "$FAIL" -gt 0 ] && exit 1 || exit 0
fi

###############################################################################
hdr "Configure"

# Homebrew's arm-none-eabi-gcc wins CMake's compiler search on this machine and
# fails the ABI test with "unrecognized command-line option '-arch'". Pin Apple
# clang explicitly rather than depending on PATH order.
CMAKE_ARGS=(
  -S "$ROOT" -B "$BUILD"
  -DCMAKE_BUILD_TYPE=Release
  -DVCOTUNER_BUILD_TESTS=ON
  -DCMAKE_C_COMPILER=/usr/bin/clang
  -DCMAKE_CXX_COMPILER=/usr/bin/clang++
  -DCMAKE_AR=/usr/bin/ar
  -DCMAKE_RANLIB=/usr/bin/ranlib
)

if [ "$QUICK" = "1" ] && [ -f "$BUILD/CMakeCache.txt" ]; then
  gate "cmake-configure" PASS "reused $BUILD (--quick)"
else
  rm -rf "$BUILD"
  if cmake "${CMAKE_ARGS[@]}" > "$ROOT/.harness-configure.log" 2>&1; then
    gate "cmake-configure" PASS
  else
    gate "cmake-configure" FAIL "see .harness-configure.log"
    tail -15 "$ROOT/.harness-configure.log"
  fi
fi

###############################################################################
hdr "Build"

if [ -f "$BUILD/CMakeCache.txt" ]; then
  if cmake --build "$BUILD" --config Release -j "$(sysctl -n hw.ncpu)" > "$ROOT/.harness-build.log" 2>&1; then
    NWARN=$(grep -c 'warning:' "$ROOT/.harness-build.log" || true)
    gate "build-app-and-tests" PASS "$NWARN compiler warnings"
  else
    gate "build-app-and-tests" FAIL "see .harness-build.log"
    grep -i 'error:' "$ROOT/.harness-build.log" | head -10
  fi
else
  gate "build-app-and-tests" SKIP "no configured build dir"
fi

APP="$BUILD/VCOTuner_artefacts/Release/VCOTuner.app"
[ -d "$APP" ] || APP="$BUILD/VCOTuner_artefacts/VCOTuner.app"

if [ -d "$APP" ]; then
  gate "app-bundle-produced" PASS "$(basename "$APP")"
else
  gate "app-bundle-produced" FAIL "no .app under $BUILD"
fi

###############################################################################
hdr "Unit tests"

if [ -x "$BUILD/tests/VCOTunerTests" ]; then
  if ctest --test-dir "$BUILD" --output-on-failure > "$ROOT/.harness-ctest.log" 2>&1; then
    SUMMARY=$(grep -E '^[0-9]+% tests passed' "$ROOT/.harness-ctest.log" | head -1)
    ASSERTS=$("$BUILD/tests/VCOTunerTests" 2>/dev/null | grep -Eo '[0-9]+ assertions' | head -1)
    gate "unit-tests" PASS "${SUMMARY:-all passed} (${ASSERTS:-?})"
  else
    gate "unit-tests" FAIL "$(grep -E 'tests passed|Failed' "$ROOT/.harness-ctest.log" | head -3 | tr '\n' ' ')"
  fi
else
  gate "unit-tests" SKIP "tests were not built"
fi

###############################################################################
hdr "UI smoke"

if [ "$RUN_UI" = "0" ]; then
  gate "ui-smoke" SKIP "--no-ui"
elif [ ! -d "$APP" ]; then
  gate "ui-smoke" SKIP "no app bundle"
else
  # Ad-hoc sign so the entitlements (microphone) are attached, same as release.
  codesign --force --deep --sign - --entitlements "$ROOT/macOS/entitlements.plist" "$APP" >/dev/null 2>&1
  UI_OUT=$(python3 "$ROOT/tools/ui_capture.py" "$APP" "$ARTIFACTS" 2>&1)
  RC=$?
  if [ $RC -eq 0 ]; then
    gate "ui-smoke" PASS "$(echo "$UI_OUT" | head -1)"
    echo "$UI_OUT" | sed 's/^/        /'
  elif [ $RC -eq 77 ]; then
    gate "ui-smoke" SKIP "pyobjc not installed"
  else
    gate "ui-smoke" FAIL "$(echo "$UI_OUT" | tail -2 | tr '\n' ' ')"
  fi
fi

###############################################################################
hdr "Summary"
printf '  %d passed, %d failed, %d skipped\n' "$PASS" "$FAIL" "$SKIP"
[ -d "$ARTIFACTS" ] && printf '  screenshots: %s\n' "$ARTIFACTS"
if [ "$FAIL" -gt 0 ]; then
  printf '\n\033[31mHARNESS FAILED\033[0m\n'; exit 1
elif [ "$SKIP" -gt 0 ]; then
  printf '\n\033[33mHARNESS PASSED WITH SKIPS\033[0m\n'; exit 0
else
  printf '\n\033[32mHARNESS PASSED\033[0m\n'; exit 0
fi
