/*
  ==============================================================================

    JSONExporter.h
    Export calibration data to JSON format

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Calibration/CalibrationTable.h"

class JSONExporter
{
public:
    static bool exportCalibration(const CalibrationTable& table,
                                  const File& outputFile);

    static String generateJSONString(const CalibrationTable& table);
};
