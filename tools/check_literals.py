#!/usr/bin/env python3
"""Flag raw non-ASCII bytes inside narrow string literals.

JUCE takes a const char* as UTF-8 in most paths but not all, and a literal like
"±" ends up rendered as "Â±". Anything non-ASCII that has to reach the UI
should be built with CharPointer_UTF8. Comments are left alone -- they never
reach a string.
"""
import pathlib, re, sys

root = pathlib.Path(sys.argv[1])
bad = []

for path in sorted(root.rglob('*')):
    if path.suffix not in ('.cpp', '.h', '.mm'):
        continue
    for n, raw in enumerate(path.read_bytes().split(b'\n'), 1):
        if all(b < 128 for b in raw):
            continue
        line = raw.decode('utf-8', 'replace')
        stripped = line.lstrip()
        if stripped.startswith('//') or stripped.startswith('*'):
            continue            # a comment, not a literal
        if 'CharPointer_UTF8' in line:
            continue            # already built explicitly
        if '"' not in line:
            continue            # non-ASCII outside any literal
        bad.append(f"{path.relative_to(root)}:{n}: {line.strip()[:100]}")

for b in bad:
    print(b)
sys.exit(1 if bad else 0)
