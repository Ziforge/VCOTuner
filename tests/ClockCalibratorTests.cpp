// tests/ClockCalibratorTests.cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

#include "dsp/ClockCalibrator.h"

using namespace vcotuner;
using Catch::Approx;

namespace
{
    /** Feeds blocks whose host timestamps advance at `trueRate` while the
        device claims `nominal`, optionally with timing jitter on each stamp.
    */
    ClockCalibrator runBlocks (double nominal, double trueRate, int numBlocks,
                               int blockSize = 256, double jitterNs = 0.0,
                               unsigned seed = 1234)
    {
        ClockCalibrator cal;
        cal.reset (nominal);

        std::mt19937 rng (seed);
        std::normal_distribution<double> jitter (0.0, jitterNs);

        double seconds = 0.0;
        for (int i = 0; i < numBlocks; ++i)
        {
            const double noise = (jitterNs > 0.0) ? jitter (rng) : 0.0;
            const uint64_t ns = (uint64_t) std::llround (seconds * 1.0e9 + noise);
            cal.addBlock (blockSize, &ns);
            seconds += blockSize / trueRate;
        }
        return cal;
    }
}

TEST_CASE ("with no host timestamps there is no estimate at all")
{
    // Reading a clock on the audio thread would measure scheduling noise and
    // call it a rate, so the absence of a time source has to stay visible
    // rather than be papered over.
    ClockCalibrator cal;
    cal.reset (48000.0);
    for (int i = 0; i < 1000; ++i)
        cal.addBlock (256, nullptr);

    REQUIRE_FALSE (cal.hostTimestampsAvailable());
    REQUIRE_FALSE (cal.estimate().valid);
    REQUIRE (cal.correctedSampleRate() == Approx (48000.0));
}

TEST_CASE ("a converter running fast is measured to within a fraction of a ppm")
{
    // 48000 nominal, actually running 100 ppm fast.
    const double nominal = 48000.0;
    const double trueRate = nominal * (1.0 + 100.0e-6);

    auto cal = runBlocks (nominal, trueRate, 2000);
    const auto e = cal.estimate();

    REQUIRE (e.valid);
    REQUIRE (e.sampleRateHz == Approx (trueRate).epsilon (1e-9));
    REQUIRE (e.ppmOffset == Approx (100.0).margin (0.1));
    REQUIRE (cal.correctedSampleRate() == Approx (trueRate).epsilon (1e-9));
}

TEST_CASE ("a hundred ppm is worth correcting: it is a sixth of a cent")
{
    // Pins the motivation. 100 ppm on the rate is 100 ppm on every frequency
    // derived from it, which is 0.173 cents -- larger than the uncertainty the
    // period fit now reports on a clean note, so it is the dominant error.
    const double ppm = 100.0e-6;
    const double cents = 1200.0 * std::log2 (1.0 + ppm);
    REQUIRE (cents == Approx (0.1732).margin (0.001));
}

TEST_CASE ("timestamp jitter averages down instead of biasing the rate")
{
    // Real host timestamps are not exact. Jitter is zero-mean, so the slope
    // through many points converges; this asserts it does, rather than the
    // noise leaking into the answer.
    const double nominal = 48000.0;
    const double trueRate = nominal * (1.0 - 50.0e-6);

    auto cal = runBlocks (nominal, trueRate, 4000, 256, 20000.0 /* 20 us */);
    const auto e = cal.estimate();

    REQUIRE (e.valid);
    REQUIRE (e.ppmOffset == Approx (-50.0).margin (1.0));
    REQUIRE (e.stdErrorHz > 0.0);
    REQUIRE (std::isfinite (e.stdErrorHz));
}

TEST_CASE ("no estimate is offered before the baseline is long enough")
{
    // 200 blocks of 256 samples is about a second: past minBlocks but short of
    // minSpanSeconds, and a short baseline is exactly what limits precision.
    auto cal = runBlocks (48000.0, 48000.0, 210);
    const auto e = cal.estimate();

    REQUIRE_FALSE (e.valid);
    REQUIRE (cal.correctedSampleRate() == Approx (48000.0));
}

TEST_CASE ("an implausible correction is refused rather than applied")
{
    // A timestamp source that is wrong, or a device that changed rate, can
    // imply a huge correction. Applying it would be far worse than the error
    // it claims to fix, so the nominal rate stands.
    auto cal = runBlocks (48000.0, 48000.0 * 1.05, 2000);   // 50000 ppm
    const auto e = cal.estimate();

    REQUIRE_FALSE (e.valid);
    REQUIRE (cal.correctedSampleRate() == Approx (48000.0));
}

TEST_CASE ("a dropout restarts the run instead of being read as a rate error")
{
    // Samples going missing puts a step between two otherwise good segments.
    // Fitting across it would read the gap as a sustained rate error; the
    // calibrator starts again from the discontinuity.
    const double nominal = 48000.0;
    const double trueRate = nominal * (1.0 + 30.0e-6);
    const int blockSize = 256;

    ClockCalibrator cal;
    cal.reset (nominal);

    double seconds = 0.0;
    for (int i = 0; i < 3000; ++i)
    {
        if (i == 500)
            seconds += 0.75;     // a three-quarter-second hole in the stream

        const uint64_t ns = (uint64_t) std::llround (seconds * 1.0e9);
        cal.addBlock (blockSize, &ns);
        seconds += blockSize / trueRate;
    }

    const auto e = cal.estimate();
    REQUIRE (e.valid);

    // Without the restart the hole would dominate: 0.75 s spread over the run
    // is thousands of ppm, which would have been refused as implausible and
    // cost the estimate entirely.
    REQUIRE (e.ppmOffset == Approx (30.0).margin (0.5));
}

TEST_CASE ("the estimate reports the baseline it was taken over")
{
    auto cal = runBlocks (48000.0, 48000.0, 2000);
    const auto e = cal.estimate();

    REQUIRE (e.valid);
    REQUIRE (e.numBlocks == 2000);
    REQUIRE (e.spanSeconds == Approx (1999.0 * 256.0 / 48000.0).epsilon (1e-6));
}
