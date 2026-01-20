/*
  ==============================================================================

    CalibrationEngine.cpp
    Automated calibration sweep engine

  ==============================================================================
*/

#include "CalibrationEngine.h"
#include <cmath>

CalibrationEngine::CalibrationEngine(VCOTuner* t, CVOutputManager* cv)
    : tuner(t), cvOutput(cv)
{
    if (tuner != nullptr)
        tuner->addListener(this);
}

CalibrationEngine::~CalibrationEngine()
{
    stopTimer();
    if (tuner != nullptr)
        tuner->removeListener(this);
}

void CalibrationEngine::startCalibration(const CalibrationSettings& s)
{
    if (tuner == nullptr || cvOutput == nullptr)
    {
        setError("Tuner or CV output not configured");
        return;
    }

    settings = s;
    calibrationData.clear();
    frequencyAccumulator.clear();
    currentNoteIndex = 0;
    currentMeasurementCount = 0;

    // Configure CV output
    cvOutput->setVoltageStandard(settings.standard);
    cvOutput->setActive(true);

    state = State::Starting;
    listeners.call(&Listener::calibrationStarted);
    listeners.call(&Listener::calibrationProgress, 0.0f, "Starting calibration...");

    // Start the state machine timer (10ms intervals like VCOTuner)
    startTimer(10);
}

void CalibrationEngine::pauseCalibration()
{
    if (isRunning())
    {
        state = State::Paused;
        stopTimer();
        cvOutput->setActive(false);
    }
}

void CalibrationEngine::resumeCalibration()
{
    if (state == State::Paused)
    {
        cvOutput->setActive(true);
        state = State::SettlingVoltage;
        settleCounter = 0;
        startTimer(10);
    }
}

void CalibrationEngine::cancelCalibration()
{
    stopTimer();
    state = State::Idle;
    cvOutput->setActive(false);
    listeners.call(&Listener::calibrationCancelled);
}

void CalibrationEngine::triggerManualMeasurement(float knownVoltage)
{
    if (!settings.useExternalCVSource)
        return;

    // In external CV source mode, user provides the voltage they set
    currentPoint.targetVoltage = knownVoltage;
    currentPoint.targetMidiNote = static_cast<int>(std::round(cvOutput->voltageToMidi(knownVoltage)));

    state = State::WaitingForMeasurement;
    startMeasurement();
}

int CalibrationEngine::getTotalPoints() const
{
    if (settings.noteStep <= 0)
        return 0;
    return ((settings.endNote - settings.startNote) / settings.noteStep) + 1;
}

float CalibrationEngine::getProgressPercent() const
{
    int total = getTotalPoints();
    if (total <= 0)
        return 0.0f;
    return (static_cast<float>(calibrationData.size()) / total) * 100.0f;
}

CalibrationTable CalibrationEngine::generateCalibrationTable() const
{
    CalibrationTable table;

    for (const auto& point : calibrationData)
    {
        CalibrationTable::Entry entry;
        entry.midiNote = point.targetMidiNote;
        entry.idealVoltage = cvOutput->midiToVoltage(point.targetMidiNote);
        entry.actualVoltage = entry.idealVoltage + point.voltageCorrection;
        entry.correctionOffset = point.voltageCorrection;
        entry.measuredFrequency = point.measuredFrequency;
        entry.errorCents = point.errorCents;
        entry.stdDevCents = point.stdDevCents;

        table.addEntry(entry);
    }

    table.sortByMidiNote();
    table.setCalibrationDate(Time::getCurrentTime());

    return table;
}

void CalibrationEngine::timerCallback()
{
    switch (state)
    {
        case State::Starting:
            // Initialize first point
            currentNoteIndex = 0;
            currentPoint = CalibrationPoint();
            currentPoint.targetMidiNote = settings.startNote;
            currentPoint.targetVoltage = cvOutput->midiToVoltage(settings.startNote);

            outputCurrentVoltage();
            state = State::SettlingVoltage;
            settleCounter = 0;
            break;

        case State::SettlingVoltage:
            settleCounter++;
            // Wait for settle time (counter * 10ms)
            if (settleCounter * 10 >= settings.settleTimeMs)
            {
                state = State::WaitingForMeasurement;
                startMeasurement();
            }
            break;

        case State::WaitingForMeasurement:
            // Waiting for VCOTuner callback - nothing to do here
            break;

        case State::ProcessingResult:
            // Measurement complete, check if we need more measurements for averaging
            currentMeasurementCount++;

            if (currentMeasurementCount >= settings.measurementsPerNote)
            {
                // Done with this note
                calibrationData.push_back(currentPoint);
                listeners.call(&Listener::calibrationPointCompleted, currentPoint);

                String status = "Note " + String(currentPoint.targetMidiNote) +
                               ": " + String(currentPoint.errorCents, 1) + " cents error";
                listeners.call(&Listener::calibrationProgress, getProgressPercent(), status);

                state = State::MovingToNext;
            }
            else
            {
                // Need more measurements
                state = State::WaitingForMeasurement;
                startMeasurement();
            }
            break;

        case State::MovingToNext:
            advanceToNextPoint();
            break;

        case State::Completed:
        case State::Error:
        case State::Paused:
        case State::Idle:
            stopTimer();
            break;
    }
}

