// Source/dsp/MeasurementStatistics.h
#pragma once

namespace vcotuner
{

struct PeriodFit
{
    bool   valid          = false;
    double periodSamples  = 0.0;
    double periodStdError = 0.0;

    /** Crossings left out of the fit because they did not sit on the cycle
        grid -- a click's spurious crossing, or one the trigger missed. Zero
        on a clean capture. Reported so a repaired measurement can be shown as
        repaired rather than passed off as pristine.
    */
    int    rejectedCrossings = 0;

    /** Crossings actually used. rejectedCrossings + used == the number of
        crossings handed in, except where re-indexing collapsed a pair.
    */
    int    usedCrossings = 0;
};

struct MeasurementResult
{
    bool   valid              = false;
    double frequency          = 0.0;
    double frequencyDeviation = 0.0;
    double pitch              = 0.0;
    double pitchDeviation     = 0.0;
    int    rejectedCrossings  = 0;
};

/** Robust least-squares fit of crossing time against cycle number.

    The periods are cumulated back into crossing times and fitted with a
    straight line whose slope is the period estimate; the standard error of
    that slope is its uncertainty.

    This replaces taking the standard deviation of the individual periods,
    which answers a different question: consecutive periods share a crossing
    time, so their errors are negatively correlated and the spread of the
    periods badly overstates the uncertainty of their mean.

    Crossings are numbered by counting cycles rather than by assuming one
    cycle per crossing. That distinction is what makes the fit robust. A click
    that forces an extra crossing does not merely contribute one outlying
    period: it shifts the cycle number of every crossing after it, which an
    index-based fit reads as a step in the line and answers with a badly wrong
    slope. Rounding each gap to the nearest whole number of median periods
    recovers the true cycle number across such a break, and any crossing that
    still does not land on the grid is dropped from the fit and counted in
    rejectedCrossings.

    The median period is the scale used for that rounding precisely because it
    survives a minority of bad crossings; the mean does not.

    Requires at least 2 periods (3 crossing times) to return a fit; fewer
    returns valid == false with finite zeroed fields. Exactly 2 periods
    leaves a single degree of freedom (n - 2 == 1), which is still a
    well-defined standard error but a deliberately wide one -- a large
    error bar is precisely how low confidence should be communicated,
    rather than discarding the measurement outright.

    outlierTolerance is the permitted distance from the cycle grid, as a
    fraction of the median period.
*/
PeriodFit fitPeriod (const double* periods, int numPeriods,
                     double outlierTolerance = 0.25);

/** Converts a period sequence into frequency, pitch and their uncertainties. */
MeasurementResult computeMeasurement (const double* periods, int numPeriods,
                                      double sampleRate,
                                      double referenceFrequency,
                                      int referencePitch);

} // namespace vcotuner
