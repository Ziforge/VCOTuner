/*
  ==============================================================================

    CVCalibrationWindow.cpp
    CV Calibration wizard window

  ==============================================================================
*/

#include "CVCalibrationWindow.h"
#include "Export/CSVExporter.h"
#include "Export/JSONExporter.h"
#include "Export/OrnamentCrimeExporter.h"

//==============================================================================
// CVSetupScreen Implementation
//==============================================================================

CVSetupScreen::CVSetupScreen(CVCalibrationWindow* p, CVOutputManager* cv)
    : parent(p), cvOutput(cv)
{
    // Title
    titleLabel.setText("CV Calibration Setup", dontSendNotification);
    titleLabel.setFont(Font(24.0f, Font::bold));
    titleLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(titleLabel);

    // Voltage Standard
    standardLabel.setText("Voltage Standard:", dontSendNotification);
    addAndMakeVisible(standardLabel);

    standardCombo.addItem("1V/Oct (Eurorack standard)", 1);
    standardCombo.addItem("Hz/V (Korg/Yamaha)", 2);
    standardCombo.setSelectedId(1);
    standardCombo.addListener(this);
    addAndMakeVisible(standardCombo);

    // Interface Type
    interfaceLabel.setText("Audio Interface:", dontSendNotification);
    addAndMakeVisible(interfaceLabel);

    interfaceCombo.addItem("Expert Sleepers (ES-8/ES-9)", 1);
    interfaceCombo.addItem("MOTU DC-coupled", 2);
    interfaceCombo.addItem("Generic DC-coupled", 3);
    interfaceCombo.setSelectedId(1);
    interfaceCombo.addListener(this);
    addAndMakeVisible(interfaceCombo);

    // Note Range
    rangeLabel.setText("Note Range:", dontSendNotification);
    addAndMakeVisible(rangeLabel);

    rangeCombo.addItem("Full (C1-C7, 24-96)", 1);
    rangeCombo.addItem("Extended (C0-C8, 12-108)", 2);
    rangeCombo.addItem("Standard (C2-C6, 36-84)", 3);
    rangeCombo.addItem("Narrow (C3-C5, 48-72)", 4);
    rangeCombo.setSelectedId(1);
    rangeCombo.addListener(this);
    addAndMakeVisible(rangeCombo);

    // Step Size
    stepLabel.setText("Step Size:", dontSendNotification);
    addAndMakeVisible(stepLabel);

    stepCombo.addItem("Every semitone", 1);
    stepCombo.addItem("Every octave", 2);
    stepCombo.addItem("Every 2 semitones", 3);
    stepCombo.addItem("Every 3 semitones", 4);
    stepCombo.setSelectedId(1);
    stepCombo.addListener(this);
    addAndMakeVisible(stepCombo);

    // Settle Time
    settleLabel.setText("Settle Time (ms):", dontSendNotification);
    addAndMakeVisible(settleLabel);

    settleSlider.setRange(50, 500, 10);
    settleSlider.setValue(200);
    settleSlider.setTextBoxStyle(Slider::TextBoxRight, false, 60, 20);
    addAndMakeVisible(settleSlider);

    // Buttons
    startButton.setButtonText("Start Calibration");
    startButton.addListener(this);
    addAndMakeVisible(startButton);

    cancelButton.setButtonText("Cancel");
    cancelButton.addListener(this);
    addAndMakeVisible(cancelButton);
}

CVSetupScreen::~CVSetupScreen() {}

