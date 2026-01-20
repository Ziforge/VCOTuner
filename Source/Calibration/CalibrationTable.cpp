/*
  ==============================================================================

    CalibrationTable.cpp
    Storage and interpolation for VCO calibration data

  ==============================================================================
*/

#include "CalibrationTable.h"
#include <algorithm>
#include <cmath>

CalibrationTable::CalibrationTable()
{
    calibrationDate = Time::getCurrentTime();
}

CalibrationTable::~CalibrationTable()
{
}

void CalibrationTable::addEntry(const Entry& entry)
{
    entries.push_back(entry);
}

void CalibrationTable::clear()
{
    entries.clear();
}

void CalibrationTable::sortByMidiNote()
{
    std::sort(entries.begin(), entries.end(),
        [](const Entry& a, const Entry& b) { return a.midiNote < b.midiNote; });
}

CalibrationTable::Entry* CalibrationTable::findEntryForNote(int midiNote)
{
    for (auto& entry : entries)
    {
        if (entry.midiNote == midiNote)
            return &entry;
    }
    return nullptr;
}

const CalibrationTable::Entry* CalibrationTable::findEntryForNote(int midiNote) const
{
    for (const auto& entry : entries)
    {
        if (entry.midiNote == midiNote)
            return &entry;
    }
    return nullptr;
}

float CalibrationTable::getCorrectedVoltage(float targetMidiPitch) const
{
    if (entries.empty())
        return (targetMidiPitch - 60.0f) / 12.0f;  // Default 1V/Oct

    // For 1V/Oct: ideal voltage = (pitch - 60) / 12
    float idealVoltage = (targetMidiPitch - 60.0f) / 12.0f;
    float correction = getCorrectionOffset(targetMidiPitch);

    return idealVoltage + correction;
}

float CalibrationTable::getCorrectionOffset(float targetMidiPitch) const
{
    return linearInterpolate(targetMidiPitch);
}

float CalibrationTable::linearInterpolate(float pitch) const
{
    if (entries.empty())
        return 0.0f;

    if (entries.size() == 1)
        return entries[0].correctionOffset;

    // Find surrounding entries
    const Entry* lower = nullptr;
    const Entry* upper = nullptr;

    for (const auto& entry : entries)
    {
        if (entry.midiNote <= pitch)
        {
            if (lower == nullptr || entry.midiNote > lower->midiNote)
                lower = &entry;
        }
        if (entry.midiNote >= pitch)
        {
            if (upper == nullptr || entry.midiNote < upper->midiNote)
                upper = &entry;
        }
    }

    // Handle edge cases
    if (lower == nullptr)
        return upper->correctionOffset;
    if (upper == nullptr)
        return lower->correctionOffset;
    if (lower == upper)
        return lower->correctionOffset;

    // Linear interpolation
    float t = (pitch - lower->midiNote) / (upper->midiNote - lower->midiNote);
    return lower->correctionOffset + t * (upper->correctionOffset - lower->correctionOffset);
}

float CalibrationTable::getMaxErrorCents() const
{
    if (entries.empty())
        return 0.0f;

    float maxErr = entries[0].errorCents;
    for (const auto& entry : entries)
        maxErr = std::max(maxErr, entry.errorCents);
    return maxErr;
}

float CalibrationTable::getMinErrorCents() const
{
    if (entries.empty())
        return 0.0f;

    float minErr = entries[0].errorCents;
    for (const auto& entry : entries)
        minErr = std::min(minErr, entry.errorCents);
    return minErr;
}

float CalibrationTable::getAverageErrorCents() const
{
    if (entries.empty())
        return 0.0f;

    float sum = 0.0f;
    for (const auto& entry : entries)
        sum += std::abs(entry.errorCents);
    return sum / entries.size();
}

float CalibrationTable::getRMSErrorCents() const
{
    if (entries.empty())
        return 0.0f;

    float sumSquares = 0.0f;
    for (const auto& entry : entries)
        sumSquares += entry.errorCents * entry.errorCents;
    return std::sqrt(sumSquares / entries.size());
}

std::pair<int, float> CalibrationTable::getWorstNote() const
{
    if (entries.empty())
        return {60, 0.0f};

    int worstNote = entries[0].midiNote;
    float worstError = std::abs(entries[0].errorCents);

    for (const auto& entry : entries)
    {
        float absError = std::abs(entry.errorCents);
        if (absError > worstError)
        {
            worstError = absError;
            worstNote = entry.midiNote;
        }
    }

    return {worstNote, worstError};
}

