// Source/dsp/ClockCalibrator.cpp
#include "ClockCalibrator.h"

#include <cmath>

namespace vcotuner
{

void ClockCalibrator::reset (double nominalSampleRate) noexcept
{
    nominal     = (nominalSampleRate > 0.0) ? nominalSampleRate : 48000.0;
    haveOrigin  = false;
    sawHostTime = false;
    originX = originY = lastX = lastY = 0.0;
    sampleIndex = 0;
    n = 0;
    sx = sy = sxx = sxy = syy = 0.0;
}

void ClockCalibrator::restart (double x, double y) noexcept
{
    originX = x;
    originY = y;
    lastX = 0.0;
    lastY = 0.0;
    n = 1;
    sx = sy = sxx = sxy = syy = 0.0;   // the origin itself contributes zeros
    haveOrigin = true;
}

void ClockCalibrator::addBlock (int numSamples, const uint64_t* hostTimeNs) noexcept
{
    if (numSamples <= 0)
        return;

    if (hostTimeNs == nullptr)
        return;     // no usable time source; see the header

    sawHostTime = true;

    const double x = (double) sampleIndex;
    const double y = (double) *hostTimeNs * 1.0e-9;

    sampleIndex += numSamples;

    if (! haveOrigin)
    {
        restart (x, y);
        return;
    }

    const double dx = x - originX;
    const double dy = y - originY;

    // Elapsed time and elapsed samples should agree to within the block
    // period. Where they do not, the stream was interrupted: samples went
    // missing, or the device restarted. Fitting across that joins two
    // unrelated segments and reads the gap as a rate error, so start again
    // from here rather than carry the damage.
    const double expectedStep = (dx - lastX) / nominal;
    const double actualStep   = dy - lastY;

    if (expectedStep > 0.0
        && std::abs (actualStep - expectedStep) > cfg.discontinuityTolerance * expectedStep)
    {
        restart (x, y);
        return;
    }

    lastX = dx;
    lastY = dy;

    ++n;
    sx  += dx;
    sy  += dy;
    sxx += dx * dx;
    sxy += dx * dy;
    syy += dy * dy;
}

ClockEstimate ClockCalibrator::estimate() const noexcept
{
    ClockEstimate e;
    e.numBlocks   = n;
    e.spanSeconds = lastY;

    if (n < cfg.minBlocks || lastY < cfg.minSpanSeconds)
        return e;

    const double denom = n * sxx - sx * sx;
    if (! (denom > 0.0))
        return e;

    // slope is seconds per sample, so the rate is its reciprocal.
    const double slope = (n * sxy - sx * sy) / denom;
    if (! (slope > 0.0))
        return e;

    const double rate = 1.0 / slope;
    const double ppm  = (rate / nominal - 1.0) * 1.0e6;

    if (! std::isfinite (ppm) || std::abs (ppm) > cfg.maxPlausiblePpm)
        return e;

    e.sampleRateHz = rate;
    e.ppmOffset    = ppm;

    // Standard error of the slope, then carried to the rate through
    // d(1/s)/ds = -1/s^2.
    if (n > 2)
    {
        const double intercept = (sy - slope * sx) / n;

        // Residual sum of squares from the accumulated sums, which avoids
        // keeping the points themselves:
        //   SSE = Syy - 2b*Sxy - 2a*Sy + b^2*Sxx + 2ab*Sx + n*a^2
        const double sse = syy
                         - 2.0 * slope * sxy
                         - 2.0 * intercept * sy
                         + slope * slope * sxx
                         + 2.0 * slope * intercept * sx
                         + n * intercept * intercept;

        if (sse >= 0.0)
        {
            const double slopeStdError = std::sqrt (sse / ((n - 2) * (sxx - sx * sx / n)));
            e.stdErrorHz = slopeStdError / (slope * slope);
        }
    }

    e.valid = true;
    return e;
}

double ClockCalibrator::correctedSampleRate() const noexcept
{
    const auto e = estimate();
    return e.valid ? e.sampleRateHz : nominal;
}

} // namespace vcotuner
