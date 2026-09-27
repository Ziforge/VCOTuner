// Source/dsp/CvScaling.cpp
#include "CvScaling.h"

#include <algorithm>
#include <cmath>

namespace vcotuner
{

namespace
{
    constexpr float kA440       = 440.0f;
    constexpr float kA440Midi   = 69.0f;
    constexpr float kZeroVoltMidi = 60.0f;   // C4 sits at 0 V in 1V/oct here

    float clampTo (float v, const CvRange& r)
    {
        return std::min (std::max (v, r.minVolts), r.maxVolts);
    }
}

float midiToVoltage (float midiPitch, CvStandard standard, float hzPerVoltScaling)
{
    switch (standard)
    {
        case CvStandard::oneVoltPerOctave:
            return (midiPitch - kZeroVoltMidi) / 12.0f;

        case CvStandard::hzPerVolt:
        {
            if (! (hzPerVoltScaling > 0.0f))
                return 0.0f;

            const float freq = kA440 * std::pow (2.0f, (midiPitch - kA440Midi) / 12.0f);
            return freq / hzPerVoltScaling;
        }
    }

    return 0.0f;
}

float voltageToMidi (float voltage, CvStandard standard, float hzPerVoltScaling)
{
    switch (standard)
    {
        case CvStandard::oneVoltPerOctave:
            return kZeroVoltMidi + voltage * 12.0f;

        case CvStandard::hzPerVolt:
        {
            const float freq = voltage * hzPerVoltScaling;

            // Hz/V cannot express zero or negative frequency, so there is no
            // pitch to return below 0 V. Answering with the 0 V pitch keeps
            // the result finite instead of handing back -inf from the log.
            if (! (freq > 0.0f))
                return kZeroVoltMidi;

            return kA440Midi + 12.0f * std::log2 (freq / kA440);
        }
    }

    return kZeroVoltMidi;
}

float frequencyToVoltage (float hz, CvStandard standard, float hzPerVoltScaling)
{
    switch (standard)
    {
        case CvStandard::oneVoltPerOctave:
        {
            if (! (hz > 0.0f))
                return 0.0f;

            const float midiPitch = kA440Midi + 12.0f * std::log2 (hz / kA440);
            return (midiPitch - kZeroVoltMidi) / 12.0f;
        }

        case CvStandard::hzPerVolt:
            return (hzPerVoltScaling > 0.0f) ? hz / hzPerVoltScaling : 0.0f;
    }

    return 0.0f;
}

float voltageToSample (float volts, const CvRange& range)
{
    const float span = range.maxVolts - range.minVolts;
    if (! (span > 0.0f))
        return 0.0f;

    return ((volts - range.minVolts) / span) * 2.0f - 1.0f;
}

float sampleToVoltage (float sample, const CvRange& range)
{
    const float span = range.maxVolts - range.minVolts;
    return range.minVolts + ((sample + 1.0f) * 0.5f) * span;
}

float applyCalibration (float voltage, const CvCalibration& cal)
{
    return cal.isCalibrated ? (cal.gain * voltage + cal.offset) : voltage;
}

float conditionOutputVoltage (float volts, const CvRange& range, const CvCalibration& cal)
{
    return clampTo (applyCalibration (clampTo (volts, range), cal), range);
}

} // namespace vcotuner
