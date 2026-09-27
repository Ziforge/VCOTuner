/*
  ==============================================================================

    MainComponent.cpp
    Created: 17 May 2016 8:21:04pm
    Author:  Johannes Neumann

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "MainComponent.h"
#include "ReportCreatorWindow.h"
#include "CVCalibrationWindow.h"
#include "ModernLookAndFeel.h"
#include "TunerDisplay.h"

/** The one ModernLookAndFeel the app uses.

    Constructed on first call rather than at file scope. A LookAndFeel built
    during static initialisation races JUCE's own static Colours, which JUCE
    asserts on in a debug build ("you're using a static LookAndFeel object");
    in a release build the assertion is gone but the ordering is still
    undefined. A function-local static is constructed after main() has started
    and destroyed after shutdown() has already dropped the components that
    refer to it.
*/
static ModernLookAndFeel& getModernLookAndFeel()
{
    static ModernLookAndFeel instance;
    return instance;
}

MainComponent::MainComponent() : tuner(&deviceManager), tunerDisplay(&tuner), display(&tuner)
{
    // Apply modern look and feel
    LookAndFeel::setDefaultLookAndFeel(&getModernLookAndFeel());

    std::unique_ptr<XmlElement> savedAudioState (getAppProperties().getUserSettings()
                                               ->getXmlValue ("audioDeviceState"));

    // Enable both input (1) and output (1) for CV generation
    deviceManager.initialise (1, 1, savedAudioState.get(), true);

    // Create CV output manager
    cvOutput = std::make_unique<CVOutputManager>();
    tuner.setCVOutputManager(cvOutput.get());

    setVisible (true);

    Process::setPriority (Process::HighPriority);

    audioSettings.setName("AudioSettingsBttn");
    audioSettings.setButtonText("Audio Settings");
    audioSettings.addListener(this);
    addAndMakeVisible(&audioSettings);

    startStop.setName("StartStopBttn");
    startStop.setButtonText("Start");
    startStop.addListener(this);
    addAndMakeVisible(&startStop);

    report.setName("ReportBttn");
    report.setButtonText("Create Report");
    report.addListener(this);
    addAndMakeVisible(&report);

    cvCalibration.setName("CVCalibrationBttn");
    cvCalibration.setButtonText("CV Calibration");
    cvCalibration.addListener(this);
    addAndMakeVisible(&cvCalibration);

    statusLabel.setName("Status Label");
    statusLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(&statusLabel);

    failureLabel.setName("Failure Label");
    failureLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(&failureLabel);

    pitchSourceLabel.setName("Pitch Source Label");
    pitchSourceLabel.setText("Pitch source: ", dontSendNotification);
    pitchSourceLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(&pitchSourceLabel);

    pitchSource.setName("PitchSourceSelector");
    pitchSource.addItem("MIDI out", 1);
    pitchSource.addItem("CV output", 2);
    pitchSource.addListener(this);
    pitchSource.setSelectedId(
        getAppProperties().getUserSettings()->getIntValue("PitchSourceID", 1),
        dontSendNotification);
    tuner.setPitchSource(pitchSource.getSelectedId() == 2
                         ? VCOTuner::PitchSource::cvOutput
                         : VCOTuner::PitchSource::midiOut);
    addAndMakeVisible(&pitchSource);

    regimeLabel.setName("Regime Label");
    regimeLabel.setText("Pitch range: ", dontSendNotification);
    regimeLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(&regimeLabel);

    regime.setName("RegimeSelector");
    regime.addItemList(StringArray(regimeTexts, numRegimes), 1);
    regime.addListener(this);
    if (getAppProperties().getUserSettings()->containsKey("RegimeID"))
        regime.setSelectedId(getAppProperties().getUserSettings()->getIntValue("RegimeID"));
    else
        regime.setSelectedId(1);
    addAndMakeVisible(&regime);

    resolutionLabel.setName("Resolution Label");
    resolutionLabel.setText("Resolution: ", dontSendNotification);
    resolutionLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(&resolutionLabel);

    resolution.setName("ResolutionSelector");
    resolution.addItemList(StringArray(resolutionsTexts, numResolutions), 1);
    resolution.addListener(this);
    if (getAppProperties().getUserSettings()->containsKey("ResolutionID"))
        resolution.setSelectedId(getAppProperties().getUserSettings()->getIntValue("ResolutionID"));
    else
        resolution.setSelectedId(1);
    addAndMakeVisible(&resolution);

    // Create tabbed component with tuner and chart views
    tabs = std::make_unique<TabbedComponent>(TabbedButtonBar::TabsAtTop);
    tabs->setTabBarDepth(32);
    tabs->setOutline(0);
    tabs->addTab("Tuner", ModernLookAndFeel::Colors::background, &tunerDisplay, false);
    tabs->addTab("Chart", ModernLookAndFeel::Colors::background, &display, false);
    tabs->setCurrentTabIndex(0);  // Start on Tuner tab
    addAndMakeVisible(tabs.get());

    tuner.addListener(this);
    tuner.addListener(&tunerDisplay);
    tuner.addListener(&display);
    if (getAppProperties().getUserSettings()->containsKey("MIDIChannel"))
        tuner.setMidiChannel(getAppProperties().getUserSettings()->getIntValue("MIDIChannel"));
    else
        tuner.setMidiChannel(1);
    
    cycle = false;

    // for first-time starters, display a help message and the audio settings
    if ((!getAppProperties().getUserSettings()->containsKey("hideWelcomeScreen"))
        || (getAppProperties().getUserSettings()->getIntValue("hideWelcomeScreen") != 1))
    {
        NativeMessageBox::showMessageBox(AlertWindow::AlertIconType::InfoIcon, "Welcome!", welcomeText);
        showAudioSettings();
        getAppProperties().getUserSettings()->setValue("hideWelcomeScreen", 1);
    }
}