void CVSetupScreen::resized()
{
    auto bounds = getLocalBounds().reduced(20);

    titleLabel.setBounds(bounds.removeFromTop(40));
    bounds.removeFromTop(20);

    const int rowHeight = 30;
    const int labelWidth = 150;
    const int spacing = 10;

    auto row = bounds.removeFromTop(rowHeight);
    standardLabel.setBounds(row.removeFromLeft(labelWidth));
    standardCombo.setBounds(row.reduced(spacing, 0));
    bounds.removeFromTop(spacing);

    row = bounds.removeFromTop(rowHeight);
    interfaceLabel.setBounds(row.removeFromLeft(labelWidth));
    interfaceCombo.setBounds(row.reduced(spacing, 0));
    bounds.removeFromTop(spacing);

    row = bounds.removeFromTop(rowHeight);
    rangeLabel.setBounds(row.removeFromLeft(labelWidth));
    rangeCombo.setBounds(row.reduced(spacing, 0));
    bounds.removeFromTop(spacing);

    row = bounds.removeFromTop(rowHeight);
    stepLabel.setBounds(row.removeFromLeft(labelWidth));
    stepCombo.setBounds(row.reduced(spacing, 0));
    bounds.removeFromTop(spacing);

    row = bounds.removeFromTop(rowHeight);
    settleLabel.setBounds(row.removeFromLeft(labelWidth));
    settleSlider.setBounds(row.reduced(spacing, 0));
    bounds.removeFromTop(30);

    // Buttons at bottom
    auto buttonRow = bounds.removeFromBottom(35);
    cancelButton.setBounds(buttonRow.removeFromLeft(100));
    buttonRow.removeFromLeft(spacing);
    startButton.setBounds(buttonRow.removeFromRight(150));
}

void CVSetupScreen::paint(Graphics& g)
{
    g.fillAll(Colours::white);
}

void CVSetupScreen::buttonClicked(Button* button)
{
    if (button == &startButton)
    {
        parent->startCalibration(getSettings());
    }
    else if (button == &cancelButton)
    {
        parent->close();
    }
}

void CVSetupScreen::comboBoxChanged(ComboBox*) {}

CalibrationEngine::CalibrationSettings CVSetupScreen::getSettings() const
{
    CalibrationEngine::CalibrationSettings settings;

    // Voltage standard
    settings.standard = (standardCombo.getSelectedId() == 1)
        ? CVOutputManager::VoltageStandard::OneVoltPerOctave
        : CVOutputManager::VoltageStandard::HzPerVolt;

    // Note range
    switch (rangeCombo.getSelectedId())
    {
        case 1: settings.startNote = 24; settings.endNote = 96; break;   // Full
        case 2: settings.startNote = 12; settings.endNote = 108; break;  // Extended
        case 3: settings.startNote = 36; settings.endNote = 84; break;   // Standard
        case 4: settings.startNote = 48; settings.endNote = 72; break;   // Narrow
        default: settings.startNote = 24; settings.endNote = 96; break;
    }

    // Step size
    switch (stepCombo.getSelectedId())
    {
        case 1: settings.noteStep = 1; break;
        case 2: settings.noteStep = 12; break;
        case 3: settings.noteStep = 2; break;
        case 4: settings.noteStep = 3; break;
        default: settings.noteStep = 1; break;
    }

    settings.settleTimeMs = static_cast<int>(settleSlider.getValue());

    return settings;
}

//==============================================================================
// CVRunningScreen Implementation
//==============================================================================

CVRunningScreen::CVRunningScreen(CVCalibrationWindow* p, CalibrationEngine* e)
    : parent(p), engine(e), progressBar(progress)
{
    titleLabel.setText("Calibration in Progress", dontSendNotification);
    titleLabel.setFont(Font(24.0f, Font::bold));
    titleLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(titleLabel);

    statusLabel.setText("Starting...", dontSendNotification);
    statusLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(statusLabel);

    addAndMakeVisible(progressBar);

    currentNoteLabel.setText("Current Note: --", dontSendNotification);
    addAndMakeVisible(currentNoteLabel);

    currentVoltageLabel.setText("Output Voltage: -- V", dontSendNotification);
    addAndMakeVisible(currentVoltageLabel);

    measuredFreqLabel.setText("Measured Frequency: -- Hz", dontSendNotification);
    addAndMakeVisible(measuredFreqLabel);

    errorLabel.setText("Pitch Error: -- cents", dontSendNotification);
    addAndMakeVisible(errorLabel);

    pauseButton.setButtonText("Pause");
    pauseButton.addListener(this);
    addAndMakeVisible(pauseButton);

    cancelButton.setButtonText("Cancel");
    cancelButton.addListener(this);
    addAndMakeVisible(cancelButton);

    engine->addListener(this);
}

