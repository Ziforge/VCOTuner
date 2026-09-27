/*
  ==============================================================================

    CVOutputManager.cpp
    DC-coupled audio output for CV generation

  ==============================================================================
*/

#include "CVOutputManager.h"

namespace
{
    vcotuner::CvStandard toCvStandard (CVOutputManager::VoltageStandard s)
    {
        return (s == CVOutputManager::VoltageStandard::HzPerVolt)
             ? vcotuner::CvStandard::hzPerVolt
             : vcotuner::CvStandard::oneVoltPerOctave;
    }
}

CVOutputManager::CVOutputManager()
{
}

CVOutputManager::~CVOutputManager()
{
}

void CVOutputManager::setVoltageStandard(VoltageStandard standard)
{
    currentStandard = standard;
}

void CVOutputManager::setInterfaceType(InterfaceType type)
{
    interfaceType = type;

    // Set voltage ranges based on interface type
    switch (type)
    {
        case InterfaceType::ExpertSleepers:
            interfaceMinVolts = -10.0f;
            interfaceMaxVolts = +10.0f;
            break;
        case InterfaceType::MOTU:
            interfaceMinVolts = -10.0f;
            interfaceMaxVolts = +10.0f;
            break;
        case InterfaceType::Generic:
            interfaceMinVolts = -10.0f;
            interfaceMaxVolts = +10.0f;
            break;
        case InterfaceType::Custom:
            // Keep existing custom values
            break;
    }
}

void CVOutputManager::setCustomVoltageRange(float minVolts, float maxVolts)
{
    interfaceMinVolts = minVolts;
    interfaceMaxVolts = maxVolts;
    interfaceType = InterfaceType::Custom;
}

void CVOutputManager::outputVoltage(float volts)
{
    currentOutputVoltage.store(
        vcotuner::conditionOutputVoltage(volts,
                                         { interfaceMinVolts, interfaceMaxVolts },
                                         { interfaceCalibration.isCalibrated,
                                           interfaceCalibration.gain,
                                           interfaceCalibration.offset }));
}

void CVOutputManager::outputPitch(int midiNote)
{
    float voltage = midiToVoltage(midiNote);
    outputVoltage(voltage);
}

void CVOutputManager::outputPitch(float midiPitchFloat)
{
    float voltage = midiToVoltage(midiPitchFloat);
    outputVoltage(voltage);
}

void CVOutputManager::outputFrequency(float hz)
{
    float voltage = frequencyToVoltage(hz);
    outputVoltage(voltage);
}

void CVOutputManager::fillOutputBuffer(float* buffer, int numSamples)
{
    if (!isActiveFlag.load())
    {
        // Clear buffer if not active
        for (int i = 0; i < numSamples; ++i)
            buffer[i] = 0.0f;
        return;
    }

    float voltage = currentOutputVoltage.load();
    float sample = voltageToSample(voltage);

    // Fill buffer with DC value
    for (int i = 0; i < numSamples; ++i)
    {
        buffer[i] = sample;
    }
}

float CVOutputManager::midiToVoltage(int midiNote) const
{
    return midiToVoltage(static_cast<float>(midiNote));
}

float CVOutputManager::midiToVoltage(float midiPitch) const
{
    return vcotuner::midiToVoltage(midiPitch, toCvStandard(currentStandard), hzPerVoltScaling);
}

float CVOutputManager::frequencyToVoltage(float hz) const
{
    return vcotuner::frequencyToVoltage(hz, toCvStandard(currentStandard), hzPerVoltScaling);
}

float CVOutputManager::voltageToMidi(float voltage) const
{
    return vcotuner::voltageToMidi(voltage, toCvStandard(currentStandard), hzPerVoltScaling);
}

float CVOutputManager::voltageToSample(float volts) const
{
    return vcotuner::voltageToSample(volts, { interfaceMinVolts, interfaceMaxVolts });
}

float CVOutputManager::sampleToVoltage(float sample) const
{
    return vcotuner::sampleToVoltage(sample, { interfaceMinVolts, interfaceMaxVolts });
}

float CVOutputManager::applyInterfaceCalibration(float voltage) const
{
    return vcotuner::applyCalibration(voltage,
                                      { interfaceCalibration.isCalibrated,
                                        interfaceCalibration.gain,
                                        interfaceCalibration.offset });
}

void CVOutputManager::setInterfaceCalibration(const InterfaceCalibration& cal)
{
    interfaceCalibration = cal;
}

void CVOutputManager::clearInterfaceCalibration()
{
    interfaceCalibration = InterfaceCalibration();
}

