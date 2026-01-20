/*
  ==============================================================================

    CSVExporter.h
    Export calibration data to CSV format

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Calibration/CalibrationTable.h"

class CSVExporter
{
public:
    static bool exportCalibration(const CalibrationTable& table,
                                  const File& outputFile,
                                  bool includeHeader = true);

    static String generateCSVString(const CalibrationTable& table,
                                    bool includeHeader = true);
};
