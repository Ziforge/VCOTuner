/*
  ==============================================================================

    CSVExporter.cpp
    Export calibration data to CSV format

  ==============================================================================
*/

#include "CSVExporter.h"

bool CSVExporter::exportCalibration(const CalibrationTable& table,
                                    const File& outputFile,
                                    bool includeHeader)
{
    String csv = generateCSVString(table, includeHeader);
    return outputFile.replaceWithText(csv);
}

String CSVExporter::generateCSVString(const CalibrationTable& table, bool includeHeader)
{
    String csv;

    // Metadata comments
    csv += "# VCOTuner Calibration Export\n";
    csv += "# Device: " + table.getDeviceName() + " (" + table.getDeviceBrand() + ")\n";
    csv += "# Interface: " + table.getInterfaceName() + "\n";
    csv += "# Standard: " + table.getVoltageStandard() + "\n";
    csv += "# Date: " + table.getCalibrationDate().toString(true, true) + "\n";

    if (table.getNotes().isNotEmpty())
        csv += "# Notes: " + table.getNotes() + "\n";

    csv += "#\n";

    // Statistics
    csv += "# Statistics:\n";
    csv += "#   Max Error: " + String(table.getMaxErrorCents(), 2) + " cents\n";
    csv += "#   Min Error: " + String(table.getMinErrorCents(), 2) + " cents\n";
    csv += "#   Avg Error: " + String(table.getAverageErrorCents(), 2) + " cents\n";
    csv += "#   RMS Error: " + String(table.getRMSErrorCents(), 2) + " cents\n";
    auto worst = table.getWorstNote();
    csv += "#   Worst Note: MIDI " + String(worst.first) + " (" + String(worst.second, 2) + " cents)\n";
    csv += "#\n";

    // Header
    if (includeHeader)
    {
        csv += "MIDINote,IdealVoltage,ActualVoltage,CorrectionOffset,MeasuredFrequency,ErrorCents,StdDevCents\n";
    }

    // Data
    for (int i = 0; i < table.getEntryCount(); ++i)
    {
        const auto& entry = table.getEntry(i);
        csv += String(entry.midiNote) + ",";
        csv += String(entry.idealVoltage, 4) + ",";
        csv += String(entry.actualVoltage, 4) + ",";
        csv += String(entry.correctionOffset, 4) + ",";
        csv += String(entry.measuredFrequency, 2) + ",";
        csv += String(entry.errorCents, 2) + ",";
        csv += String(entry.stdDevCents, 2) + "\n";
    }

    return csv;
}
