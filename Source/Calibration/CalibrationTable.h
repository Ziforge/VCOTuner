/*
  ==============================================================================

    CalibrationTable.h
    Storage and interpolation for VCO calibration data

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <optional>

class CalibrationTable
{
public:
    // Single calibration point
    struct Entry
    {
        int midiNote = 0;
        float idealVoltage = 0.0f;      // What voltage "should" produce this note
        float actualVoltage = 0.0f;     // What voltage actually produces this note
        float correctionOffset = 0.0f;  // actualVoltage - idealVoltage
        float measuredFrequency = 0.0f;
        float errorCents = 0.0f;        // Error in cents
        float stdDevCents = 0.0f;       // Measurement stability
    };

    CalibrationTable();
    ~CalibrationTable();

    // Building the table
    void addEntry(const Entry& entry);
    void clear();
    void sortByMidiNote();

    // Access
    int getEntryCount() const { return static_cast<int>(entries.size()); }
    const Entry& getEntry(int index) const { return entries[index]; }
    Entry* findEntryForNote(int midiNote);
    const Entry* findEntryForNote(int midiNote) const;
    const std::vector<Entry>& getAllEntries() const { return entries; }

    // Interpolation for arbitrary pitches
    float getCorrectedVoltage(float targetMidiPitch) const;
    float getCorrectionOffset(float targetMidiPitch) const;

    // Statistics
    float getMaxErrorCents() const;
    float getMinErrorCents() const;
    float getAverageErrorCents() const;
    float getRMSErrorCents() const;
    std::pair<int, float> getWorstNote() const;  // Note with largest |error|

    // Polynomial fit for smooth correction curves
    std::vector<double> getPolynomialCoefficients(int degree = 4) const;
    float evaluatePolynomial(const std::vector<double>& coefficients, float pitch) const;

    // Metadata
    void setDeviceName(const String& name) { deviceName = name; }
    void setDeviceBrand(const String& brand) { deviceBrand = brand; }
    void setInterfaceName(const String& name) { interfaceName = name; }
    void setNotes(const String& n) { notes = n; }
    void setCalibrationDate(Time date) { calibrationDate = date; }
    void setVoltageStandard(const String& standard) { voltageStandard = standard; }

    String getDeviceName() const { return deviceName; }
    String getDeviceBrand() const { return deviceBrand; }
    String getInterfaceName() const { return interfaceName; }
    String getNotes() const { return notes; }
    Time getCalibrationDate() const { return calibrationDate; }
    String getVoltageStandard() const { return voltageStandard; }

    // Serialization
    void saveToFile(const File& file) const;
    bool loadFromFile(const File& file);

private:
    std::vector<Entry> entries;

    // Metadata
    String deviceName;
    String deviceBrand;
    String interfaceName;
    String notes;
    Time calibrationDate;
    String voltageStandard = "1V/Oct";

    // Interpolation helpers
    float linearInterpolate(float pitch) const;

};
