/*
  ==============================================================================

    CVCalibrationWindow.h
    CV Calibration wizard window

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "VCOTuner.h"
#include "Visualizer.h"
#include "CVOutput/CVOutputManager.h"
#include "Calibration/CalibrationEngine.h"
#include "Calibration/CalibrationTable.h"

class CVCalibrationWindow;

//==============================================================================
// Setup Screen - Configure calibration parameters
class CVSetupScreen : public Component,
                      public Button::Listener,
                      public ComboBox::Listener
{
public:
    CVSetupScreen(CVCalibrationWindow* parent, CVOutputManager* cvOut);
    ~CVSetupScreen() override;

    void resized() override;
    void paint(Graphics& g) override;
    void buttonClicked(Button* button) override;
    void comboBoxChanged(ComboBox* combo) override;

    CalibrationEngine::CalibrationSettings getSettings() const;

private:
    CVCalibrationWindow* parent;
    CVOutputManager* cvOutput;

    Label titleLabel;

    Label standardLabel;
    ComboBox standardCombo;

    Label interfaceLabel;
    ComboBox interfaceCombo;

    Label rangeLabel;
    ComboBox rangeCombo;

    Label stepLabel;
    ComboBox stepCombo;

    Label settleLabel;
    Slider settleSlider;

    TextButton startButton;
    TextButton cancelButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CVSetupScreen)
};

//==============================================================================
// Running Screen - Shows calibration progress
class CVRunningScreen : public Component,
                        public CalibrationEngine::Listener,
                        public Button::Listener
{
public:
    CVRunningScreen(CVCalibrationWindow* parent, CalibrationEngine* engine);
    ~CVRunningScreen() override;

    void resized() override;
    void paint(Graphics& g) override;
    void buttonClicked(Button* button) override;

    // CalibrationEngine::Listener
    void calibrationStarted() override;
    void calibrationPointCompleted(const CalibrationEngine::CalibrationPoint& point) override;
    void calibrationProgress(float percent, const String& status) override;
    void calibrationCompleted(const CalibrationTable& table) override;
    void calibrationError(const String& error) override;
    void calibrationCancelled() override;

private:
    CVCalibrationWindow* parent;
    CalibrationEngine* engine;

    Label titleLabel;
    Label statusLabel;
    ProgressBar progressBar;

    Label currentNoteLabel;
    Label currentVoltageLabel;
    Label measuredFreqLabel;
    Label errorLabel;

    // Simple visualization of measurements
    std::vector<float> errorHistory;

    TextButton pauseButton;
    TextButton cancelButton;

    double progress = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CVRunningScreen)
};

//==============================================================================
// Results Screen - Shows calibration table and statistics
class CVResultsScreen : public Component,
                        public Button::Listener,
                        public TableListBoxModel
{
public:
    CVResultsScreen(CVCalibrationWindow* parent, const CalibrationTable& table);
    ~CVResultsScreen() override;

    void resized() override;
    void paint(Graphics& g) override;
    void buttonClicked(Button* button) override;

    // TableListBoxModel
    int getNumRows() override;
    void paintRowBackground(Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;

private:
    CVCalibrationWindow* parent;
    CalibrationTable calibrationTable;

    Label titleLabel;
    Label statsLabel;

    TableListBox tableBox;

    TextButton exportCSVButton;
    TextButton exportJSONButton;
    TextButton exportOCButton;
    TextButton doneButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CVResultsScreen)
};

//==============================================================================
// Main Calibration Window
class CVCalibrationWindow : public Component
{
public:
    CVCalibrationWindow(VCOTuner* tuner, CVOutputManager* cvOutput, Visualizer* visualizer);
    ~CVCalibrationWindow() override;

    void resized() override;

    // Screen navigation
    void showSetupScreen();
    void startCalibration(const CalibrationEngine::CalibrationSettings& settings);
    void showResults(const CalibrationTable& table);
    void close();

    VCOTuner* getTuner() { return tuner; }
    CVOutputManager* getCVOutput() { return cvOutput; }
    CalibrationEngine* getEngine() { return engine.get(); }

private:
    enum class Screen { Setup, Running, Results };

    VCOTuner* tuner;
    CVOutputManager* cvOutput;
    Visualizer* visualizer;

    std::unique_ptr<CalibrationEngine> engine;
    std::unique_ptr<Component> currentScreen;
    Screen currentScreenType = Screen::Setup;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CVCalibrationWindow)
};