CVRunningScreen::~CVRunningScreen()
{
    engine->removeListener(this);
}

void CVRunningScreen::resized()
{
    auto bounds = getLocalBounds().reduced(20);

    titleLabel.setBounds(bounds.removeFromTop(40));
    bounds.removeFromTop(10);

    statusLabel.setBounds(bounds.removeFromTop(25));
    bounds.removeFromTop(10);

    progressBar.setBounds(bounds.removeFromTop(25));
    bounds.removeFromTop(20);

    const int rowHeight = 25;
    currentNoteLabel.setBounds(bounds.removeFromTop(rowHeight));
    currentVoltageLabel.setBounds(bounds.removeFromTop(rowHeight));
    measuredFreqLabel.setBounds(bounds.removeFromTop(rowHeight));
    errorLabel.setBounds(bounds.removeFromTop(rowHeight));

    // Buttons at bottom
    auto buttonRow = bounds.removeFromBottom(35);
    cancelButton.setBounds(buttonRow.removeFromLeft(100));
    buttonRow.removeFromLeft(10);
    pauseButton.setBounds(buttonRow.removeFromLeft(100));
}

void CVRunningScreen::paint(Graphics& g)
{
    g.fillAll(Colours::white);

    // Draw simple error history visualization
    if (!errorHistory.empty())
    {
        auto historyArea = getLocalBounds().reduced(20);
        historyArea.removeFromTop(220);
        historyArea.removeFromBottom(50);

        g.setColour(Colours::lightgrey);
        g.fillRect(historyArea);

        g.setColour(Colours::grey);
        g.drawRect(historyArea);

        // Draw zero line
        int centerY = historyArea.getCentreY();
        g.setColour(Colours::darkgrey);
        g.drawHorizontalLine(centerY, (float)historyArea.getX(), (float)historyArea.getRight());

        // Draw error points
        g.setColour(Colours::blue);
        float xStep = historyArea.getWidth() / (float)std::max(1, (int)errorHistory.size() - 1);
        float scale = historyArea.getHeight() / 100.0f;  // Scale for ±50 cents

        for (size_t i = 0; i < errorHistory.size(); ++i)
        {
            float x = historyArea.getX() + i * xStep;
            float y = centerY - errorHistory[i] * scale;
            y = jlimit((float)historyArea.getY(), (float)historyArea.getBottom(), y);

            g.fillEllipse(x - 3, y - 3, 6, 6);

            if (i > 0)
            {
                float prevX = historyArea.getX() + (i - 1) * xStep;
                float prevY = centerY - errorHistory[i - 1] * scale;
                prevY = jlimit((float)historyArea.getY(), (float)historyArea.getBottom(), prevY);
                g.drawLine(prevX, prevY, x, y);
            }
        }

        // Labels
        g.setColour(Colours::black);
        g.drawText("+50c", historyArea.getX() - 40, historyArea.getY() - 10, 35, 20, Justification::right);
        g.drawText("-50c", historyArea.getX() - 40, historyArea.getBottom() - 10, 35, 20, Justification::right);
        g.drawText("0", historyArea.getX() - 20, centerY - 10, 15, 20, Justification::right);
    }
}

void CVRunningScreen::buttonClicked(Button* button)
{
    if (button == &pauseButton)
    {
        if (engine->isPaused())
        {
            engine->resumeCalibration();
            pauseButton.setButtonText("Pause");
        }
        else
        {
            engine->pauseCalibration();
            pauseButton.setButtonText("Resume");
        }
    }
    else if (button == &cancelButton)
    {
        engine->cancelCalibration();
    }
}

void CVRunningScreen::calibrationStarted()
{
    statusLabel.setText("Calibration started...", dontSendNotification);
    errorHistory.clear();
    repaint();
}

