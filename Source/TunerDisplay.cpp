/*
  ==============================================================================

    TunerDisplay.cpp
    Scientific precision tuner display for VCO calibration

  ==============================================================================
*/

#include "TunerDisplay.h"

TunerDisplay::TunerDisplay(VCOTuner* t) : tuner(t)
{
}

TunerDisplay::~TunerDisplay()
{
}

void TunerDisplay::paint(Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Dark background
    g.fillAll(ModernLookAndFeel::Colors::background);

    // Main panel
    auto panelBounds = bounds.reduced(15);
    ModernLookAndFeel::drawPanel(g, panelBounds, 12.0f);

    auto contentBounds = panelBounds.reduced(20);

    // Calculate target frequency for current MIDI note
    float targetFrequency = 440.0f * std::pow(2.0f, (currentMidiNote - 69) / 12.0f);
    float errorHz = currentFrequency - targetFrequency;

    // === HEADER: Status and MIDI Note ===
    auto headerArea = contentBounds.removeFromTop(40);

    // Status indicator (left)
    g.setColour(isActive ? ModernLookAndFeel::Colors::meter : ModernLookAndFeel::Colors::meterBad.withAlpha(0.5f));
    g.fillEllipse(headerArea.getX(), headerArea.getCentreY() - 6, 12, 12);
    g.setColour(ModernLookAndFeel::Colors::textSecondary);
    g.setFont(Font(12.0f));
    g.drawText(isActive ? "MEASURING" : "STANDBY", headerArea.getX() + 18, headerArea.getY(), 100, 40, Justification::centredLeft);

    // MIDI Note number (right)
    g.setColour(ModernLookAndFeel::Colors::textDim);
    g.setFont(Font(11.0f));
    g.drawText("MIDI", headerArea.getRight() - 80, headerArea.getY() + 2, 40, 16, Justification::right);
    g.setColour(hasSignal ? ModernLookAndFeel::Colors::textPrimary : ModernLookAndFeel::Colors::textDim);
    g.setFont(Font(20.0f, Font::bold));
    g.drawText(hasSignal ? String(currentMidiNote) : "--", headerArea.getRight() - 40, headerArea.getY(), 40, 40, Justification::centred);

    contentBounds.removeFromTop(10);

    // === PRIMARY: Note Name & Octave ===
    auto noteSection = contentBounds.removeFromTop(90);
    auto noteBg = noteSection.reduced(20, 5);
    g.setColour(ModernLookAndFeel::Colors::background);
    g.fillRoundedRectangle(noteBg, 8.0f);
    g.setColour(ModernLookAndFeel::Colors::panelLight);
    g.drawRoundedRectangle(noteBg, 8.0f, 1.0f);

    String noteName = hasSignal ? MidiMessage::getMidiNoteName(currentMidiNote, true, true, 4) : "--";
    g.setColour(hasSignal ? ModernLookAndFeel::Colors::accent : ModernLookAndFeel::Colors::textDim);
    g.setFont(Font(64.0f, Font::bold));
    g.drawText(noteName, noteBg, Justification::centred);

    contentBounds.removeFromTop(15);

    // === FREQUENCY MEASUREMENTS ===
    auto freqSection = contentBounds.removeFromTop(120);
    float colWidth = freqSection.getWidth() / 3.0f;

    // Column 1: Measured Frequency
    auto col1 = freqSection.removeFromLeft(colWidth);
    drawMeasurementBox(g, col1.reduced(5), "MEASURED",
                       hasSignal ? String(currentFrequency, 4) + " Hz" : "----.---- Hz",
                       ModernLookAndFeel::Colors::textPrimary);

    // Column 2: Target Frequency
    auto col2 = freqSection.removeFromLeft(colWidth);
    drawMeasurementBox(g, col2.reduced(5), "TARGET",
                       hasSignal ? String(targetFrequency, 4) + " Hz" : "----.---- Hz",
                       ModernLookAndFeel::Colors::textSecondary);

    // Column 3: Error in Hz
    auto col3 = freqSection;
    Colour errorHzColor = ModernLookAndFeel::Colors::textDim;
    if (hasSignal)
    {
        if (std::abs(errorHz) < 1.0f)
            errorHzColor = ModernLookAndFeel::Colors::meter;
        else if (std::abs(errorHz) < 5.0f)
            errorHzColor = ModernLookAndFeel::Colors::meterWarn;
        else
            errorHzColor = ModernLookAndFeel::Colors::meterBad;
    }
    String errorHzStr = hasSignal ? ((errorHz >= 0 ? "+" : "") + String(errorHz, 4) + " Hz") : "+----.---- Hz";
    drawMeasurementBox(g, col3.reduced(5), "ERROR (Hz)", errorHzStr, errorHzColor);

    contentBounds.removeFromTop(15);

    // === CENTS ERROR (Large Display) ===
    auto centsSection = contentBounds.removeFromTop(100);

    // Cents color coding
    Colour centsColor = ModernLookAndFeel::Colors::textDim;
    if (hasSignal)
    {
        if (std::abs(currentCents) < 2.0f)
            centsColor = ModernLookAndFeel::Colors::meter;
        else if (std::abs(currentCents) < 10.0f)
            centsColor = ModernLookAndFeel::Colors::meterWarn;
        else
            centsColor = ModernLookAndFeel::Colors::meterBad;
    }

    // Label
    g.setColour(ModernLookAndFeel::Colors::textDim);
    g.setFont(Font(11.0f));
    g.drawText("PITCH ERROR", centsSection.getX(), centsSection.getY(), centsSection.getWidth(), 16, Justification::centred);

    // Large cents value
    g.setColour(centsColor);
    g.setFont(Font(48.0f, Font::bold));
    String centsText = hasSignal ? ((currentCents >= 0 ? "+" : "") + String(currentCents, 2)) : "+---.--";
    g.drawText(centsText, centsSection.getX(), centsSection.getY() + 18, centsSection.getWidth(), 55, Justification::centred);

    // Unit label
    g.setColour(ModernLookAndFeel::Colors::textSecondary);
    g.setFont(Font(16.0f));
    g.drawText("cents", centsSection.getX(), centsSection.getY() + 70, centsSection.getWidth(), 20, Justification::centred);

    contentBounds.removeFromTop(10);

    // === PRECISION METER (Linear bar instead of needle) ===
    auto meterSection = contentBounds.removeFromTop(50);
    drawPrecisionMeter(g, meterSection.reduced(20, 5));

    contentBounds.removeFromTop(15);

    // === ADDITIONAL DATA ===
    auto dataSection = contentBounds.removeFromTop(60);
    float dataColWidth = dataSection.getWidth() / 4.0f;

    // Ratio (measured/target)
    auto dataCol1 = dataSection.removeFromLeft(dataColWidth);
    float ratio = hasSignal ? currentFrequency / targetFrequency : 1.0f;
    drawSmallDataBox(g, dataCol1.reduced(3), "RATIO",
                     hasSignal ? String(ratio, 6) : "-.------");

    // Semitone offset
    auto dataCol2 = dataSection.removeFromLeft(dataColWidth);
    float semitones = currentCents / 100.0f;
    drawSmallDataBox(g, dataCol2.reduced(3), "SEMITONES",
                     hasSignal ? ((semitones >= 0 ? "+" : "") + String(semitones, 4)) : "+-.----");

    // Deviation (if available from measurement)
    auto dataCol3 = dataSection.removeFromLeft(dataColWidth);
    drawSmallDataBox(g, dataCol3.reduced(3), "DEVIATION",
                     hasSignal ? ("±" + String(currentDeviation * 100.0f, 2) + "c") : "±--.--c");

    // Period (1/f)
    auto dataCol4 = dataSection;
    float periodMs = hasSignal ? 1000.0f / currentFrequency : 0.0f;
    drawSmallDataBox(g, dataCol4.reduced(3), "PERIOD",
                     hasSignal ? String(periodMs, 4) + " ms" : "-.---- ms");
}