MainComponent::~MainComponent()
{
    tuner.removeListener(this);
    tuner.removeListener(&tunerDisplay);
    tuner.removeListener(&display);
    getAppProperties().getUserSettings()->setValue("RegimeID", regime.getSelectedId());
    getAppProperties().getUserSettings()->setValue("ResolutionID", resolution.getSelectedId());
    getAppProperties().getUserSettings()->setValue("PitchSourceID", pitchSource.getSelectedId());
}


void MainComponent::resized()
{
    const int borderWidth = 10;
    const int buttonWidth = 100;
    const int buttonHeight = 24;

    audioSettings.setBounds(borderWidth, borderWidth, buttonWidth, buttonHeight);
    cvCalibration.setBounds(audioSettings.getRight() + borderWidth, borderWidth, buttonWidth, buttonHeight);
    report.setBounds(getWidth() - buttonWidth - borderWidth, borderWidth, buttonWidth, buttonHeight);
    startStop.setBounds(report.getX() - buttonWidth - borderWidth, borderWidth, buttonWidth, buttonHeight);
    statusLabel.setBounds(cvCalibration.getRight() + borderWidth,
                          borderWidth,
                          startStop.getX() - borderWidth - borderWidth - cvCalibration.getRight(),
                          buttonHeight);

    regime.setBounds(getWidth() - 120 - borderWidth, audioSettings.getBottom() + borderWidth, 120, buttonHeight);
    regimeLabel.setBounds(regime.getX() - 80 - borderWidth, audioSettings.getBottom() + borderWidth, 80, buttonHeight);
    resolution.setBounds(regimeLabel.getX() - 120 - borderWidth, audioSettings.getBottom() + borderWidth, 120, buttonHeight);
    resolutionLabel.setBounds(resolution.getX() - 80 - borderWidth, audioSettings.getBottom() + borderWidth, 80, buttonHeight);

    // Shares the settings row rather than adding a second one: another row
    // would push the tabbed area down, and TunerDisplay lays its readouts out
    // at fixed offsets that then overflow the panel.
    pitchSourceLabel.setBounds(borderWidth, audioSettings.getBottom() + borderWidth, 90, buttonHeight);
    pitchSource.setBounds(pitchSourceLabel.getRight(), audioSettings.getBottom() + borderWidth, 120, buttonHeight);

    // failureLabel gets a fixed-height row at the very bottom, beneath the
    // tabbed area. jmax guards a window shrunk past MainWindow's resize limits
    // (or that limit changing later) from handing the tabs a negative height.
    failureLabel.setBounds(borderWidth,
                           getHeight() - borderWidth - buttonHeight,
                           getWidth() - 2 * borderWidth,
                           buttonHeight);

    // Tabbed component takes the main area
    if (tabs != nullptr)
    {
        const int tabsTop = regimeLabel.getBottom() + borderWidth;
        tabs->setBounds(borderWidth,
                        tabsTop,
                        getWidth() - 2 * borderWidth,
                        jmax(0, failureLabel.getY() - borderWidth - tabsTop));
    }
}

