// Source/dsp/MeasurementStatistics.cpp
#include "MeasurementStatistics.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace vcotuner
{

namespace
{
    /** Median of a copy. Taken by value because nth_element reorders. */
    double medianOf (std::vector<double> v)
    {
        if (v.empty())
            return 0.0;

        const size_t mid = v.size() / 2;
        std::nth_element (v.begin(), v.begin() + (long) mid, v.end());
        const double upper = v[mid];

        if (v.size() % 2 != 0)
            return upper;

        // Even count: the median is the mean of the two middle values. The
        // nth_element above already placed everything below `mid` before it,
        // so the second one is just the largest of that lower part.
        const double lower = *std::max_element (v.begin(), v.begin() + (long) mid);
        return 0.5 * (lower + upper);
    }
}

PeriodFit fitPeriod (const double* periods, int numPeriods, double outlierTolerance)
{
    PeriodFit fit;

    if (periods == nullptr || numPeriods < 2)
        return fit;

    // Cumulate periods back into crossing times: n+1 points for n periods.
    const int numCrossings = numPeriods + 1;
    std::vector<double> times ((size_t) numCrossings);
    times[0] = 0.0;
    for (int i = 0; i < numPeriods; ++i)
        times[(size_t) (i + 1)] = times[(size_t) i] + periods[i];

    const double medianPeriod = medianOf (std::vector<double> (periods, periods + numPeriods));

    if (! (medianPeriod > 0.0))
        return fit;

    // Number the crossings by counting cycles rather than by position. See the
    // header: an extra or missing crossing renumbers everything after it, and
    // that is what wrecks a plain index fit.
    std::vector<double> cycleNumber, crossingTime;
    cycleNumber.reserve ((size_t) numCrossings);
    crossingTime.reserve ((size_t) numCrossings);

    // The anchor is only ever moved to a crossing that landed on the grid, so
    // a rejected crossing's untrustworthy time never becomes the reference for
    // the ones that follow it.
    double anchorTime  = times[0];
    double anchorCycle = 0.0;
    cycleNumber.push_back (0.0);
    crossingTime.push_back (times[0]);

    int rejected = 0;

    for (int i = 1; i < numCrossings; ++i)
    {
        const double gap = times[(size_t) i] - anchorTime;
        const double cycles = std::floor (gap / medianPeriod + 0.5);

        // Closer to the anchor than half a period: this crossing cannot be a
        // new cycle, so it is the spurious one a click inserted.
        if (cycles < 1.0)
        {
            ++rejected;
            continue;
        }

        if (std::abs (gap - cycles * medianPeriod) > outlierTolerance * medianPeriod)
        {
            ++rejected;
            continue;
        }

        anchorCycle += cycles;
        anchorTime   = times[(size_t) i];
        cycleNumber.push_back (anchorCycle);
        crossingTime.push_back (anchorTime);
    }

    const int n = (int) cycleNumber.size();

    fit.rejectedCrossings = rejected;
    fit.usedCrossings     = n;

    // Three points are the minimum that leaves a degree of freedom for the
    // standard error.
    if (n < 3)
        return fit;

    double meanX = 0.0, meanY = 0.0;
    for (int i = 0; i < n; ++i) { meanX += cycleNumber[(size_t) i]; meanY += crossingTime[(size_t) i]; }
    meanX /= n;
    meanY /= n;

    double sxx = 0.0, sxy = 0.0;
    for (int i = 0; i < n; ++i)
    {
        const double dx = cycleNumber[(size_t) i] - meanX;
        sxx += dx * dx;
        sxy += dx * (crossingTime[(size_t) i] - meanY);
    }

    // Guards against every kept crossing sharing one cycle number, which no
    // real capture produces but which would divide by zero here.
    if (sxx <= 0.0)
        return fit;

    const double slope = sxy / sxx;
    const double intercept = meanY - slope * meanX;

    if (! (slope > 0.0))
        return fit;

    fit.periodSamples = slope;

    double sse = 0.0;
    for (int i = 0; i < n; ++i)
    {
        const double residual = crossingTime[(size_t) i] - (intercept + slope * cycleNumber[(size_t) i]);
        sse += residual * residual;
    }

    fit.periodStdError = std::sqrt (sse / ((n - 2) * sxx));
    fit.valid = true;

    return fit;
}

MeasurementResult computeMeasurement (const double* periods, int numPeriods,
                                      double sampleRate,
                                      double referenceFrequency,
                                      int referencePitch)
{
    MeasurementResult result;

    const auto fit = fitPeriod (periods, numPeriods);
    result.rejectedCrossings = fit.rejectedCrossings;

    if (! fit.valid || fit.periodSamples <= 0.0
        || referenceFrequency <= 0.0 || sampleRate <= 0.0)
        return result;

    result.frequency = sampleRate / fit.periodSamples;

    // Relative uncertainty carries straight across from period to frequency.
    const double relative = fit.periodStdError / fit.periodSamples;
    result.frequencyDeviation = result.frequency * relative;

    result.pitch = 12.0 * std::log2 (result.frequency / referenceFrequency)
                 + referencePitch;

    // d(pitch)/d(f) = 12 / (f * ln2), so the semitone uncertainty is just the
    // relative uncertainty scaled by 12 / ln2.
    result.pitchDeviation = 12.0 * relative / std::log (2.0);
    result.valid = true;

    return result;
}

} // namespace vcotuner
