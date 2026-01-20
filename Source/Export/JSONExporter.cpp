/*
  ==============================================================================

    JSONExporter.cpp
    Export calibration data to JSON format

  ==============================================================================
*/

#include "JSONExporter.h"

bool JSONExporter::exportCalibration(const CalibrationTable& table,
                                     const File& outputFile)
{
    String json = generateJSONString(table);
    return outputFile.replaceWithText(json);
}

String JSONExporter::generateJSONString(const CalibrationTable& table)
{
    var data(new DynamicObject());

    data.getDynamicObject()->setProperty("format_version", "1.0");
    data.getDynamicObject()->setProperty("generator", "VCOTuner");
    data.getDynamicObject()->setProperty("generated_at",
        table.getCalibrationDate().toISO8601(true));

    // Device info
    var device(new DynamicObject());
    device.getDynamicObject()->setProperty("brand", table.getDeviceBrand());
    device.getDynamicObject()->setProperty("model", table.getDeviceName());
    device.getDynamicObject()->setProperty("notes", table.getNotes());
    data.getDynamicObject()->setProperty("device_under_test", device);

    // Interface info
    var cvInterface(new DynamicObject());
    cvInterface.getDynamicObject()->setProperty("name", table.getInterfaceName());
    data.getDynamicObject()->setProperty("cv_interface", cvInterface);

    // Settings
    var settings(new DynamicObject());
    settings.getDynamicObject()->setProperty("voltage_standard", table.getVoltageStandard());
    settings.getDynamicObject()->setProperty("reference_note", 60);
    settings.getDynamicObject()->setProperty("reference_frequency_hz", 261.63);
    data.getDynamicObject()->setProperty("calibration_settings", settings);

    // Calibration points
    Array<var> pointsArray;
    for (int i = 0; i < table.getEntryCount(); ++i)
    {
        const auto& entry = table.getEntry(i);
        var point(new DynamicObject());
        point.getDynamicObject()->setProperty("midi_note", entry.midiNote);
        point.getDynamicObject()->setProperty("ideal_voltage", entry.idealVoltage);
        point.getDynamicObject()->setProperty("corrected_voltage", entry.actualVoltage);
        point.getDynamicObject()->setProperty("correction_offset", entry.correctionOffset);
        point.getDynamicObject()->setProperty("measured_frequency_hz", entry.measuredFrequency);
        point.getDynamicObject()->setProperty("error_cents", entry.errorCents);
        point.getDynamicObject()->setProperty("std_dev_cents", entry.stdDevCents);
        pointsArray.add(point);
    }
    data.getDynamicObject()->setProperty("calibration_points", pointsArray);

    // Statistics
    var stats(new DynamicObject());
    stats.getDynamicObject()->setProperty("total_points", table.getEntryCount());
    stats.getDynamicObject()->setProperty("max_error_cents", table.getMaxErrorCents());
    stats.getDynamicObject()->setProperty("min_error_cents", table.getMinErrorCents());
    stats.getDynamicObject()->setProperty("average_error_cents", table.getAverageErrorCents());
    stats.getDynamicObject()->setProperty("rms_error_cents", table.getRMSErrorCents());
    auto worst = table.getWorstNote();
    stats.getDynamicObject()->setProperty("worst_note", worst.first);
    stats.getDynamicObject()->setProperty("worst_error_cents", worst.second);
    data.getDynamicObject()->setProperty("statistics", stats);

    // Polynomial fit
    auto coefficients = table.getPolynomialCoefficients(4);
    if (!coefficients.empty())
    {
        var poly(new DynamicObject());
        poly.getDynamicObject()->setProperty("degree", 4);
        Array<var> coeffArray;
        for (double c : coefficients)
            coeffArray.add(c);
        poly.getDynamicObject()->setProperty("coefficients", coeffArray);
        data.getDynamicObject()->setProperty("polynomial_fit", poly);
    }

    return JSON::toString(data, true);
}
