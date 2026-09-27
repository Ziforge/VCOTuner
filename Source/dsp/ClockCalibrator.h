// Source/dsp/ClockCalibrator.h
#pragma once

#include <cstdint>

namespace vcotuner
{

struct ClockEstimate
{
    bool   valid         = false;
    double sampleRateHz  = 0.0;   // the converter's measured rate
    double stdErrorHz    = 0.0;   // uncertainty of that rate
    double ppmOffset     = 0.0;   // measured vs nominal, in parts per million
    double spanSeconds   = 0.0;   // baseline the estimate was taken over
    int    numBlocks     = 0;
};

struct ClockCalibratorConfig
{
    /** Blocks needed before an estimate is offered. */
    int    minBlocks        = 200;

    /** Baseline needed before an estimate is offered. Timing jitter averages
        down over the span, so a short baseline is the thing that limits
        precision, not the block count.
    */
    double minSpanSeconds   = 2.0;

    /** Largest correction that will ever be believed. A converter is out by
        tens to a few hundred ppm; anything past this is a broken timestamp
        source or a device that changed rate underneath us, and applying it
        would do more damage than the error it claims to fix.
    */
    double maxPlausiblePpm  = 2000.0;

    /** A block whose duration disagrees with its sample count by more than
        this fraction did not arrive contiguously -- a dropout, or the stream
        being restarted. Accumulating across it would fit a line to two
        unrelated segments, so the run is restarted instead.
    */
    double discontinuityTolerance = 0.5;
};

/** Measures the audio device's true sample rate against the host clock.

    The app converts a period in samples to a frequency in Hz by dividing by
    the sample rate, and uses the rate the device reports -- a nominal 48000,
    not what the converter's crystal actually runs at. Real interfaces are out
    by tens to hundreds of ppm, and 100 ppm is 0.17 cents. That error is
    systematic, so unlike jitter it does not average away with a longer
    measurement.

    It cancels in the pitch offsets, which are ratios against a reference pitch
    measured through the same clock, so live tuning was never affected by it.
    It does not cancel in any absolute reading: the frequency readout, the
    error-in-Hz display, and the frequencies recorded in a report.

    Each audio block gives a (sample count, host time) pair. Fitting a line
    through them gives seconds per sample, and its reciprocal is the true rate.
    This is the same estimator the period fit uses, for the same reason: the
    slope of many points is far better conditioned than any single difference.

    The sums are accumulated online, so the audio thread stores nothing per
    block and never allocates, and the baseline can grow without bound.
*/
class ClockCalibrator
{
public:
    void reset (double nominalSampleRate) noexcept;
    void setConfig (const ClockCalibratorConfig& c) noexcept { cfg = c; }

    /** Call once per audio block, from the audio thread.

        hostTimeNs is the device timestamp JUCE passes in
        AudioIODeviceCallbackContext. It is null when the host does not supply
        one, in which case no estimate is ever produced -- deliberately, since
        the alternative is reading a clock on the audio thread and calling the
        scheduling noise on it a measurement.
    */
    void addBlock (int numSamples, const uint64_t* hostTimeNs) noexcept;

    ClockEstimate estimate() const noexcept;

    bool hostTimestampsAvailable() const noexcept { return sawHostTime; }

    /** nominal * (1 + ppm/1e6) when an estimate is available and plausible,
        and the nominal rate otherwise. This is the divisor to use when
        converting a period in samples to a frequency.
    */
    double correctedSampleRate() const noexcept;

private:
    void restart (double x, double y) noexcept;

    ClockCalibratorConfig cfg {};

    double nominal = 48000.0;

    bool   haveOrigin = false;
    bool   sawHostTime = false;

    // Sums are kept relative to the first accepted point so they stay small
    // and well conditioned however long the run lasts.
    double originX = 0.0, originY = 0.0;
    double lastX = 0.0, lastY = 0.0;

    long long sampleIndex = 0;
    int    n   = 0;
    double sx  = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0, syy = 0.0;
};

} // namespace vcotuner
