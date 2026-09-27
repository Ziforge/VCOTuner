#!/usr/bin/env python3
"""Launch the built app, capture each tab, and report what was seen.

The app is a normal windowed macOS build, so the only reliable handle on it is
its CGWindowID -- AppleScript's "frontmost" loses to whatever terminal is
running the harness. Capture by window id instead.
"""
import subprocess, sys, time, pathlib

try:
    import Quartz
except ImportError:
    print("SKIP: pyobjc (Quartz) not available", file=sys.stderr)
    sys.exit(77)

APP = sys.argv[1]
OUT = pathlib.Path(sys.argv[2])
OUT.mkdir(parents=True, exist_ok=True)


def windows():
    wl = Quartz.CGWindowListCopyWindowInfo(
        Quartz.kCGWindowListOptionOnScreenOnly | Quartz.kCGWindowListExcludeDesktopElements,
        Quartz.kCGNullWindowID)
    return [w for w in wl if 'VCOTuner' in str(w.get('kCGWindowOwnerName', ''))]


def main_window(timeout=90):
    """Wait for the app's window.

    Generous, and it reports how long it waited. The bundle lives on an
    external disk and the harness ad-hoc signs it immediately before this
    runs, so the first launch after a full rebuild spends a long time in
    Gatekeeper validating every file in it -- long enough that a 20 second
    limit failed the gate while the app was still coming up.
    """
    start = time.time()
    deadline = start + timeout
    while time.time() < deadline:
        ws = windows()
        if ws:
            waited = time.time() - start
            if waited > 5:
                print(f"  (window took {waited:.0f}s to appear)")
            return max(ws, key=lambda w: w['kCGWindowBounds']['Height'])
        time.sleep(0.5)
    return None


def click(win, lx, ly):
    b = win['kCGWindowBounds']
    pt = (b['X'] + lx, b['Y'] + ly)
    for kind in (Quartz.kCGEventLeftMouseDown, Quartz.kCGEventLeftMouseUp):
        Quartz.CGEventPost(Quartz.kCGHIDEventTap,
                           Quartz.CGEventCreateMouseEvent(None, kind, pt, Quartz.kCGMouseButtonLeft))
        time.sleep(0.12)
    time.sleep(0.7)


def shoot(win, name):
    path = OUT / f"{name}.png"
    subprocess.run(["screencapture", f"-l{win['kCGWindowNumber']}", "-o", str(path)], check=True)
    return path


subprocess.run(["osascript", "-e", 'tell application "VCOTuner" to quit'],
               capture_output=True)
time.sleep(2)
subprocess.run(["open", APP], check=True)

win = main_window()
if win is None:
    print("FAIL: app window never appeared")
    sys.exit(1)

b = win['kCGWindowBounds']
print(f"window {int(b['Width'])}x{int(b['Height'])} (id {win['kCGWindowNumber']})")

shots = [shoot(win, "01_tuner_tab")]
click(win, 150, 125)                     # the Chart tab sits right of Tuner
shots.append(shoot(win, "02_chart_tab"))
click(win, 60, 125)                      # back to Tuner
shots.append(shoot(win, "03_back_to_tuner"))

subprocess.run(["osascript", "-e", 'tell application "VCOTuner" to quit'],
               capture_output=True)

for s in shots:
    size = s.stat().st_size
    print(f"  captured {s.name} ({size} bytes)")
    if size < 10000:
        print(f"FAIL: {s.name} looks empty")
        sys.exit(1)
print("OK: app launched, both tabs reachable, screenshots captured")