void CalibrationEngine::advanceToNextPoint()
{
    currentNoteIndex++;
    int currentNote = settings.startNote + (currentNoteIndex * settings.noteStep);

    if (currentNote > settings.endNote)
    {
        // Done!
        finishCalibration();
        return;
    }

    // Setup next point
    currentPoint = CalibrationPoint();
    currentPoint.targetMidiNote = currentNote;
    currentPoint.targetVoltage = cvOutput->midiToVoltage(currentNote);
    currentMeasurementCount = 0;
    frequencyAccumulator.clear();

    outputCurrentVoltage();
    state = State::SettlingVoltage;
    settleCounter = 0;
}

void CalibrationEngine::outputCurrentVoltage()
{
    if (!settings.useExternalCVSource && cvOutput != nullptr)
    {
        cvOutput->outputVoltage(currentPoint.targetVoltage);
    }
}

void CalibrationEngine::startMeasurement()
{
    if (tuner != nullptr)
    {
        // Use single measurement mode
        tuner->startSingleMeasurement(currentPoint.targetMidiNote);
    }
}

void CalibrationEngine::newMeasurementReady(const VCOTuner::measurement_t& m)
{
    if (state != State::WaitingForMeasurement)
        return;

    processCurrentMeasurement(m);
    state = State::ProcessingResult;
}

void CalibrationEngine::tunerStopped()
{
    // Tuner stopped unexpectedly - might be an error
    if (state == State::WaitingForMeasurement)
    {
        setError("Measurement failed - no signal detected");
    }
}

void CalibrationEngine::processCurrentMeasurement(const VCOTuner::measurement_t& m)
{
    frequencyAccumulator.push_back(static_cast<float>(m.frequency));

    // Calculate average frequency
    float avgFreq = 0.0f;
    for (float f : frequencyAccumulator)
        avgFreq += f;
    avgFreq /= frequencyAccumulator.size();

    // Calculate ideal frequency for this MIDI note
    float idealFreq = 440.0f * std::pow(2.0f, (currentPoint.targetMidiNote - 69) / 12.0f);

    // Calculate measured pitch (in MIDI note numbers)
    float measuredPitch = 69.0f + 12.0f * std::log2(avgFreq / 440.0f);

    // Calculate pitch error in semitones
    float pitchError = measuredPitch - currentPoint.targetMidiNote;

    // Convert to cents
    float errorCents = pitchError * 100.0f;

    // Calculate voltage correction needed
    // For 1V/Oct: if we're sharp, we need less voltage
    float voltageCorrection = 0.0f;
    if (settings.standard == CVOutputManager::VoltageStandard::OneVoltPerOctave)
    {
        voltageCorrection = -pitchError / 12.0f;  // Convert semitones to volts
    }
    else
    {
        // Hz/V: more complex - would need to adjust based on frequency
        float targetVoltage = idealFreq / 1000.0f;  // Assuming 1V = 1kHz
        float actualVoltage = avgFreq / 1000.0f;
        voltageCorrection = targetVoltage - actualVoltage;
    }

    // Update current point
    currentPoint.measuredFrequency = avgFreq;
    currentPoint.measuredPitch = measuredPitch;
    currentPoint.pitchError = pitchError;
    currentPoint.errorCents = errorCents;
    currentPoint.voltageCorrection = voltageCorrection;
    currentPoint.stdDevCents = static_cast<float>(m.pitchDeviation) * 100.0f;
    currentPoint.timestamp = Time::getCurrentTime();
}

void CalibrationEngine::finishCalibration()
{
    stopTimer();
    cvOutput->setActive(false);
    state = State::Completed;

    CalibrationTable table = generateCalibrationTable();
    listeners.call(&Listener::calibrationCompleted, table);
}

void CalibrationEngine::setError(const String& error)
{
    stopTimer();
    cvOutput->setActive(false);
    state = State::Error;
    listeners.call(&Listener::calibrationError, error);
}