void TunerDisplay::drawMeasurementBox(Graphics& g, Rectangle<float> bounds, const String& label, const String& value, Colour valueColor)
{
    // Background
    g.setColour(ModernLookAndFeel::Colors::background.withAlpha(0.5f));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(ModernLookAndFeel::Colors::panelLight.withAlpha(0.3f));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    // Label
    g.setColour(ModernLookAndFeel::Colors::textDim);
    g.setFont(Font(10.0f));
    g.drawText(label, bounds.getX(), bounds.getY() + 8, bounds.getWidth(), 14, Justification::centred);

    // Value
    g.setColour(valueColor);
    g.setFont(Font(18.0f, Font::bold));
    g.drawText(value, bounds.getX(), bounds.getY() + 28, bounds.getWidth(), 50, Justification::centred);
}

void TunerDisplay::drawSmallDataBox(Graphics& g, Rectangle<float> bounds, const String& label, const String& value)
{
    // Label
    g.setColour(ModernLookAndFeel::Colors::textDim);
    g.setFont(Font(9.0f));
    g.drawText(label, bounds.getX(), bounds.getY(), bounds.getWidth(), 14, Justification::centred);

    // Value
    g.setColour(ModernLookAndFeel::Colors::textSecondary);
    g.setFont(Font(14.0f, Font::bold));
    g.drawText(value, bounds.getX(), bounds.getY() + 16, bounds.getWidth(), 40, Justification::centred);
}

