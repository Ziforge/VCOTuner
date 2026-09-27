// Source/dsp/CvScaling.h
#pragma once

namespace vcotuner
{

enum class CvStandard
{
    oneVoltPerOctave,
    hzPerVolt
};

/** How a voltage maps onto the audio sample the interface converts back to it.

    A DC-coupled output is just a DAC: full negative scale is minVolts and full
    positive scale is maxVolts, so the mapping is linear and the range is a
    property of the interface, not of the pitch.
*/
struct CvRange
{
    float minVolts = -10.0f;
    float maxVolts = +10.0f;
};

/** Linear correction measured for a particular interface. */
struct CvCalibration
{
    bool  isCalibrated = false;
    float gain   = 1.0f;
    float offset = 0.0f;
};

/** Pure conversions between pitch, voltage and sample value.

    Kept free of JUCE so it can be linked into the tests, which is the whole
    point: these are the numbers that decide what note a VCO is asked to play,
    and an error here is indistinguishable from an oscillator that will not
    track. The manager in Source/CVOutput owns the device and the state; this
    owns only the arithmetic.
*/
float midiToVoltage (float midiPitch, CvStandard standard, float hzPerVoltScaling);
float voltageToMidi (float voltage,   CvStandard standard, float hzPerVoltScaling);
float frequencyToVoltage (float hz,   CvStandard standard, float hzPerVoltScaling);

float voltageToSample (float volts,  const CvRange& range);
float sampleToVoltage (float sample, const CvRange& range);

float applyCalibration (float voltage, const CvCalibration& cal);

/** Clamp, correct, then clamp again.

    The second clamp is the point. Correction is gain * v + offset, so a
    voltage that was inside the range before it can land outside it after, and
    voltageToSample() would then return a magnitude past +/-1 for the interface
    to clip silently -- losing accuracy at exactly the extremes the calibration
    was measured to fix.
*/
float conditionOutputVoltage (float volts, const CvRange& range, const CvCalibration& cal);

} // namespace vcotuner
