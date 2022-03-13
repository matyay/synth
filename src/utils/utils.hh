#ifndef UTILS_UTILS_HH
#define UTILS_UTILS_HH

#include <string>

#include <cstddef>
#include <cstdint>

namespace Utils {

// ============================================================================

/// Returns a timestamp in milliseconds
int64_t makeTimestamp ();

/// Return true when a string represents a floating point number
bool isFloat (const std::string& a_String);

/// Converts a string to float. Support rational notation "<num>/<den>"
float stof (const std::string& a_Str);

// ============================================================================

/// Returns a MIDI note index for the given note provided as string in english
/// notation.
int noteToIndex (const std::string& a_Note);

/// Converts a note index to control voltage assuming 1V/octave, 0V at A0.
float noteToCv (int a_Note);

/// Converts a note in english notation to control voltage assuming 1V/octave
/// and 0V at A0.
float noteToCv (const std::string& a_Note);

/// Converts a note intex to its base frequency.in Hz
float noteToFrequency (int a_Note);

/// Converts a note in english notation to its base frequency in Hz
float noteToFrequency(const std::string& a_Note);


/// Converts frequency in Hz to CV
float frequencyToCv (float a_Frequency);

/// Converts control voltage to frequency in Hz assuming 1V/octave, 0V at A0.
float cvToFrequency (float a_Cv);


/// Converts a CV specification to its value. This can be one of eg.: "1.0",
/// "B#0" or "440Hz"
float parseCvSpec (const std::string& a_String);

// ============================================================================
}; // Utils
#endif // UTILS_UTILS_HH

