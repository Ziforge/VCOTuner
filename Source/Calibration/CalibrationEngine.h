/*
  ==============================================================================

    CalibrationEngine.h
    Automated calibration sweep engine

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "CalibrationTable.h"
#include "../CVOutput/CVOutputManager.h"
#include "../VCOTuner.h"

class CalibrationEngine : public VCOTuner::Listener,
                          private Timer
{
public:
    struct CalibrationSettings
    {
        int startNote = 24;           // C1
        int endNote = 96;             // C7
        int noteStep = 1;             // Every semitone
        int settleTimeMs = 200;       // Time for VCO to stabilize after CV change
        int measurementsPerNote = 1;  // Number of measurements to average
        CVOutputManager::VoltageStandard standard = CVOutputManager::VoltageStandard::OneVoltPerOctave;
        bool useExternalCVSource = false;  // Use o_C or other external CV instead
    };

    struct CalibrationPoint
    {
        int targetMidiNote = 0;
        float targetVoltage = 0.0f;
        float measuredFrequency = 0.0f;
        float measuredPitch = 0.0f;     // Calculated from frequency
        float pitchError = 0.0f;        // Deviation from ideal in semitones
        float errorCents = 0.0f;        // Error in cents
        float voltageCorrection = 0.0f;
        float stdDevCents = 0.0f;
        Time timestamp;
    };

    // Listener interface
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void calibrationStarted() {}
        virtual void calibrationPointCompleted(const CalibrationPoint& point) {}
        virtual void calibrationProgress(float percent, const String& status) {}
        virtual void calibrationCompleted(const CalibrationTable& table) {}
        virtual void calibrationError(const String& error) {}
        virtual void calibrationCancelled() {}
    };

    CalibrationEngine(VCOTuner* tuner, CVOutputManager* cvOutput);
    ~CalibrationEngine() override;

    // Control
    void startCalibration(const CalibrationSettings& settings);
    void pauseCalibration();
    void resumeCalibration();
    void cancelCalibration();

    // For external CV source mode - user triggers each measurement
    void triggerManualMeasurement(float knownVoltage);

    // State
    bool isRunning() const { return state != State::Idle && state != State::Completed && state != State::Error; }
    bool isPaused() const { return state == State::Paused; }

    // Progress
    int getTotalPoints() const;
    int getCompletedPoints() const { return static_cast<int>(calibrationData.size()); }
    float getProgressPercent() const;
    const CalibrationPoint& getCurrentPoint() const { return currentPoint; }

    // Results
    const std::vector<CalibrationPoint>& getCalibrationData() const { return calibrationData; }
    CalibrationTable generateCalibrationTable() const;

    // Listener management
    void addListener(Listener* l) { listeners.add(l); }
    void removeListener(Listener* l) { listeners.remove(l); }

    // VCOTuner::Listener overrides
    void newMeasurementReady(const VCOTuner::measurement_t& m) override;
    void tunerStarted() override {}
    void tunerStopped() override;
    void tunerFinished() override {}
    void tunerStatusChanged(String statusString) override {}

private:
    enum class State
    {
        Idle,
        Starting,
        SettlingVoltage,
        WaitingForMeasurement,
        ProcessingResult,
        MovingToNext,
        Paused,
        Completed,
        Error
    };

    void timerCallback() override;
    void advanceToNextPoint();
    void processCurrentMeasurement(const VCOTuner::measurement_t& m);
    void outputCurrentVoltage();
    void startMeasurement();
    void finishCalibration();
    void setError(const String& error);

    VCOTuner* tuner;
    CVOutputManager* cvOutput;
    ListenerList<Listener> listeners;

    State state = State::Idle;
    CalibrationSettings settings;

    int currentNoteIndex = 0;
    int currentMeasurementCount = 0;
    CalibrationPoint currentPoint;
    std::vector<CalibrationPoint> calibrationData;

    // For averaging multiple measurements
    std::vector<float> frequencyAccumulator;

    // Timing
    int settleCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CalibrationEngine)
};
