// Source/dsp/PeriodDetector.h
#pragma once

#include <vector>

namespace vcotuner
{

struct PeriodDetectorConfig
{
    double hysteresisFraction = 0.1;    // of measured amplitude
    int    stabilityWindow    = 5;      // consecutive periods compared
    double stabilityTolerance = 0.1;    // 10% spread allowed
    int    maxPeriods         = 600;    // storage limit
    int    warmupSamples      = 2048;   // level-tracking window
    double silenceFloor       = 1e-4;   // amplitude below this => silent
    int    requiredPeriods    = 10;     // valid periods needed for 'stable'

    /** How fast the trigger level follows the signal, as a blend coefficient
        applied once per cycle. 0 disables tracking and latches the level for
        the whole measurement.

        The level is latched at the end of warm-up, and an oscillator whose DC
        offset drifts during a long capture then crosses a threshold that no
        longer sits at the middle of its waveform. That does not merely add
        noise: the crossing happens at a progressively different phase each
        cycle, so the crossing times acquire a ramp and the fitted period comes
        out biased. Following the drift keeps the crossing at a fixed phase.

        Updated only between cycles, never within one, so the threshold a
        period is measured against is the same at both of its ends. The rate is
        slow for the reason the level was latched in the first place: a
        threshold that chases noise would put that noise straight into the
        crossing times.
    */
    double midpointTrackingRate = 0.05;

    /** Share of the collected periods allowed to sit off the cycle grid before
        the note is called unstable. A dropout inserts or removes one crossing
        and so disturbs at most two periods; a genuinely jittery oscillator
        disturbs a large fraction of them. Tolerating a bounded few is what
        separates "one click" from "this is not a steady pitch", and the fit
        in MeasurementStatistics then excludes them rather than absorbing them.
    */
    double maxOutlierFraction = 0.02;

    /** Outliers always tolerated regardless of fraction. One dropout costs two
        periods, so a lower allowance than this would fail short captures for
        the single glitch this is meant to survive.
    */
    int    minOutliersAllowed = 2;
};

enum class DetectorStatus
{
    collecting,         // still gathering
    stable,             // enough valid periods collected
    failedUnstable,     // never reached a steady rate
    failedNoCrossings,  // silent, or no crossings at all
    failedBufferFull    // ran out of storage before stabilising
};

class PeriodDetector
{
public:
    /** Allocates period storage up front.

        reset() is called from the real-time audio thread, where a heap
        allocation can cause dropouts. Call this once from a non-realtime
        thread with the largest maxPeriods any later config will use: the
        reserve inside reset() then asks for a capacity the buffer already
        has, which the standard requires to be a no-op.
    */
    void prepare (int maxPeriods);

    void reset (const PeriodDetectorConfig& config);
    void processBlock (const float* samples, int numSamples);

    DetectorStatus status() const noexcept { return currentStatus; }

    double midpoint()  const noexcept { return levelMidpoint; }
    double amplitude() const noexcept { return levelAmplitude; }

    int numPeriods() const noexcept { return (int) periods.size(); }
    const double* periodData() const noexcept { return periods.data(); }

    int numValidPeriods() const noexcept;
    const double* validPeriods() const noexcept;

    /** Periods judged off-grid by the last stability check. Zero until the
        check has run.
    */
    int numOutliers() const noexcept { return outlierCount; }

private:
    void finishWarmup();
    void trackLevel() noexcept;
    void processCrossing (double s);
    void recordCrossing (double position);
    void updateStability();
    int  countOutliers (const double* values, int count);

    PeriodDetectorConfig cfg {};
    DetectorStatus currentStatus = DetectorStatus::collecting;

    long long sampleCounter  = 0;
    int       warmupRemaining = 0;
    double    runningMin     = 0.0;
    double    runningMax     = 0.0;
    double    levelMidpoint  = 0.0;
    double    levelAmplitude = 0.0;
    bool      haveLevel      = false;

    // Extent of the cycle currently being collected, folded into the trigger
    // level at each crossing. Separate from runningMin/Max, which belong to
    // warm-up and stop being updated once it ends.
    double    cycleMin       =  1e30;
    double    cycleMax       = -1e30;
    bool      haveCycleExtent = false;

    std::vector<double> periods;

    // Scratch for the median in countOutliers(). Sized with periods so the
    // audio thread never allocates: nth_element needs to reorder a copy.
    std::vector<double> scratch;
    int outlierCount = 0;
    double lastCrossing   = -1.0;
    double lastSample     =  0.0;
    bool   armed          = false;
    bool   haveLastSample = false;
    int    firstValidIndex = -1;
};

} // namespace vcotuner