void TunerDisplay::drawPrecisionMeter(Graphics& g, Rectangle<float> bounds)
{
    const float maxCents = 50.0f;

    // Background track
    g.setColour(ModernLookAndFeel::Colors::panel);
    g.fillRoundedRectangle(bounds, 4.0f);

    // Draw scale markings
    float centerX = bounds.getCentreX();
    float meterWidth = bounds.getWidth();

    // Tick marks and labels
    g.setFont(Font(9.0f));
    for (int i = -5; i <= 5; ++i)
    {
        float x = centerX + (i / 5.0f) * (meterWidth / 2.0f - 10);
        float tickHeight = (i == 0) ? bounds.getHeight() : bounds.getHeight() * 0.4f;
        float tickY = bounds.getCentreY() - tickHeight / 2;

        Colour tickColor = (i == 0) ? ModernLookAndFeel::Colors::meter : ModernLookAndFeel::Colors::panelLight;
        g.setColour(tickColor);
        g.fillRect(x - 0.5f, tickY, 1.0f, tickHeight);

        // Labels at -50, -25, 0, +25, +50
        if (i % 2 == 0 || i == 0)
        {
            g.setColour(ModernLookAndFeel::Colors::textDim);
            String labelText = (i == 0) ? "0" : String(i * 10);
            g.drawText(labelText, x - 15, bounds.getBottom() + 2, 30, 12, Justification::centred);
        }
    }

    // Draw colored zones
    float zoneHeight = bounds.getHeight() * 0.3f;
    float zoneY = bounds.getCentreY() - zoneHeight / 2;

    // Green zone (center ±5 cents)
    float greenWidth = (5.0f / maxCents) * (meterWidth / 2.0f - 10) * 2;
    g.setColour(ModernLookAndFeel::Colors::meter.withAlpha(0.2f));
    g.fillRect(centerX - greenWidth / 2, zoneY, greenWidth, zoneHeight);

    // Yellow zones (±5 to ±15 cents)
    float yellowInner = (5.0f / maxCents) * (meterWidth / 2.0f - 10);
    float yellowOuter = (15.0f / maxCents) * (meterWidth / 2.0f - 10);
    g.setColour(ModernLookAndFeel::Colors::meterWarn.withAlpha(0.15f));
    g.fillRect(centerX + yellowInner, zoneY, yellowOuter - yellowInner, zoneHeight);
    g.fillRect(centerX - yellowOuter, zoneY, yellowOuter - yellowInner, zoneHeight);

    // Indicator position
    if (hasSignal)
    {
        float normalizedCents = jlimit(-1.0f, 1.0f, currentCents / maxCents);
        float indicatorX = centerX + normalizedCents * (meterWidth / 2.0f - 10);

        // Determine indicator color
        Colour indicatorColor;
        if (std::abs(currentCents) < 2.0f)
            indicatorColor = ModernLookAndFeel::Colors::meter;
        else if (std::abs(currentCents) < 10.0f)
            indicatorColor = ModernLookAndFeel::Colors::meterWarn;
        else
            indicatorColor = ModernLookAndFeel::Colors::meterBad;

        // Draw indicator line
        g.setColour(indicatorColor);
        g.fillRect(indicatorX - 2, bounds.getY(), 4.0f, bounds.getHeight());

        // Glow
        g.setColour(indicatorColor.withAlpha(0.3f));
        g.fillRect(indicatorX - 4, bounds.getY(), 8.0f, bounds.getHeight());
    }
}

void TunerDisplay::newMeasurementReady(const VCOTuner::measurement_t& m)
{
    currentMidiNote = m.midiPitch;
    currentFrequency = (float)m.frequency;
    currentCents = (float)(m.pitchOffset * 100.0); // Convert semitones to cents
    currentDeviation = (float)m.pitchDeviation;
    hasSignal = true;
    repaint();
}