void MainComponent::paint(Graphics& g)
{
    g.fillAll(ModernLookAndFeel::Colors::background);
}


void MainComponent::buttonClicked (Button* bttn)
{
    if (bttn == &audioSettings)
        showAudioSettings();
    else if (bttn == &startStop)
    {
        // re-apply the currently selected settings on a start.
        // this prevents the VCOTuner and the comboboxes being out of
        // sync after a report (when report is finished, the settings from the
        // report remain active but the comboboxes still show the old value)
        if (!tuner.isRunning())
        {
            comboBoxChanged(&regime);
            comboBoxChanged(&resolution);
        }
        tuner.toggleState();
        if (tuner.isRunning())
        {
            display.clearCache();
            cycle = true;
        }
    }
    else if (bttn == &report)
    {
        ReportCreatorWindow* reportWindow = new ReportCreatorWindow(&tuner, &display);

        DialogWindow::LaunchOptions o;
        o.content.setOwned (reportWindow);
        o.dialogTitle                   = "Create Report";
        o.componentToCentreAround       = this;
        o.dialogBackgroundColour        = Colours::lightgrey;
        o.escapeKeyTriggersCloseButton  = false;
        o.resizable                     = true;
        o.useNativeTitleBar             = true;

        o.launchAsync();
    }
    else if (bttn == &cvCalibration)
    {
        CVCalibrationWindow* calibrationWindow = new CVCalibrationWindow(&tuner, cvOutput.get(), &display);

        DialogWindow::LaunchOptions o;
        o.content.setOwned(calibrationWindow);
        o.dialogTitle                   = "CV Calibration";
        o.componentToCentreAround       = this;
        o.dialogBackgroundColour        = Colours::lightgrey;
        o.escapeKeyTriggersCloseButton  = false;
        o.resizable                     = true;
        o.useNativeTitleBar             = true;

        o.launchAsync();
    }
}

void MainComponent::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &regime)
    {
        bool wasRunning = false;
        bool wasCycling = cycle;
        if (tuner.isRunning())
        {
            wasRunning = true;
            tuner.toggleState();
        }
        
        int selected = comboBoxThatHasChanged->getSelectedId() - 1;
        tuner.setNumMeasurementRange(regimes[selected].startNote, regimes[selected].interval, regimes[selected].endNote);
        display.clearCache();
        
        
        if (wasRunning)
        {
            tuner.toggleState();
            cycle = wasCycling;
        }
    }
    else if (comboBoxThatHasChanged == &pitchSource)
    {
        // Switching where the pitch comes from mid-sweep would leave the
        // oscillator held by one source and driven by the other, so stop and
        // restart around the change the way the other settings do.
        bool wasRunning = false;
        bool wasCycling = cycle;
        if (tuner.isRunning())
        {
            wasRunning = true;
            tuner.toggleState();
        }

        tuner.setPitchSource(comboBoxThatHasChanged->getSelectedId() == 2
                             ? VCOTuner::PitchSource::cvOutput
                             : VCOTuner::PitchSource::midiOut);
        display.clearCache();

        if (wasRunning)
        {
            tuner.toggleState();
            cycle = wasCycling;
        }
    }
    else if (comboBoxThatHasChanged == &resolution)
    {
        bool wasRunning = false;
        bool wasCycling = cycle;
        if (tuner.isRunning())
        {
            wasRunning = true;
            tuner.toggleState();
        }
        
        int selected = comboBoxThatHasChanged->getSelectedId() - 1;
        tuner.setResolution(resolutions[selected]);        
        
        if (wasRunning)
        {
            tuner.toggleState();
            cycle = wasCycling;
        }
    }
}

