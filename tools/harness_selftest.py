#!/usr/bin/env python3
"""Prove each static guard actually trips.

Breaks one thing at a time, asserts the named gate fails, then restores from an
in-memory copy. A guard that cannot fail is not a guard, so this runs before
trusting a green harness.
"""
import pathlib, subprocess, sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
HARNESS = [str(ROOT / 'tools/debug_harness.sh'), '--static-only']

PM = 'const String plusMinus (CharPointer_UTF8 ("\\xc2\\xb1"));'
TAB_OK = ('        g.setColour(Colors::panelLight.withAlpha(0.3f));\n'
          '        g.drawLine(0, (float)h - 1, (float)w, (float)h - 1);\n    }')
FONT_OK = 'GlyphArrangement::getStringWidth(g.getCurrentFont(), String(measurements[i].midiPitch))'
CV_OK = '            cvOutputManager->fillOutputBuffer(outputChannelData[0], numSamples);'
CLOCK_OK = 'fitFrequency(detector, effectiveSampleRate(),'

CASES = [
    ('no-raw-utf8-literals', 'Source/TunerDisplay.cpp',
     PM, 'const String plusMinus ("±");'),
    ('tabbar-overlay-not-opaque', 'Source/ModernLookAndFeel.h',
     TAB_OK, '        g.fillRect(0, 0, w, h);\n' + TAB_OK),
    ('no-juce7-font-api', 'Source/Visualizer.cpp',
     FONT_OK, 'g.getCurrentFont().getStringWidth(String(measurements[i].midiPitch))'),
    ('cv-before-early-returns', 'Source/VCOTuner.cpp',
     CV_OK, '            /* moved below the early returns */'),
    ('frequency-uses-corrected-clock', 'Source/VCOTuner.cpp',
     CLOCK_OK, 'fitFrequency(detector, sampleRate,'),
]


def harness_output():
    return subprocess.run(HARNESS, capture_output=True, text=True, cwd=ROOT).stdout


failed = False
print('harness selftest:')

for gate, relpath, anchor, broken in CASES:
    path = ROOT / relpath
    original = path.read_text()
    if anchor not in original:
        print(f'  BROKEN {gate}: anchor not found in {relpath}')
        failed = True
        continue
    path.write_text(original.replace(anchor, broken, 1))
    try:
        out = harness_output()
        if any('FAIL' in line and gate in line for line in out.splitlines()):
            print(f'  ok     {gate} trips when broken')
        else:
            print(f'  BROKEN {gate} did NOT trip')
            failed = True
    finally:
        path.write_text(original)

out = harness_output()
if any('FAIL' in line for line in out.splitlines()):
    print('  BROKEN baseline is not clean after restore')
    print('\n'.join('         ' + l for l in out.splitlines() if 'FAIL' in l))
    failed = True
else:
    print('  ok     baseline clean after restore')

print('SELFTEST FAILED' if failed else 'SELFTEST PASSED')
sys.exit(1 if failed else 0)
