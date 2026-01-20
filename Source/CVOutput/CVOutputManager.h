/*
  ==============================================================================

    CVOutputManager.h
    DC-coupled audio output for CV generation

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <vector>

class CVOutputManager
{
public:
    // Voltage standards
    enum class VoltageStandard
    {
        OneVoltPerOctave,   // 1V/Oct: MIDI 60 = 0V, each semitone = 1/12V
        HzPerVolt           // Hz/V: voltage proportional to frequency
    };

    // Interface presets
    enum class InterfaceType
    {
        ExpertSleepers,     // ES-8, ES-9: -10V to +10V
        MOTU,               // MOTU DC-coupled: typically -10V to +10V
        Generic,            // Generic DC-coupled
        Custom              // User-defined range
    };

    // Calibration method for interface
    enum class CalibrationMethod
    {
        None,               // No calibration (trust factory)
        Multimeter,         // User measures with DMM
        ReferenceSource,    // Use external reference (o_C, etc.)
        Loopback            // Output -> Input loopback test
    };

    // Interface calibration data
    struct InterfaceCalibration
    {
        bool isCalibrated = false;
        float gain = 1.0f;          // Correction multiplier
        float offset = 0.0f;        // Correction offset in volts
        std::vector<std::pair<float, float>> calibrationPoints;  // ideal -> actual
        Time calibrationDate;
        String interfaceName;
    };

    CVOutputManager();
    ~CVOutputManager();

    // Configuration
    void setVoltageStandard(VoltageStandard standard);
    VoltageStandard getVoltageStandard() const { return currentStandard; }

    void setInterfaceType(InterfaceType type);
    void setCustomVoltageRange(float minVolts, float maxVolts);
    void setOutputChannel(int channel) { outputChannel = channel; }
    void setHzPerVoltScale(float hzPerVolt) { hzPerVoltScaling = hzPerVolt; }

    // Activation
    void setActive(bool active) { isActiveFlag = active; }
    bool isActive() const { return isActiveFlag; }

    // Voltage output
    void outputVoltage(float volts);
    void outputPitch(int midiNote);
    void outputPitch(float midiPitchFloat);  // For microtonal
    void outputFrequency(float hz);
    float getCurrentVoltage() const { return currentOutputVoltage.load(); }

    // Audio callback - called from audio thread
    void fillOutputBuffer(float* buffer, int numSamples);

    // Interface calibration
    void setInterfaceCalibration(const InterfaceCalibration& cal);
    const InterfaceCalibration& getInterfaceCalibration() const { return interfaceCalibration; }
    void clearInterfaceCalibration();

    // Calibration helpers
    void addCalibrationPoint(float idealVoltage, float actualVoltage);
    void computeCalibrationFromPoints();

    // Persistence
    void saveCalibration(const File& file) const;
    bool loadCalibration(const File& file);

    // Conversion utilities (public for testing/display)
    float midiToVoltage(int midiNote) const;
    float midiToVoltage(float midiPitch) const;
    float frequencyToVoltage(float hz) const;
    float voltageToMidi(float voltage) const;

private:
    float voltageToSample(float volts) const;
    float sampleToVoltage(float sample) const;
    float applyInterfaceCalibration(float voltage) const;

    std::atomic<float> currentOutputVoltage{0.0f};
    std::atomic<bool> isActiveFlag{false};

    VoltageStandard currentStandard = VoltageStandard::OneVoltPerOctave;
    InterfaceType interfaceType = InterfaceType::ExpertSleepers;

    float interfaceMinVolts = -10.0f;
    float interfaceMaxVolts = +10.0f;
    float hzPerVoltScaling = 1000.0f;  // Default: 1V = 1kHz for Hz/V mode

    int outputChannel = 0;

    InterfaceCalibration interfaceCalibration;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CVOutputManager)
};