void MainComponent::showAudioSettings()
{
    class SettingsWrapperComponent: public Component,
                                    public ComboBox::Listener,
                                    public TextButton::Listener
    {
    public:
        SettingsWrapperComponent(VCOTuner* tunerToUse, juce::AudioDeviceManager& m)
        : selectorComponent(m, 1, 1, 0, 0, false, true, false, false)
        {
            t = tunerToUse;

            // Added first so it sits behind the fixed controls below. It used
            // to be added last, which put it on top - so when it overflowed
            // the space given to it, it covered them.
            addAndMakeVisible(&selectorComponent);

            channelLabel.setName("MidiChannel Label");
            channelLabel.setText("MIDI Channel: ", dontSendNotification);
            channelLabel.setJustificationType(juce::Justification::centredRight);
            addAndMakeVisible(&channelLabel);
            
            channelEdit.setName("MidiChannel Edit");
            const char* items[16] = {"1", "2", "3", "4", "5", "6", "7", "8",
                                     "9", "10", "11", "12", "13", "14", "15", "16"};
            channelEdit.addItemList(StringArray(items, 16), 1);
            channelEdit.addListener(this);
            if (getAppProperties().getUserSettings()->containsKey("MIDIChannel"))
                channelEdit.setSelectedId(getAppProperties().getUserSettings()->getIntValue("MIDIChannel"));
            else
                channelEdit.setSelectedId(1);
            addAndMakeVisible(&channelEdit);
            
            close.setButtonText("Close");
            close.addListener(this);
            addAndMakeVisible(&close);
        }
        
        void comboBoxChanged (ComboBox* comboBoxThatHasChanged) override
        {
            int channel = comboBoxThatHasChanged->getSelectedId();
            t->setMidiChannel(channel);
            getAppProperties().getUserSettings()->setValue("MIDIChannel", channel);
        }
        
        void resized() override
        {
            const int height = selectorComponent.getItemHeight();
            const int border = 10;

            // Lay the fixed-height controls out from the bottom upward, and
            // give the device selector whatever is left.
            //
            // These used to be positioned off selectorComponent.getBottom().
            // AudioDeviceSelectorComponent overrides its own height in its
            // resized() to fit however many devices and channels are attached,
            // so its bottom edge is not where we put it - and when it grew, it
            // pushed the MIDI channel control clean off the bottom of the
            // dialog. That made the channel unreachable exactly when a device
            // was connected, which is the only time you need to set it.
            const int closeTop   = getHeight() - border - height;
            const int channelTop = closeTop - border - height;

            close.setBounds(border, closeTop, getWidth() - 2 * border, height);
            channelLabel.setBounds(0, channelTop, proportionOfWidth (0.35f), height);
            channelEdit.setBounds(proportionOfWidth (0.35f), channelTop, proportionOfWidth (0.6f), height);

            selectorComponent.setBounds(0, 0, getWidth(), jmax(0, channelTop - border));
        }
        
        void buttonClicked (Button* bttn) override
        {
            if (bttn == &close)
            {
                if (DialogWindow* dw = findParentComponentOfClass<DialogWindow>())
                    dw->exitModalState (0);
            }
        }
        
    private:
        AudioDeviceSelectorComponent selectorComponent;
        Label channelLabel;
        TextButton close;
        ComboBox channelEdit;
        VCOTuner* t;
    };
    
    SettingsWrapperComponent content(&tuner, deviceManager);
    content.setSize(400, 410);
    
    
    DialogWindow::LaunchOptions o;
    o.content.setNonOwned (&content);
    o.dialogTitle                   = "Audio & Midi Settings";
    o.componentToCentreAround       = this;
    o.dialogBackgroundColour        = Colours::lightgrey;
    o.escapeKeyTriggersCloseButton  = true;
    o.resizable                     = true;
    o.useNativeTitleBar             = true;
    
    o.runModal();
    
    std::unique_ptr<XmlElement> audioState (deviceManager.createStateXml());
    
    getAppProperties().getUserSettings()->setValue ("audioDeviceState", audioState.get());
    getAppProperties().getUserSettings()->saveIfNeeded();
}


void MainComponent::tunerStarted()
{
    startStop.setButtonText("Stop");

    // A new run: clear any failures left over from the previous one, so a
    // stale "Not reading: MIDI 84" doesn't linger once that note is being
    // re-measured (or the range/settings have changed). Fires on every path
    // that begins a run - the initial Start press and, in live tuning, every
    // automatic re-cycle from tunerFinished() - since all of them go through
    // VCOTuner::switchState(prepRefMeasurement).
    failureLabel.setText({}, dontSendNotification);
}

void MainComponent::tunerStatusChanged(String statusString)
{
    statusLabel.setText(statusString, juce::dontSendNotification);
}