void CVRunningScreen::calibrationPointCompleted(const CalibrationEngine::CalibrationPoint& point)
{
    currentNoteLabel.setText("Current Note: MIDI " + String(point.targetMidiNote), dontSendNotification);
    currentVoltageLabel.setText("Output Voltage: " + String(point.targetVoltage, 3) + " V", dontSendNotification);
    measuredFreqLabel.setText("Measured Frequency: " + String(point.measuredFrequency, 2) + " Hz", dontSendNotification);

    String errorText = "Pitch Error: " + String(point.errorCents, 1) + " cents";
    if (std::abs(point.errorCents) < 5)
        errorLabel.setColour(Label::textColourId, Colours::darkgreen);
    else if (std::abs(point.errorCents) < 15)
        errorLabel.setColour(Label::textColourId, Colours::orange);
    else
        errorLabel.setColour(Label::textColourId, Colours::red);
    errorLabel.setText(errorText, dontSendNotification);

    errorHistory.push_back(point.errorCents);
    repaint();
}

void CVRunningScreen::calibrationProgress(float percent, const String& status)
{
    progress = percent / 100.0;
    statusLabel.setText(status, dontSendNotification);
    repaint();
}

void CVRunningScreen::calibrationCompleted(const CalibrationTable& table)
{
    statusLabel.setText("Calibration complete!", dontSendNotification);
    progress = 1.0;

    // Small delay before showing results
    Timer::callAfterDelay(500, [this, table]() {
        parent->showResults(table);
    });
}

void CVRunningScreen::calibrationError(const String& error)
{
    statusLabel.setText("Error: " + error, dontSendNotification);
    statusLabel.setColour(Label::textColourId, Colours::red);
}

void CVRunningScreen::calibrationCancelled()
{
    parent->showSetupScreen();
}

//==============================================================================
// CVResultsScreen Implementation
//==============================================================================

CVResultsScreen::CVResultsScreen(CVCalibrationWindow* p, const CalibrationTable& table)
    : parent(p), calibrationTable(table)
{
    titleLabel.setText("Calibration Results", dontSendNotification);
    titleLabel.setFont(Font(24.0f, Font::bold));
    titleLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(titleLabel);

    // Statistics
    String statsText;
    statsText += "Points: " + String(calibrationTable.getEntryCount()) + "   |   ";
    statsText += "Max Error: " + String(calibrationTable.getMaxErrorCents(), 1) + " cents   |   ";
    statsText += "Avg Error: " + String(calibrationTable.getAverageErrorCents(), 1) + " cents   |   ";
    statsText += "RMS: " + String(calibrationTable.getRMSErrorCents(), 1) + " cents";
    statsLabel.setText(statsText, dontSendNotification);
    statsLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(statsLabel);

    // Table
    tableBox.setModel(this);
    tableBox.getHeader().addColumn("MIDI", 1, 60);
    tableBox.getHeader().addColumn("Ideal V", 2, 80);
    tableBox.getHeader().addColumn("Corrected V", 3, 90);
    tableBox.getHeader().addColumn("Freq (Hz)", 4, 90);
    tableBox.getHeader().addColumn("Error (c)", 5, 80);
    addAndMakeVisible(tableBox);

    // Export buttons
    exportCSVButton.setButtonText("Export CSV");
    exportCSVButton.addListener(this);
    addAndMakeVisible(exportCSVButton);

    exportJSONButton.setButtonText("Export JSON");
    exportJSONButton.addListener(this);
    addAndMakeVisible(exportJSONButton);

    exportOCButton.setButtonText("Export o_C");
    exportOCButton.addListener(this);
    addAndMakeVisible(exportOCButton);

    doneButton.setButtonText("Done");
    doneButton.addListener(this);
    addAndMakeVisible(doneButton);
}

CVResultsScreen::~CVResultsScreen() {}

void CVResultsScreen::resized()
{
    auto bounds = getLocalBounds().reduced(20);

    titleLabel.setBounds(bounds.removeFromTop(40));
    bounds.removeFromTop(5);

    statsLabel.setBounds(bounds.removeFromTop(25));
    bounds.removeFromTop(10);

    // Buttons at bottom
    auto buttonRow = bounds.removeFromBottom(35);
    doneButton.setBounds(buttonRow.removeFromRight(80));
    buttonRow.removeFromRight(10);
    exportOCButton.setBounds(buttonRow.removeFromRight(100));
    buttonRow.removeFromRight(10);
    exportJSONButton.setBounds(buttonRow.removeFromRight(100));
    buttonRow.removeFromRight(10);
    exportCSVButton.setBounds(buttonRow.removeFromRight(100));

    bounds.removeFromBottom(10);
    tableBox.setBounds(bounds);
}

