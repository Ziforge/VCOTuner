/*
  ==============================================================================

    Visualizer.cpp
    Created: 22 May 2016 2:45:34pm
    Author:  Johannes Neumann
    Updated: Modern dark theme visualization

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "Visualizer.h"
#include "ModernLookAndFeel.h"

Visualizer::Visualizer(VCOTuner* t)
{
    tuner = t;
}

Visualizer::~Visualizer()
{
}

void Visualizer::paint(juce::Graphics &g, int width, int height)
{
    // Dark background
    g.fillAll(ModernLookAndFeel::Colors::background);

    if (measurements.size() == 0)
    {
        // Draw empty state message
        g.setColour(ModernLookAndFeel::Colors::textDim);
        g.setFont(Font(18.0f));
        g.drawText("No measurement data yet", 0, height / 2 - 30, width, 30, juce::Justification::centred);
        g.setFont(Font(14.0f));
        g.drawText("Press Start to begin measuring", 0, height / 2, width, 30, juce::Justification::centred);
        return;
    }

    // Calculate display range
    double max = 0;
    double min = 0;
    for (int i = 0; i < measurements.size(); i++)
    {
        double value = measurements[i].pitchOffset;
        double deviation = measurements[i].pitchDeviation;
        if (value - deviation < min)
            min = value - deviation;
        if (value + deviation > max)
            max = value + deviation;
    }
    double expandAmount = (max - min) * 0.2;
    min -= expandAmount;
    max += expandAmount;

    paintWithFixedScaling(g, width, height, min, max);
}

void Visualizer::paintWithFixedScaling(Graphics& g, int width, int height, double min, double max)
{
    // Dark background
    g.fillAll(ModernLookAndFeel::Colors::background);

    if (measurements.size() == 0)
    {
        g.setColour(ModernLookAndFeel::Colors::textDim);
        g.setFont(Font(18.0f));
        g.drawText("No Data", 0, 0, width, height, juce::Justification::centred);
        return;
    }

    // Full width chart layout (tuner is now in separate tab)
    const int bottomBarHeight = 35;
    const int topInfoHeight = 70;
    int imageHeight = height - bottomBarHeight - topInfoHeight;

    // Draw top info panel
    drawTopInfoPanel(g, width, topInfoHeight);

    // prepare coordinate transformation (flipping the y axis)
    heightForFlipping = (float)imageHeight + topInfoHeight;

    const float sidebarWidth = 60.0f;  // Y-axis labels width
    const float chartTop = (float)topInfoHeight;
    double columnWidth = (double)(width - sidebarWidth) / (double)measurements.size();
    const double allowedPitchOffset = 0.02; // 2 cents allowed (tight)

    if (min > -allowedPitchOffset)
        min = -allowedPitchOffset;
    if (max < allowedPitchOffset)
        max = allowedPitchOffset;
    if (max < min)
        return;

    double vertScaling = (double)imageHeight / (max - min);

    // Draw graph area background
    Rectangle<float> graphArea(sidebarWidth, chartTop, (float)(width - sidebarWidth), (float)imageHeight);
    g.setColour(ModernLookAndFeel::Colors::panel.withAlpha(0.3f));
    g.fillRect(graphArea);

    // Draw "in-tune" zone (green band around center)
    double tuneZoneTop = (allowedPitchOffset - min) * vertScaling;
    double tuneZoneBottom = (-allowedPitchOffset - min) * vertScaling;
    g.setColour(ModernLookAndFeel::Colors::meter.withAlpha(0.08f));
    g.fillRect(Rectangle<float>(sidebarWidth, yFlip((float)tuneZoneTop),
                                 (float)(width - sidebarWidth), (float)(tuneZoneTop - tuneZoneBottom)));

    // Draw maximum "in-tune" pitch offset and center line
    g.setColour(ModernLookAndFeel::Colors::meter.withAlpha(0.4f));
    const float dashLengths[] = {6, 4};
    double position = (allowedPitchOffset - min) * vertScaling;
    g.drawDashedLine(Line<float>(sidebarWidth, yFlip((float)position), (float)width, yFlip((float)position)), dashLengths, 2);
    position = (-allowedPitchOffset - min) * vertScaling;
    g.drawDashedLine(Line<float>(sidebarWidth, yFlip((float)position), (float)width, yFlip((float)position)), dashLengths, 2);

    // Center line (0 offset)
    position = (-min) * vertScaling;
    g.setColour(ModernLookAndFeel::Colors::textSecondary);
    g.drawLine(sidebarWidth, yFlip((float)position), (float)width, yFlip((float)position), 2.0f);

    // Draw Y-axis grid and labels
    g.setColour(ModernLookAndFeel::Colors::textDim);
    const int numIntervals = 13;
    const double allowedIntervals[numIntervals] = {0.005, 0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1, 2, 5, 10, 20, 50};
    double lineInterval = allowedIntervals[0];
    int currentIntervalIndex = 0;

    int numLinesAllowed = imageHeight / 40;
    while (((max - min) / lineInterval) > numLinesAllowed)
    {
        lineInterval = allowedIntervals[++currentIntervalIndex];
        if (currentIntervalIndex >= numIntervals)
            break;
    }
    bool useSemitoneTexts = lineInterval >= 1.0;

    int numPosLines = (int)trunc(max / lineInterval);
    int numNegLines = (int)trunc(-min / lineInterval);
    for (double y = numPosLines; y > -numNegLines; y--)
    {
        double linePos = (y * lineInterval - min) * vertScaling;
        double number = y * lineInterval * ((useSemitoneTexts) ? 1.0 : 100.0);
        String numberString = (std::abs(number - round(number)) > 0.1) ? String(number, 1) : String((int)round(number));
        String lineText = numberString;
        if (!useSemitoneTexts)
            lineText += "c";

        g.setColour(ModernLookAndFeel::Colors::textSecondary);
        g.setFont(Font(11.0f));
        g.drawText(lineText, juce::Rectangle<float>(0, yFlip(float(linePos) + 7), sidebarWidth - 6, 14), Justification::centredRight);

        // don't overwrite maximum "in-tune" lines
        if (y * lineInterval == allowedPitchOffset || y * lineInterval == -allowedPitchOffset)
            continue;

        g.setColour(ModernLookAndFeel::Colors::panelLight.withAlpha(0.3f));
        const float lineDashLengths[] = {2, 8};
        g.drawDashedLine(Line<float>(sidebarWidth, yFlip((float)linePos), (float)width, yFlip((float)linePos)), lineDashLengths, 2);
    }

    // Draw pitch measurement bars with modern styling
    for (int i = 0; i < measurements.size(); i++)
    {
        float left = sidebarWidth + i * (float)columnWidth;
        float barCenter = left + (float)columnWidth / 2.0f;
        float barWidth = jmax(2.0f, (float)columnWidth * 0.7f);

        // draw deviation range
        float maxPosition = (float)((measurements[i].pitchOffset + measurements[i].pitchDeviation - min) * vertScaling) ;
        float minPosition = (float)((measurements[i].pitchOffset - measurements[i].pitchDeviation - min) * vertScaling) ;

        // Determine color based on pitch offset
        float pitchCents = (float)(measurements[i].pitchOffset * 100.0);
        Colour barColor;
        if (std::abs(pitchCents) < 5.0f)
            barColor = ModernLookAndFeel::Colors::meter;
        else if (std::abs(pitchCents) < 15.0f)
            barColor = ModernLookAndFeel::Colors::meterWarn;
        else
            barColor = ModernLookAndFeel::Colors::meterBad;

        // Draw deviation band
        g.setColour(barColor.withAlpha(0.25f));
        g.fillRoundedRectangle(barCenter - barWidth / 2, yFlip(maxPosition), barWidth, maxPosition - minPosition, 2.0f);

        // Draw average value bar
        float pointPosition = (float)((measurements[i].pitchOffset - min) * vertScaling) ;
        float centerLineY = (float)((-min) * vertScaling) ;

        // Bar from center to value
        float barTop = jmin(pointPosition, centerLineY);
        float barHeight = std::abs(pointPosition - centerLineY);

        g.setColour(barColor);
        g.fillRoundedRectangle(barCenter - barWidth / 2, yFlip(barTop + barHeight), barWidth, barHeight, 2.0f);

        // Draw glow effect for the bar
        g.setColour(barColor.withAlpha(0.3f));
        g.fillRoundedRectangle(barCenter - barWidth / 2 - 2, yFlip(barTop + barHeight) - 2,
                               barWidth + 4, barHeight + 4, 3.0f);
    }

    // Draw the X-Axis label
    g.setColour(ModernLookAndFeel::Colors::textSecondary);
    g.setFont(Font(12.0f));
    g.drawText("MIDI Note", 0, height - bottomBarHeight, (int)sidebarWidth - 10, bottomBarHeight, Justification::centredRight);

    // Draw note labels on X axis
    const int numPitchTextIntervals = 5;
    const int pitchTextIntervals[numPitchTextIntervals] = {1, 2, 5, 10, 20};
    int currentPitchTextIntervalIndex = 0;
    while (g.getCurrentFont().getStringWidth("123.") > pitchTextIntervals[currentPitchTextIntervalIndex] * columnWidth)
    {
        currentPitchTextIntervalIndex++;
        if (currentPitchTextIntervalIndex >= numPitchTextIntervals)
            break;
    }
    int pitchTextInterval = pitchTextIntervals[currentPitchTextIntervalIndex];
    int startLine = 0;
    int endLine = measurements.size() - 1;
    while (measurements[startLine].midiPitch % pitchTextInterval != 0)
    {
        startLine++;
        if (startLine >= measurements.size())
            return;
    }
    while (measurements[endLine].midiPitch % pitchTextInterval != 0)
    {
        endLine--;
        if (endLine < 0 || endLine < startLine)
            return;
    }

    for (int i = startLine; i <= endLine; i += pitchTextInterval)
    {
        g.setColour(ModernLookAndFeel::Colors::textSecondary);
        g.setFont(Font(11.0f));
        float textWidth = g.getCurrentFont().getStringWidth(String(measurements[i].midiPitch));
        float xLeft = sidebarWidth + i * float(columnWidth);
        float x = xLeft + float(columnWidth) / 2.0f - textWidth / 2.0f;
        float yPos = height - bottomBarHeight + 8;
        g.drawText(String(measurements[i].midiPitch), juce::Rectangle<float>(x, yPos, textWidth, bottomBarHeight - 10), Justification::centred);

        // the line for the reference pitch will be drawn later
        if (measurements[i].midiPitch == tuner->getReferencePitch())
            continue;

        // also draw dim vertical lines for the larger divisions
        if (pitchTextInterval >= 2)
        {
            g.setColour(ModernLookAndFeel::Colors::accentAlt.withAlpha(0.05f));
            g.fillRect(Rectangle<float>(xLeft, chartTop, float(columnWidth), (float)imageHeight));
        }
    }

    // Draw a highlight for the reference pitch (if included in the measurements)
    if (measurements[0].midiPitch < tuner->getReferencePitch() &&
        measurements.getLast().midiPitch > tuner->getReferencePitch())
    {
        for (int i = 0; i < measurements.size(); i++)
        {
            if (measurements[i].midiPitch == tuner->getReferencePitch())
            {
                float xLeft = sidebarWidth + i * float(columnWidth);
                g.setColour(ModernLookAndFeel::Colors::accentAlt.withAlpha(0.15f));
                g.fillRect(Rectangle<float>(xLeft, chartTop, float(columnWidth), (float)imageHeight));

                // Draw reference label
                g.setColour(ModernLookAndFeel::Colors::accentAlt);
                g.setFont(Font(10.0f, Font::bold));
                g.drawText("REF", xLeft, height - bottomBarHeight + 22, float(columnWidth), 12, Justification::centred);
            }
        }
    }

}

void Visualizer::drawTopInfoPanel(Graphics& g, int width, int panelHeight)
{
    // Panel background
    Rectangle<float> panelBounds(10, 5, width - 20, panelHeight - 10);
    ModernLookAndFeel::drawPanel(g, panelBounds, 10.0f);

    if (measurements.size() == 0)
        return;

    // Find current/latest measurement
    const auto& current = measurements.getLast();

    // Calculate statistics
    float maxOffset = 0, minOffset = 0, avgOffset = 0;
    for (int i = 0; i < measurements.size(); i++)
    {
        float offset = (float)measurements[i].pitchOffset;
        if (offset > maxOffset) maxOffset = offset;
        if (offset < minOffset) minOffset = offset;
        avgOffset += offset;
    }
    avgOffset /= measurements.size();

    // Layout: divide into sections
    float sectionWidth = (panelBounds.getWidth() - 40) / 4.0f;
    float startX = panelBounds.getX() + 20;
    float topY = panelBounds.getY() + 8;

    // Section 1: Current Note & Frequency
    g.setColour(ModernLookAndFeel::Colors::textDim);
    g.setFont(Font(10.0f));
    g.drawText("CURRENT NOTE", startX, topY, sectionWidth, 12, Justification::left);

    g.setColour(ModernLookAndFeel::Colors::textPrimary);
    g.setFont(Font(24.0f, Font::bold));
    String noteName = MidiMessage::getMidiNoteName(current.midiPitch, true, true, 4);
    g.drawText(noteName, startX, topY + 14, sectionWidth, 28, Justification::left);

    g.setColour(ModernLookAndFeel::Colors::textSecondary);
    g.setFont(Font(12.0f));
    g.drawText(String(current.frequency, 2) + " Hz", startX, topY + 42, sectionWidth, 16, Justification::left);

    // Section 2: Current Error (cents)
    startX += sectionWidth;
    g.setColour(ModernLookAndFeel::Colors::textDim);
    g.setFont(Font(10.0f));
    g.drawText("PITCH ERROR", startX, topY, sectionWidth, 12, Justification::left);

    float currentCents = (float)(current.pitchOffset * 100.0);
    Colour errorColor;
    if (std::abs(currentCents) < 5.0f)
        errorColor = ModernLookAndFeel::Colors::meter;
    else if (std::abs(currentCents) < 15.0f)
        errorColor = ModernLookAndFeel::Colors::meterWarn;
    else
        errorColor = ModernLookAndFeel::Colors::meterBad;

    g.setColour(errorColor);
    g.setFont(Font(24.0f, Font::bold));
    String errorStr = (currentCents >= 0 ? "+" : "") + String(currentCents, 1) + "c";
    g.drawText(errorStr, startX, topY + 14, sectionWidth, 28, Justification::left);

    // Mini tuning meter
    Rectangle<float> meterBounds(startX, topY + 46, sectionWidth - 20, 12);
    ModernLookAndFeel::drawMeter(g, meterBounds, currentCents, -50.0f, 50.0f);

    // Section 3: Statistics
    startX += sectionWidth;
    g.setColour(ModernLookAndFeel::Colors::textDim);
    g.setFont(Font(10.0f));
    g.drawText("STATISTICS", startX, topY, sectionWidth, 12, Justification::left);

    g.setColour(ModernLookAndFeel::Colors::textSecondary);
    g.setFont(Font(11.0f));
    g.drawText("Max: " + String(maxOffset * 100, 1) + "c", startX, topY + 16, sectionWidth, 14, Justification::left);
    g.drawText("Min: " + String(minOffset * 100, 1) + "c", startX, topY + 30, sectionWidth, 14, Justification::left);
    g.drawText("Avg: " + String(avgOffset * 100, 1) + "c", startX, topY + 44, sectionWidth, 14, Justification::left);

    // Section 4: Progress
    startX += sectionWidth;
    g.setColour(ModernLookAndFeel::Colors::textDim);
    g.setFont(Font(10.0f));
    g.drawText("PROGRESS", startX, topY, sectionWidth, 12, Justification::left);

    g.setColour(ModernLookAndFeel::Colors::textPrimary);
    g.setFont(Font(18.0f, Font::bold));
    g.drawText(String(measurements.size()) + " pts", startX, topY + 14, sectionWidth, 24, Justification::left);

    g.setColour(ModernLookAndFeel::Colors::textSecondary);
    g.setFont(Font(11.0f));
    g.drawText("Range: " + String(measurements[0].midiPitch) + "-" + String(measurements.getLast().midiPitch),
               startX, topY + 42, sectionWidth, 14, Justification::left);
}

void Visualizer::paint(Graphics& g)
{
    paint(g, getWidth(), getHeight());
}

float Visualizer::yFlip(float y)
{
    return heightForFlipping - y;
}

void Visualizer::newMeasurementReady(const VCOTuner::measurement_t& m)
{
    bool found = false;
    for (int i = 0; i < measurements.size(); i++)
    {
        if (measurements[i].midiPitch == m.midiPitch)
        {
            measurements.set(i, m);
            found = true;
            repaint();
        }
    }

    if (!found)
        measurements.add(m);

    repaint();
}

void Visualizer::drawTunerDial(Graphics& g, Rectangle<float> bounds, float cents, int midiNote, float frequency)
{
    // Panel background
    ModernLookAndFeel::drawPanel(g, bounds, 12.0f);

    float centerX = bounds.getCentreX();
    float centerY = bounds.getCentreY() + 20;
    float radius = jmin(bounds.getWidth(), bounds.getHeight()) * 0.35f;

    // Draw the tuner arc
    drawTunerArc(g, centerX, centerY, radius, cents);

    // Draw note name - large and prominent
    String noteName = MidiMessage::getMidiNoteName(midiNote, true, true, 4);
    g.setColour(ModernLookAndFeel::Colors::textPrimary);
    g.setFont(Font(48.0f, Font::bold));
    g.drawText(noteName, bounds.getX(), bounds.getY() + 15, bounds.getWidth(), 50, Justification::centred);

    // Draw frequency
    g.setColour(ModernLookAndFeel::Colors::textSecondary);
    g.setFont(Font(16.0f));
    g.drawText(String(frequency, 2) + " Hz", bounds.getX(), bounds.getY() + 60, bounds.getWidth(), 20, Justification::centred);

    // Draw cents value below the dial
    Colour centsColor;
    if (std::abs(cents) < 5.0f)
        centsColor = ModernLookAndFeel::Colors::meter;
    else if (std::abs(cents) < 15.0f)
        centsColor = ModernLookAndFeel::Colors::meterWarn;
    else
        centsColor = ModernLookAndFeel::Colors::meterBad;

    g.setColour(centsColor);
    g.setFont(Font(28.0f, Font::bold));
    String centsStr = (cents >= 0 ? "+" : "") + String(cents, 1) + " cents";
    g.drawText(centsStr, bounds.getX(), bounds.getBottom() - 50, bounds.getWidth(), 30, Justification::centred);

    // Draw flat/sharp indicators
    g.setFont(Font(18.0f));
    g.setColour(cents < -5 ? ModernLookAndFeel::Colors::meterWarn : ModernLookAndFeel::Colors::textDim);
    g.drawText("FLAT", bounds.getX() + 20, centerY - 10, 60, 20, Justification::left);

    g.setColour(cents > 5 ? ModernLookAndFeel::Colors::meterWarn : ModernLookAndFeel::Colors::textDim);
    g.drawText("SHARP", bounds.getRight() - 80, centerY - 10, 60, 20, Justification::right);
}

void Visualizer::drawTunerArc(Graphics& g, float centerX, float centerY, float radius, float cents)
{
    const float maxCents = 50.0f;
    const float arcAngle = MathConstants<float>::pi * 0.8f; // Total arc span

    // Draw tick marks around the arc
    g.setColour(ModernLookAndFeel::Colors::panelLight);
    for (int i = -5; i <= 5; ++i)
    {
        float tickAngle = -MathConstants<float>::halfPi + (i / 5.0f) * arcAngle * 0.5f;
        float innerRadius = radius - 15;
        float outerRadius = (i == 0) ? radius + 5 : radius - 5;

        float x1 = centerX + std::cos(tickAngle) * innerRadius;
        float y1 = centerY + std::sin(tickAngle) * innerRadius;
        float x2 = centerX + std::cos(tickAngle) * outerRadius;
        float y2 = centerY + std::sin(tickAngle) * outerRadius;

        g.setColour(i == 0 ? ModernLookAndFeel::Colors::textSecondary : ModernLookAndFeel::Colors::panelLight);
        g.drawLine(x1, y1, x2, y2, i == 0 ? 3.0f : 1.5f);
    }

    // Draw colored arc segments
    for (int i = 0; i < 20; ++i)
    {
        float segmentStart = -MathConstants<float>::halfPi - arcAngle * 0.5f + (i / 20.0f) * arcAngle;
        float segmentEnd = segmentStart + arcAngle / 20.0f * 0.8f;

        // Color based on distance from center
        float distFromCenter = std::abs(i - 10) / 10.0f;
        Colour segColor;
        if (distFromCenter < 0.2f)
            segColor = ModernLookAndFeel::Colors::meter;
        else if (distFromCenter < 0.5f)
            segColor = ModernLookAndFeel::Colors::meterWarn;
        else
            segColor = ModernLookAndFeel::Colors::meterBad;

        Path segment;
        segment.addCentredArc(centerX, centerY, radius, radius, 0, segmentStart, segmentEnd, true);
        g.setColour(segColor.withAlpha(0.4f));
        g.strokePath(segment, PathStrokeType(8.0f, PathStrokeType::curved, PathStrokeType::rounded));
    }

    // Draw the needle
    float normalizedCents = jlimit(-1.0f, 1.0f, cents / maxCents);
    float needleAngle = -MathConstants<float>::halfPi + normalizedCents * arcAngle * 0.5f;

    float needleLength = radius + 10;
    float needleEndX = centerX + std::cos(needleAngle) * needleLength;
    float needleEndY = centerY + std::sin(needleAngle) * needleLength;

    // Needle shadow
    g.setColour(Colours::black.withAlpha(0.3f));
    g.drawLine(centerX + 2, centerY + 2, needleEndX + 2, needleEndY + 2, 4.0f);

    // Needle color based on accuracy
    Colour needleColor;
    if (std::abs(cents) < 5.0f)
        needleColor = ModernLookAndFeel::Colors::meter;
    else if (std::abs(cents) < 15.0f)
        needleColor = ModernLookAndFeel::Colors::meterWarn;
    else
        needleColor = ModernLookAndFeel::Colors::meterBad;

    // Draw needle with glow
    g.setColour(needleColor.withAlpha(0.3f));
    g.drawLine(centerX, centerY, needleEndX, needleEndY, 8.0f);

    g.setColour(needleColor);
    g.drawLine(centerX, centerY, needleEndX, needleEndY, 3.0f);

    // Center pivot
    g.setColour(ModernLookAndFeel::Colors::panel);
    g.fillEllipse(centerX - 12, centerY - 12, 24, 24);
    g.setColour(ModernLookAndFeel::Colors::textPrimary);
    g.fillEllipse(centerX - 8, centerY - 8, 16, 16);
}
