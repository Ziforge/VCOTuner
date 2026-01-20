/*
  ==============================================================================

    TunerDisplay.h
    Guitar pedal style tuner display

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "VCOTuner.h"
#include "ModernLookAndFeel.h"

class TunerDisplay : public Component,
                     public VCOTuner::Listener
{
public:
    TunerDisplay(VCOTuner* t);
    ~TunerDisplay() override;

    void paint(Graphics& g) override;
    void resized() override {}

    // For consistency with Visualizer interface
    void clearCache() { hasSignal = false; repaint(); }

    void newMeasurementReady(const VCOTuner::measurement_t& m) override;
    void tunerStarted() override { isActive = true; repaint(); }
    void tunerStopped() override { isActive = false; repaint(); }
    void tunerFinished() override {}
    void tunerStatusChanged(String) override {}

private:
    void drawMeasurementBox(Graphics& g, Rectangle<float> bounds, const String& label, const String& value, Colour valueColor);
    void drawSmallDataBox(Graphics& g, Rectangle<float> bounds, const String& label, const String& value);
    void drawPrecisionMeter(Graphics& g, Rectangle<float> bounds);

    VCOTuner* tuner;

    // Current state
    int currentMidiNote = 60;
    float currentFrequency = 440.0f;
    float currentCents = 0.0f;
    float currentDeviation = 0.0f;
    bool isActive = false;
    bool hasSignal = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TunerDisplay)
};