void CVResultsScreen::paint(Graphics& g)
{
    g.fillAll(Colours::white);
}

void CVResultsScreen::buttonClicked(Button* button)
{
    if (button == &exportCSVButton)
    {
        FileChooser chooser("Save CSV...", File(), "*.csv");
        if (chooser.browseForFileToSave(true))
        {
            CSVExporter::exportCalibration(calibrationTable, chooser.getResult().withFileExtension("csv"));
        }
    }
    else if (button == &exportJSONButton)
    {
        FileChooser chooser("Save JSON...", File(), "*.json");
        if (chooser.browseForFileToSave(true))
        {
            JSONExporter::exportCalibration(calibrationTable, chooser.getResult().withFileExtension("json"));
        }
    }
    else if (button == &exportOCButton)
    {
        FileChooser chooser("Save o_C Header...", File(), "*.h");
        if (chooser.browseForFileToSave(true))
        {
            OrnamentCrimeExporter::exportAsCHeader(calibrationTable, chooser.getResult().withFileExtension("h"));
        }
    }
    else if (button == &doneButton)
    {
        parent->close();
    }
}

int CVResultsScreen::getNumRows()
{
    return calibrationTable.getEntryCount();
}

void CVResultsScreen::paintRowBackground(Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll(Colours::lightblue);
    else if (rowNumber % 2)
        g.fillAll(Colour(0xffeeeeee));
}

void CVResultsScreen::paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool /*rowIsSelected*/)
{
    g.setColour(Colours::black);

    if (rowNumber >= 0 && rowNumber < calibrationTable.getEntryCount())
    {
        const auto& entry = calibrationTable.getEntry(rowNumber);
        String text;

        switch (columnId)
        {
            case 1: text = String(entry.midiNote); break;
            case 2: text = String(entry.idealVoltage, 3); break;
            case 3: text = String(entry.actualVoltage, 3); break;
            case 4: text = String(entry.measuredFrequency, 1); break;
            case 5:
                text = String(entry.errorCents, 1);
                if (std::abs(entry.errorCents) > 10)
                    g.setColour(Colours::red);
                else if (std::abs(entry.errorCents) > 5)
                    g.setColour(Colours::orange);
                break;
        }

        g.drawText(text, 4, 0, width - 8, height, Justification::centredLeft);
    }
}

//==============================================================================
// CVCalibrationWindow Implementation
//==============================================================================

CVCalibrationWindow::CVCalibrationWindow(VCOTuner* t, CVOutputManager* cv, Visualizer* v)
    : tuner(t), cvOutput(cv), visualizer(v)
{
    engine = std::make_unique<CalibrationEngine>(tuner, cvOutput);
    showSetupScreen();
    setSize(500, 450);
}

CVCalibrationWindow::~CVCalibrationWindow()
{
    if (engine->isRunning())
        engine->cancelCalibration();
}

void CVCalibrationWindow::resized()
{
    if (currentScreen)
        currentScreen->setBounds(getLocalBounds());
}

void CVCalibrationWindow::showSetupScreen()
{
    currentScreen = std::make_unique<CVSetupScreen>(this, cvOutput);
    addAndMakeVisible(currentScreen.get());
    currentScreenType = Screen::Setup;
    resized();
}

void CVCalibrationWindow::startCalibration(const CalibrationEngine::CalibrationSettings& settings)
{
    currentScreen = std::make_unique<CVRunningScreen>(this, engine.get());
    addAndMakeVisible(currentScreen.get());
    currentScreenType = Screen::Running;
    resized();

    engine->startCalibration(settings);
}

void CVCalibrationWindow::showResults(const CalibrationTable& table)
{
    currentScreen = std::make_unique<CVResultsScreen>(this, table);
    addAndMakeVisible(currentScreen.get());
    currentScreenType = Screen::Results;
    resized();
}

void CVCalibrationWindow::close()
{
    if (auto* dw = findParentComponentOfClass<DialogWindow>())
        dw->exitModalState(0);
}