std::vector<double> CalibrationTable::getPolynomialCoefficients(int degree) const
{
    // Simple least-squares polynomial fit
    // For production, consider using a proper math library

    int n = static_cast<int>(entries.size());
    if (n < degree + 1)
        return {};

    // Build normal equations matrix
    // We're fitting: correction = a0 + a1*x + a2*x^2 + ... where x = midiNote

    int m = degree + 1;
    std::vector<std::vector<double>> A(m, std::vector<double>(m, 0.0));
    std::vector<double> b(m, 0.0);

    for (const auto& entry : entries)
    {
        double x = entry.midiNote;
        double y = entry.correctionOffset;

        for (int i = 0; i < m; ++i)
        {
            for (int j = 0; j < m; ++j)
            {
                A[i][j] += std::pow(x, i + j);
            }
            b[i] += y * std::pow(x, i);
        }
    }

    // Solve using Gaussian elimination with partial pivoting
    std::vector<double> coefficients(m, 0.0);

    // Forward elimination
    for (int k = 0; k < m; ++k)
    {
        // Find pivot
        int maxRow = k;
        for (int i = k + 1; i < m; ++i)
        {
            if (std::abs(A[i][k]) > std::abs(A[maxRow][k]))
                maxRow = i;
        }
        std::swap(A[k], A[maxRow]);
        std::swap(b[k], b[maxRow]);

        if (std::abs(A[k][k]) < 1e-10)
            return {};  // Singular matrix

        // Eliminate column
        for (int i = k + 1; i < m; ++i)
        {
            double factor = A[i][k] / A[k][k];
            for (int j = k; j < m; ++j)
                A[i][j] -= factor * A[k][j];
            b[i] -= factor * b[k];
        }
    }

    // Back substitution
    for (int i = m - 1; i >= 0; --i)
    {
        coefficients[i] = b[i];
        for (int j = i + 1; j < m; ++j)
            coefficients[i] -= A[i][j] * coefficients[j];
        coefficients[i] /= A[i][i];
    }

    return coefficients;
}

float CalibrationTable::evaluatePolynomial(const std::vector<double>& coefficients, float pitch) const
{
    double result = 0.0;
    double x = 1.0;

    for (double coef : coefficients)
    {
        result += coef * x;
        x *= pitch;
    }

    return static_cast<float>(result);
}

void CalibrationTable::saveToFile(const File& file) const
{
    var data(new DynamicObject());

    data.getDynamicObject()->setProperty("version", "1.0");
    data.getDynamicObject()->setProperty("deviceName", deviceName);
    data.getDynamicObject()->setProperty("deviceBrand", deviceBrand);
    data.getDynamicObject()->setProperty("interfaceName", interfaceName);
    data.getDynamicObject()->setProperty("notes", notes);
    data.getDynamicObject()->setProperty("calibrationDate", calibrationDate.toISO8601(true));
    data.getDynamicObject()->setProperty("voltageStandard", voltageStandard);

    Array<var> entriesArray;
    for (const auto& entry : entries)
    {
        var e(new DynamicObject());
        e.getDynamicObject()->setProperty("midiNote", entry.midiNote);
        e.getDynamicObject()->setProperty("idealVoltage", entry.idealVoltage);
        e.getDynamicObject()->setProperty("actualVoltage", entry.actualVoltage);
        e.getDynamicObject()->setProperty("correctionOffset", entry.correctionOffset);
        e.getDynamicObject()->setProperty("measuredFrequency", entry.measuredFrequency);
        e.getDynamicObject()->setProperty("errorCents", entry.errorCents);
        e.getDynamicObject()->setProperty("stdDevCents", entry.stdDevCents);
        entriesArray.add(e);
    }
    data.getDynamicObject()->setProperty("entries", entriesArray);

    // Statistics
    var stats(new DynamicObject());
    stats.getDynamicObject()->setProperty("maxErrorCents", getMaxErrorCents());
    stats.getDynamicObject()->setProperty("minErrorCents", getMinErrorCents());
    stats.getDynamicObject()->setProperty("avgErrorCents", getAverageErrorCents());
    stats.getDynamicObject()->setProperty("rmsErrorCents", getRMSErrorCents());
    auto worst = getWorstNote();
    stats.getDynamicObject()->setProperty("worstNote", worst.first);
    stats.getDynamicObject()->setProperty("worstError", worst.second);
    data.getDynamicObject()->setProperty("statistics", stats);

    file.replaceWithText(JSON::toString(data, true));
}

bool CalibrationTable::loadFromFile(const File& file)
{
    if (!file.existsAsFile())
        return false;

    var data = JSON::parse(file.loadFileAsString());
    if (!data.isObject())
        return false;

    deviceName = data.getProperty("deviceName", "").toString();
    deviceBrand = data.getProperty("deviceBrand", "").toString();
    interfaceName = data.getProperty("interfaceName", "").toString();
    notes = data.getProperty("notes", "").toString();
    voltageStandard = data.getProperty("voltageStandard", "1V/Oct").toString();

    String dateStr = data.getProperty("calibrationDate", "").toString();
    if (dateStr.isNotEmpty())
        calibrationDate = Time::fromISO8601(dateStr);

    entries.clear();
    if (data.hasProperty("entries"))
    {
        auto* arr = data.getProperty("entries", var()).getArray();
        if (arr != nullptr)
        {
            for (const auto& e : *arr)
            {
                Entry entry;
                entry.midiNote = e.getProperty("midiNote", 0);
                entry.idealVoltage = e.getProperty("idealVoltage", 0.0f);
                entry.actualVoltage = e.getProperty("actualVoltage", 0.0f);
                entry.correctionOffset = e.getProperty("correctionOffset", 0.0f);
                entry.measuredFrequency = e.getProperty("measuredFrequency", 0.0f);
                entry.errorCents = e.getProperty("errorCents", 0.0f);
                entry.stdDevCents = e.getProperty("stdDevCents", 0.0f);
                entries.push_back(entry);
            }
        }
    }

    return true;
}
