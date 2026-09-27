// tests/RealtimeAllocationCheck.cpp
//
// Counts heap allocations while the audio-thread code runs.
//
// A separate executable rather than a Catch2 case: it replaces global
// operator new, which is not something to impose on the rest of the suite.
// Exits non-zero if anything on the audio path allocates -- a malloc there can
// block on a lock held by another thread and produce a dropout, and the
// dropout shows up as a failed note rather than as the memory bug it is.
#include "dsp/PeriodDetector.h"
#include "dsp/ClockCalibrator.h"
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <vector>

static std::atomic<long> allocs{0};
static std::atomic<bool> counting{false};

void* operator new(std::size_t n) {
    if (counting.load()) allocs.fetch_add(1);
    void* p = std::malloc(n ? n : 1);
    if (!p) throw std::bad_alloc();
    return p;
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

using namespace vcotuner;
static const double kPi = 3.14159265358979323846;

int main() {
    const double sr = 48000.0, freq = 440.0;
    const int block = 256, blocks = 4000;

    std::vector<float> buf((size_t) block);

    PeriodDetector det;
    det.prepare(PeriodDetectorConfig().maxPeriods);       // the non-realtime step
    ClockCalibrator cal;

    long phase = 0;
    for (int run = 0; run < 12; ++run) {
        PeriodDetectorConfig cfg;
        cfg.requiredPeriods = 200;
        cfg.warmupSamples = 480;

        // everything from here on is what the audio thread does
        counting.store(true);
        det.reset(cfg);                                   // reset() runs on the audio thread
        cal.reset(sr);
        for (int b = 0; b < blocks; ++b) {
            for (int i = 0; i < block; ++i, ++phase)
                buf[(size_t) i] = (float) (0.9 * std::sin(2.0 * kPi * freq * phase / sr));
            det.processBlock(buf.data(), block);
            const uint64_t ns = (uint64_t) ((phase / sr) * 1e9);
            cal.addBlock(block, &ns);
            if ((b % 64) == 0) (void) cal.estimate();     // published from the audio thread
        }
        counting.store(false);
    }

    printf("detector status : %d\n", (int) det.status());
    printf("clock estimate  : valid=%d ppm=%+.3f\n", (int) cal.estimate().valid, cal.estimate().ppmOffset);
    printf("allocations on the audio path: %ld\n", allocs.load());
    return allocs.load() == 0 ? 0 : 1;
}
