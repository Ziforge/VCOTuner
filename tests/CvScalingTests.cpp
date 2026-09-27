// tests/CvScalingTests.cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cmath>

#include "dsp/CvScaling.h"

using namespace vcotuner;
using Catch::Approx;

TEST_CASE ("one volt per octave really is one volt per octave")
{
    // The defining property, stated as such: twelve semitones apart must be
    // exactly one volt apart, anywhere in the range.
    for (float pitch : { 24.0f, 48.0f, 60.0f, 72.0f, 108.0f })
    {
        const float low  = midiToVoltage (pitch,         CvStandard::oneVoltPerOctave, 1000.0f);
        const float high = midiToVoltage (pitch + 12.0f, CvStandard::oneVoltPerOctave, 1000.0f);
        REQUIRE (high - low == Approx (1.0f).margin (1e-6));
    }

    // and a semitone is a twelfth of that
    const float a = midiToVoltage (60.0f, CvStandard::oneVoltPerOctave, 1000.0f);
    const float b = midiToVoltage (61.0f, CvStandard::oneVoltPerOctave, 1000.0f);
    REQUIRE (b - a == Approx (1.0f / 12.0f).margin (1e-6));
}

TEST_CASE ("pitch survives a round trip through voltage")
{
    // If these two disagree, a note asked for and the note read back differ,
    // and the difference is indistinguishable from an oscillator that will
    // not track.
    for (float pitch = 24.0f; pitch <= 108.0f; pitch += 1.0f)
    {
        {
            const float v = midiToVoltage (pitch, CvStandard::oneVoltPerOctave, 1000.0f);
            REQUIRE (voltageToMidi (v, CvStandard::oneVoltPerOctave, 1000.0f)
                     == Approx (pitch).margin (1e-3));
        }
        {
            const float v = midiToVoltage (pitch, CvStandard::hzPerVolt, 1000.0f);
            REQUIRE (voltageToMidi (v, CvStandard::hzPerVolt, 1000.0f)
                     == Approx (pitch).margin (1e-3));
        }
    }
}

TEST_CASE ("frequency and pitch routes agree with each other")
{
    // A440 is MIDI 69 by definition, so asking by frequency and asking by
    // pitch must produce the same voltage.
    for (auto standard : { CvStandard::oneVoltPerOctave, CvStandard::hzPerVolt })
    {
        const float byPitch = midiToVoltage (69.0f, standard, 1000.0f);
        const float byFreq  = frequencyToVoltage (440.0f, standard, 1000.0f);
        REQUIRE (byPitch == Approx (byFreq).margin (1e-5));
    }
}

TEST_CASE ("Hz per volt doubles the voltage for an octave, unlike 1V/oct")
{
    // The two standards are different shapes, not different constants. This
    // pins that: Hz/V is proportional to frequency, so an octave is a factor
    // of two rather than a fixed step.
    const float v1 = midiToVoltage (69.0f, CvStandard::hzPerVolt, 1000.0f);
    const float v2 = midiToVoltage (81.0f, CvStandard::hzPerVolt, 1000.0f);
    REQUIRE (v2 == Approx (2.0f * v1).margin (1e-5));
    REQUIRE (v1 == Approx (0.44f).margin (1e-4));     // 440 Hz / 1000 Hz per V
}

TEST_CASE ("the sample mapping hits the rails exactly at the range endpoints")
{
    const CvRange range { -10.0f, +10.0f };

    REQUIRE (voltageToSample (-10.0f, range) == Approx (-1.0f));
    REQUIRE (voltageToSample (  0.0f, range) == Approx ( 0.0f).margin (1e-6));
    REQUIRE (voltageToSample (+10.0f, range) == Approx (+1.0f));

    // and an asymmetric interface maps its own zero, not the midpoint of the
    // symmetric case
    const CvRange unipolar { 0.0f, 10.0f };
    REQUIRE (voltageToSample (0.0f, unipolar) == Approx (-1.0f));
    REQUIRE (voltageToSample (5.0f, unipolar) == Approx ( 0.0f).margin (1e-6));
}

TEST_CASE ("voltage survives a round trip through the sample mapping")
{
    const CvRange range { -10.0f, +10.0f };
    for (float v = -10.0f; v <= 10.0f; v += 0.5f)
        REQUIRE (sampleToVoltage (voltageToSample (v, range), range) == Approx (v).margin (1e-4));
}

TEST_CASE ("a degenerate range is refused rather than dividing by zero")
{
    const CvRange broken { 5.0f, 5.0f };
    REQUIRE (voltageToSample (5.0f, broken) == Approx (0.0f));
    REQUIRE (std::isfinite (voltageToSample (5.0f, broken)));
}

TEST_CASE ("calibration cannot push the output past the interface range")
{
    // The bug this pins: clamping before the correction and not after let a
    // corrected voltage leave the range, which voltageToSample() then turns
    // into a magnitude past +/-1 for the interface to clip silently.
    const CvRange range { -10.0f, +10.0f };
    const CvCalibration cal { true, 1.05f, 0.4f };    // 5% gain, 0.4 V offset

    for (float v = -12.0f; v <= 12.0f; v += 0.25f)
    {
        const float out = conditionOutputVoltage (v, range, cal);
        REQUIRE (out >= range.minVolts);
        REQUIRE (out <= range.maxVolts);
        REQUIRE (std::abs (voltageToSample (out, range)) <= 1.0f + 1e-6f);
    }

    // the top of the range is where it used to escape
    REQUIRE (conditionOutputVoltage (10.0f, range, cal) == Approx (10.0f));
}

TEST_CASE ("an uncalibrated interface is passed through untouched")
{
    const CvCalibration none {};
    REQUIRE (applyCalibration (3.3f, none) == Approx (3.3f));

    const CvCalibration cal { true, 2.0f, 1.0f };
    REQUIRE (applyCalibration (3.0f, cal) == Approx (7.0f));
}

TEST_CASE ("Hz per volt below zero volts returns a finite pitch")
{
    // Hz/V cannot express zero or negative frequency. The log would return
    // -inf and carry it into every pitch computed from it.
    const float pitch = voltageToMidi (-1.0f, CvStandard::hzPerVolt, 1000.0f);
    REQUIRE (std::isfinite (pitch));
}