void CVOutputManager::addCalibrationPoint(float idealVoltage, float actualVoltage)
{
    interfaceCalibration.calibrationPoints.push_back({idealVoltage, actualVoltage});
}

void CVOutputManager::computeCalibrationFromPoints()
{
    auto& points = interfaceCalibration.calibrationPoints;

    if (points.size() < 2)
    {
        // Not enough points for calibration
        interfaceCalibration.isCalibrated = false;
        return;
    }

    // Simple linear regression: actual = gain * ideal + offset
    // We want: corrected = gain * ideal + offset such that corrected produces actualVoltage
    // So we need the inverse: ideal_corrected = (actual_desired - offset) / gain

    // Calculate means
    float sumIdeal = 0, sumActual = 0;
    for (const auto& p : points)
    {
        sumIdeal += p.first;
        sumActual += p.second;
    }
    float meanIdeal = sumIdeal / points.size();
    float meanActual = sumActual / points.size();

    // Calculate slope (gain) and intercept (offset)
    float numerator = 0, denominator = 0;
    for (const auto& p : points)
    {
        float diffIdeal = p.first - meanIdeal;
        float diffActual = p.second - meanActual;
        numerator += diffIdeal * diffActual;
        denominator += diffIdeal * diffIdeal;
    }

    if (std::abs(denominator) < 1e-10f)
    {
        // Degenerate case
        interfaceCalibration.gain = 1.0f;
        interfaceCalibration.offset = meanActual - meanIdeal;
    }
    else
    {
        // The interface outputs: actual = interfaceGain * ideal + interfaceOffset
        // We measured: when we output 'ideal', we get 'actual'
        // To correct: we need to output 'corrected' such that interface produces 'ideal'
        // So: ideal = interfaceGain * corrected + interfaceOffset
        //     corrected = (ideal - interfaceOffset) / interfaceGain

        float interfaceGain = numerator / denominator;
        float interfaceOffset = meanActual - interfaceGain * meanIdeal;

        // Store inverse correction
        interfaceCalibration.gain = 1.0f / interfaceGain;
        interfaceCalibration.offset = -interfaceOffset / interfaceGain;
    }

    interfaceCalibration.isCalibrated = true;
    interfaceCalibration.calibrationDate = Time::getCurrentTime();
}

void CVOutputManager::saveCalibration(const File& file) const
{
    var calibData;

    calibData.getDynamicObject()->setProperty("isCalibrated", interfaceCalibration.isCalibrated);
    calibData.getDynamicObject()->setProperty("gain", interfaceCalibration.gain);
    calibData.getDynamicObject()->setProperty("offset", interfaceCalibration.offset);
    calibData.getDynamicObject()->setProperty("interfaceName", interfaceCalibration.interfaceName);
    calibData.getDynamicObject()->setProperty("calibrationDate",
        interfaceCalibration.calibrationDate.toISO8601(true));

    Array<var> pointsArray;
    for (const auto& p : interfaceCalibration.calibrationPoints)
    {
        var point;
        point.getDynamicObject()->setProperty("ideal", p.first);
        point.getDynamicObject()->setProperty("actual", p.second);
        pointsArray.add(point);
    }
    calibData.getDynamicObject()->setProperty("points", pointsArray);

    file.replaceWithText(JSON::toString(calibData, true));
}

bool CVOutputManager::loadCalibration(const File& file)
{
    if (!file.existsAsFile())
        return false;

    var calibData = JSON::parse(file.loadFileAsString());

    if (!calibData.isObject())
        return false;

    interfaceCalibration.isCalibrated = calibData.getProperty("isCalibrated", false);
    interfaceCalibration.gain = calibData.getProperty("gain", 1.0f);
    interfaceCalibration.offset = calibData.getProperty("offset", 0.0f);
    interfaceCalibration.interfaceName = calibData.getProperty("interfaceName", "").toString();

    String dateStr = calibData.getProperty("calibrationDate", "").toString();
    if (dateStr.isNotEmpty())
        interfaceCalibration.calibrationDate = Time::fromISO8601(dateStr);

    interfaceCalibration.calibrationPoints.clear();
    if (calibData.hasProperty("points"))
    {
        auto* pointsArray = calibData.getProperty("points", var()).getArray();
        if (pointsArray != nullptr)
        {
            for (const auto& point : *pointsArray)
            {
                float ideal = point.getProperty("ideal", 0.0f);
                float actual = point.getProperty("actual", 0.0f);
                interfaceCalibration.calibrationPoints.push_back({ideal, actual});
            }
        }
    }

    return true;
}