void MainComponent::tunerStopped()
{
    // Fatal errors only (no MIDI device, audio device stopped, MIDI-to-CV not
    // responding, ...). Per-note failures never reach here - they go to the
    // status line via measurementFailed(), in both live and report mode.
    StringArray errors = tuner.getLastErrors();
    for (int i = 0; i < errors.size(); i++)
        NativeMessageBox::showMessageBox(AlertWindow::WarningIcon, "Error!", errors[i]);

    startStop.setButtonText("Start");
    cycle = false;
}

void MainComponent::tunerFinished()
{
    startStop.setButtonText("Start");

    // Live tuning (cycle == true) restarts the sweep below and runs until
    // Stop is pressed - there is no "end" to summarise here, and a dialog
    // that fired every cycle would just be noise, so this must never raise
    // one. The failure label under the graph is what live tuning shows
    // instead, and it already reflects the sweep that just finished.
    //
    // The "Create Report" wizard drives its own sweep independently of this
    // class (see ReportDetailsEditorScreen::tunerFinished(), which shows the
    // end-of-report failure summary via showMeasurementFailureSummary() at
    // the point the report's measurement is actually complete) - there is no
    // report-mode case for this class to handle.
    if (cycle)
        tuner.toggleState();
}

void MainComponent::measurementFailed (int /*midiPitch*/, vcotuner::MeasurementError reason)
{
    // Used only by the assertion below, which compiles away in a release
    // build -- hence the explicit ignore rather than a commented-out name.
    ignoreUnused (reason);

    // Per-note failures are never fatal - only the reasons that abort the
    // whole sweep (routed through tunerStopped instead) are.
    jassert (! vcotuner::isFatal (reason));

    const auto& failures = tuner.getFailures();

    if (failures.empty())
    {
        failureLabel.setText ({}, dontSendNotification);
        return;
    }

    // A long sweep with a bad connection can fail most of its notes; listing
    // all of them would overflow the label. Cap the list and note the rest.
    constexpr int maxNotesShown = 10;

    StringArray pitches;
    for (size_t i = 0; i < failures.size() && (int) i < maxNotesShown; ++i)
        pitches.add (String (failures[i].midiPitch));

    String text = "Not reading: MIDI " + pitches.joinIntoString (", ");
    if ((int) failures.size() > maxNotesShown)
        text << " (+" << (int) failures.size() - maxNotesShown << " more)";

    failureLabel.setText (text, dontSendNotification);
}

const MainComponent::regime_t MainComponent::regimes[numRegimes] = {
    {54, 66, 6},
    {54, 66, 3},
    {54, 66, 1},
    {48, 72, 12},
    {48, 72, 6},
    {48, 72, 1},
    {36, 84, 12},
    {36, 84, 6},
    {36, 84, 1},
    {24, 96, 12},
    {24, 96, 6},
    {24, 96, 1},
};
const char* MainComponent::regimeTexts[numRegimes] = {
    "narrow > coarse (54-66, +6)",
    "narrow > normal (54-66, +3)",
    "narrow > fine (54-66, +1)",
    "medium > coarse (48-72, +12)",
    "medium > normal (48-72, +6)",
    "medium > fine (48-72, +1)",
    "large > coarse (36-84, +12)",
    "large > normal (36-84, +6)",
    "large > fine (36-84, +1)",
    "huge > coarse (24-96, +12)",
    "huge > normal (24-96, +6)",
    "huge > fine (24-96, +1)",
};

const int MainComponent::resolutions[numResolutions] = {20, 50, 100, 200, 400};
const char* MainComponent::resolutionsTexts[numResolutions] = {
    "20 - quick & dirty",
    "50 - not quite enough",
    "100 - okay",
    "200 - neat and tidy",
    "400 - never accurate enough"
};

const String MainComponent::welcomeText = String("Welcome to the VCO Tuner!") + newLine + newLine + "Please follow these steps to get running:" + newLine + "1) connect a MIDI-CV interface to your Computer" + newLine + "2) connect the CV output of the interface to your oscillators frequency input" + newLine + "3) Connect one of the oscillators basic waveforms (sine, saw, triangle, pulse, etc.) directly to your soundcard (use attenuation to avoid clipping)." + newLine + newLine + "When you close this dialog, the audio settings panel will open. Please select your audio and midi device there." + newLine + newLine + "Have fun!" + newLine + newLine + "PS: If you find bugs, please raise an issue on the github repository under https://github.com/TheSlowGrowth/VCOTuner. Thanks!";

