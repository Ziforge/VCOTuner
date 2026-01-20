/*
  ==============================================================================

    OrnamentCrimeExporter.h
    Export calibration data in Ornament & Crime compatible formats

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Calibration/CalibrationTable.h"
#include <array>

class OrnamentCrimeExporter
{
public:
    // o_C DAC specifications
    static constexpr int OC_DAC_BITS = 16;
    static constexpr float OC_MIN_VOLTAGE = -3.0f;
    static constexpr float OC_MAX_VOLTAGE = +6.0f;
    static constexpr int OC_OCTAVE_COUNT = 10;  // -3V to +6V = 9 octaves, 10 boundary points

    // o_C calibration data structure
    struct OCCalibrationData
    {
        std::array<uint16_t, 11> dacValues;  // 11 points for 10 octaves
        int channel = 0;
    };

    // Export as C header for compiling into firmware
    static bool exportAsCHeader(const CalibrationTable& table,
                                const File& outputFile,
                                const String& arrayName = "custom_cal",
                                int channel = 0);

    // Export human-readable format for manual entry via o_C calibration menu
    static bool exportAsReadable(const CalibrationTable& table,
                                 const File& outputFile);

    // Generate just the string content (for preview)
    static String generateCHeaderString(const CalibrationTable& table,
                                        const String& arrayName = "custom_cal",
                                        int channel = 0);

    static String generateReadableString(const CalibrationTable& table);

    // Convert calibration table to o_C DAC values
    static OCCalibrationData convertToOCFormat(const CalibrationTable& table);

    // Import o_C calibration (if we can parse it)
    static CalibrationTable importFromOCData(const OCCalibrationData& ocData);

    // Utility functions
    static uint16_t voltageToDACValue(float voltage);
    static float dacValueToVoltage(uint16_t dacValue);
    static int voltageToOctaveIndex(float voltage);  // Which of the 11 calibration points
};
