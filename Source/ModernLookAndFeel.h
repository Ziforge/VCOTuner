/*
  ==============================================================================

    ModernLookAndFeel.h
    Modern dark theme for VCOTuner

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class ModernLookAndFeel : public LookAndFeel_V4
{
public:
    // Color palette
    struct Colors
    {
        static inline const Colour background      = Colour(0xff1a1a2e);
        static inline const Colour backgroundLight = Colour(0xff25253d);
        static inline const Colour panel           = Colour(0xff2d2d44);
        static inline const Colour panelLight      = Colour(0xff3d3d5c);
        static inline const Colour accent          = Colour(0xff00d4aa);
        static inline const Colour accentAlt       = Colour(0xff7b68ee);
        static inline const Colour warning         = Colour(0xffffb347);
        static inline const Colour error           = Colour(0xffff6b6b);
        static inline const Colour success         = Colour(0xff4ecdc4);
        static inline const Colour textPrimary     = Colour(0xfff0f0f0);
        static inline const Colour textSecondary   = Colour(0xffa0a0b0);
        static inline const Colour textDim         = Colour(0xff707080);
        static inline const Colour meter           = Colour(0xff00ff88);
        static inline const Colour meterWarn       = Colour(0xffffcc00);
        static inline const Colour meterBad        = Colour(0xffff4444);
    };

    ModernLookAndFeel()
    {
        // Set default colours
        setColour(ResizableWindow::backgroundColourId, Colors::background);
        setColour(DocumentWindow::backgroundColourId, Colors::background);

        setColour(TextButton::buttonColourId, Colors::panel);
        setColour(TextButton::buttonOnColourId, Colors::accent);
        setColour(TextButton::textColourOffId, Colors::textPrimary);
        setColour(TextButton::textColourOnId, Colors::background);

        setColour(ComboBox::backgroundColourId, Colors::panel);
        setColour(ComboBox::textColourId, Colors::textPrimary);
        setColour(ComboBox::outlineColourId, Colors::panelLight);
        setColour(ComboBox::arrowColourId, Colors::textSecondary);

        setColour(PopupMenu::backgroundColourId, Colors::panel);
        setColour(PopupMenu::textColourId, Colors::textPrimary);
        setColour(PopupMenu::highlightedBackgroundColourId, Colors::accent);
        setColour(PopupMenu::highlightedTextColourId, Colors::background);

        setColour(Label::textColourId, Colors::textPrimary);

        setColour(Slider::backgroundColourId, Colors::panel);
        setColour(Slider::trackColourId, Colors::accent);
        setColour(Slider::thumbColourId, Colors::textPrimary);

        setColour(ProgressBar::backgroundColourId, Colors::panel);
        setColour(ProgressBar::foregroundColourId, Colors::accent);

        setColour(TableHeaderComponent::backgroundColourId, Colors::panelLight);
        setColour(TableHeaderComponent::textColourId, Colors::textPrimary);
        setColour(TableListBox::backgroundColourId, Colors::panel);
        setColour(TableListBox::textColourId, Colors::textPrimary);
    }

    void drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
        auto cornerSize = 6.0f;

        Colour baseColour = backgroundColour;

        if (shouldDrawButtonAsDown)
            baseColour = baseColour.brighter(0.1f);
        else if (shouldDrawButtonAsHighlighted)
            baseColour = baseColour.brighter(0.05f);

        // Draw shadow
        g.setColour(Colours::black.withAlpha(0.3f));
        g.fillRoundedRectangle(bounds.translated(0, 2), cornerSize);

        // Draw button
        g.setColour(baseColour);
        g.fillRoundedRectangle(bounds, cornerSize);

        // Draw subtle gradient overlay
        ColourGradient gradient(Colours::white.withAlpha(0.05f), bounds.getX(), bounds.getY(),
                                Colours::black.withAlpha(0.05f), bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(bounds, cornerSize);

        // Draw border
        g.setColour(baseColour.brighter(0.2f));
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
    }

    void drawComboBox(Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH, ComboBox& box) override
    {
        auto bounds = Rectangle<int>(0, 0, width, height).toFloat().reduced(1.0f);
        auto cornerSize = 6.0f;

        g.setColour(box.findColour(ComboBox::backgroundColourId));
        g.fillRoundedRectangle(bounds, cornerSize);

        g.setColour(box.findColour(ComboBox::outlineColourId));
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);

        // Draw arrow
        Path arrow;
        auto arrowBounds = Rectangle<float>(buttonX + buttonW * 0.3f, buttonY + buttonH * 0.4f,
                                             buttonW * 0.4f, buttonH * 0.2f);
        arrow.addTriangle(arrowBounds.getX(), arrowBounds.getY(),
                          arrowBounds.getCentreX(), arrowBounds.getBottom(),
                          arrowBounds.getRight(), arrowBounds.getY());

        g.setColour(box.findColour(ComboBox::arrowColourId));
        g.fillPath(arrow);
    }

    void drawProgressBar(Graphics& g, ProgressBar& progressBar, int width, int height,
                         double progress, const String& textToShow) override
    {
        auto bounds = Rectangle<float>(0, 0, (float)width, (float)height).reduced(1.0f);
        auto cornerSize = 4.0f;

        // Background
        g.setColour(Colors::panel);
        g.fillRoundedRectangle(bounds, cornerSize);

        // Progress fill
        if (progress > 0)
        {
            auto fillBounds = bounds.withWidth(bounds.getWidth() * (float)progress);

            ColourGradient gradient(Colors::accent, fillBounds.getX(), fillBounds.getY(),
                                    Colors::accent.darker(0.2f), fillBounds.getX(), fillBounds.getBottom(), false);
            g.setGradientFill(gradient);
            g.fillRoundedRectangle(fillBounds, cornerSize);
        }

        // Border
        g.setColour(Colors::panelLight);
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);

        // Text
        if (textToShow.isNotEmpty())
        {
            g.setColour(Colors::textPrimary);
            g.setFont(Font(12.0f));
            g.drawText(textToShow, bounds, Justification::centred, false);
        }
    }

    void drawLinearSlider(Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const Slider::SliderStyle style, Slider& slider) override
    {
        auto trackWidth = 4.0f;
        auto bounds = Rectangle<int>(x, y, width, height).toFloat();

        Point<float> startPoint(bounds.getX(), bounds.getCentreY());
        Point<float> endPoint(bounds.getRight(), bounds.getCentreY());

        // Draw track background
        Path backgroundTrack;
        backgroundTrack.startNewSubPath(startPoint);
        backgroundTrack.lineTo(endPoint);
        g.setColour(Colors::panel);
        g.strokePath(backgroundTrack, PathStrokeType(trackWidth, PathStrokeType::curved, PathStrokeType::rounded));

        // Draw active track
        Path valueTrack;
        Point<float> thumbPoint(sliderPos, bounds.getCentreY());
        valueTrack.startNewSubPath(startPoint);
        valueTrack.lineTo(thumbPoint);
        g.setColour(Colors::accent);
        g.strokePath(valueTrack, PathStrokeType(trackWidth, PathStrokeType::curved, PathStrokeType::rounded));

        // Draw thumb
        auto thumbWidth = 16.0f;
        g.setColour(Colors::textPrimary);
        g.fillEllipse(Rectangle<float>(thumbWidth, thumbWidth).withCentre(thumbPoint));
    }

    void drawTableHeaderBackground(Graphics& g, TableHeaderComponent& header) override
    {
        auto bounds = header.getLocalBounds();
        g.setColour(Colors::panelLight);
        g.fillRect(bounds);
    }

    void drawTableHeaderColumn(Graphics& g, TableHeaderComponent& header, const String& columnName,
                               int columnId, int width, int height, bool isMouseOver,
                               bool isMouseDown, int columnFlags) override
    {
        auto bounds = Rectangle<int>(0, 0, width, height);

        if (isMouseOver)
            g.setColour(Colors::panel);
        else
            g.setColour(Colors::panelLight);
        g.fillRect(bounds);

        g.setColour(Colors::background);
        g.drawLine((float)width - 1, 2, (float)width - 1, (float)height - 4);

        g.setColour(Colors::textPrimary);
        g.setFont(Font(13.0f, Font::bold));
        g.drawText(columnName, bounds.reduced(4, 0), Justification::centredLeft, true);
    }

    Font getTextButtonFont(TextButton&, int buttonHeight) override
    {
        return Font(jmin(14.0f, buttonHeight * 0.6f));
    }

    Font getLabelFont(Label& label) override
    {
        return Font(14.0f);
    }

    // Tab button styling
    int getTabButtonBestWidth(TabBarButton& button, int tabDepth) override
    {
        return 100;
    }

    void drawTabButton(TabBarButton& button, Graphics& g, bool isMouseOver, bool isMouseDown) override
    {
        auto bounds = button.getLocalBounds().toFloat();
        bool isFrontTab = button.isFrontTab();

        // Background
        if (isFrontTab)
        {
            g.setColour(Colors::panel);
            g.fillRoundedRectangle(bounds.reduced(2, 0), 6.0f);
            g.setColour(Colors::accent);
            g.fillRect(bounds.removeFromBottom(3));
        }
        else if (isMouseOver)
        {
            g.setColour(Colors::panelLight.withAlpha(0.3f));
            g.fillRoundedRectangle(bounds.reduced(2, 2), 6.0f);
        }

        // Text
        g.setColour(isFrontTab ? Colors::textPrimary : Colors::textSecondary);
        g.setFont(Font(14.0f, isFrontTab ? Font::bold : Font::plain));
        g.drawText(button.getButtonText(), bounds.reduced(4), Justification::centred);
    }

    void drawTabAreaBehindFrontButton(TabbedButtonBar& bar, Graphics& g, int w, int h) override
    {
        g.setColour(Colors::background);
        g.fillRect(0, 0, w, h);
        g.setColour(Colors::panelLight.withAlpha(0.3f));
        g.drawLine(0, (float)h - 1, (float)w, (float)h - 1);
    }

    // Utility functions for drawing modern UI elements
    static void drawPanel(Graphics& g, Rectangle<float> bounds, float cornerSize = 8.0f)
    {
        // Shadow
        g.setColour(Colours::black.withAlpha(0.2f));
        g.fillRoundedRectangle(bounds.translated(0, 2), cornerSize);

        // Panel
        g.setColour(Colors::panel);
        g.fillRoundedRectangle(bounds, cornerSize);

        // Border
        g.setColour(Colors::panelLight);
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
    }

    static void drawMeter(Graphics& g, Rectangle<float> bounds, float value, float minVal = -50.0f, float maxVal = 50.0f)
    {
        auto cornerSize = 4.0f;

        // Background
        g.setColour(Colors::panel);
        g.fillRoundedRectangle(bounds, cornerSize);

        // Calculate normalized value
        float normalized = (value - minVal) / (maxVal - minVal);
        normalized = jlimit(0.0f, 1.0f, normalized);

        // Determine color based on value (center = good, edges = bad)
        float distFromCenter = std::abs(normalized - 0.5f) * 2.0f;
        Colour meterColor;
        if (distFromCenter < 0.2f)
            meterColor = Colors::meter;
        else if (distFromCenter < 0.5f)
            meterColor = Colors::meterWarn;
        else
            meterColor = Colors::meterBad;

        // Draw fill
        auto fillWidth = bounds.getWidth() * normalized;
        auto fillBounds = bounds.withWidth(fillWidth);
        g.setColour(meterColor);
        g.fillRoundedRectangle(fillBounds, cornerSize);

        // Center line indicator
        g.setColour(Colors::textDim);
        float centerX = bounds.getCentreX();
        g.drawLine(centerX, bounds.getY() + 2, centerX, bounds.getBottom() - 2, 1.0f);
    }

    static void drawTunerNeedle(Graphics& g, Rectangle<float> bounds, float cents, float maxCents = 50.0f)
    {
        auto centerX = bounds.getCentreX();
        auto centerY = bounds.getBottom() - 10.0f;
        auto radius = jmin(bounds.getWidth(), bounds.getHeight()) * 0.8f;

        // Draw arc background
        Path arcBg;
        arcBg.addCentredArc(centerX, centerY, radius, radius, 0,
                            -MathConstants<float>::pi * 0.75f,
                            MathConstants<float>::pi * 0.75f, true);
        g.setColour(Colors::panel);
        g.strokePath(arcBg, PathStrokeType(8.0f, PathStrokeType::curved, PathStrokeType::rounded));

        // Draw colored segments
        for (int i = 0; i < 15; ++i)
        {
            float startAngle = -MathConstants<float>::pi * 0.75f + (i / 15.0f) * MathConstants<float>::pi * 1.5f;
            float endAngle = startAngle + MathConstants<float>::pi * 0.1f;

            Path segment;
            segment.addCentredArc(centerX, centerY, radius - 2, radius - 2, 0, startAngle, endAngle, true);

            float distFromCenter = std::abs(i - 7.5f) / 7.5f;
            Colour segColor;
            if (distFromCenter < 0.25f)
                segColor = Colors::meter;
            else if (distFromCenter < 0.6f)
                segColor = Colors::meterWarn;
            else
                segColor = Colors::meterBad;

            g.setColour(segColor.withAlpha(0.3f));
            g.strokePath(segment, PathStrokeType(4.0f, PathStrokeType::curved, PathStrokeType::rounded));
        }

        // Calculate needle angle
        float normalizedCents = jlimit(-1.0f, 1.0f, cents / maxCents);
        float needleAngle = normalizedCents * MathConstants<float>::pi * 0.75f - MathConstants<float>::halfPi;

        // Draw needle
        Path needle;
        float needleLength = radius * 0.85f;
        float needleEndX = centerX + std::cos(needleAngle) * needleLength;
        float needleEndY = centerY + std::sin(needleAngle) * needleLength;

        needle.startNewSubPath(centerX, centerY);
        needle.lineTo(needleEndX, needleEndY);

        // Needle color based on tuning accuracy
        Colour needleColor;
        float absCents = std::abs(cents);
        if (absCents < 5.0f)
            needleColor = Colors::meter;
        else if (absCents < 15.0f)
            needleColor = Colors::meterWarn;
        else
            needleColor = Colors::meterBad;

        g.setColour(needleColor);
        g.strokePath(needle, PathStrokeType(3.0f, PathStrokeType::curved, PathStrokeType::rounded));

        // Draw center dot
        g.setColour(Colors::textPrimary);
        g.fillEllipse(centerX - 6, centerY - 6, 12, 12);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModernLookAndFeel)
};
