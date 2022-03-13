#include "utils.hh"

#include <strutils.hh>

#include <chrono>
#include <regex>
#include <sstream>

#include <cmath>

namespace Utils {

// ============================================================================

int64_t makeTimestamp () {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

// ============================================================================

bool isFloat (const std::string& a_String) {
    std::istringstream iss(a_String);
    float  dummy;

    iss >> std::noskipws >> dummy;
    return iss && iss.eof();
}

float stof (const std::string& a_Str) {

    // Rational number "<num>/<den>"
    size_t p = a_Str.find("/");
    if (p != std::string::npos) {

        std::string num = a_Str.substr(0, p);
        std::string den = a_Str.substr(p+1);

        double val = std::stod(num) / std::stod(den);
        return (float)val;
    }

    // Regular decimal notation
    return std::stof(a_Str);
}

// ============================================================================

int noteToIndex(const std::string& a_Note) {
    // https://www.inspiredacoustics.com/en/MIDI_note_numbers_and_center_frequencies

    static std::regex expr("^(([A-G])(#|b)?)([0-9])$");

    // Match the regex
    std::smatch match;
    std::regex_match(a_Note, match, expr);

    // No match
    if (match.empty()) {
        return -1;
    }

    // Base note
    auto noteStr = match[1].str();
    // Octave
    int  octave  = std::stoi(match[4].str());

    // Convert base note to index
    int note = -1;

    if (noteStr == "C") {
        note = 0;
    }
    else if (noteStr == "C#" || noteStr == "Db") {
        note = 1;
    }
    else if (noteStr == "D") {
        note = 2;
    }
    else if (noteStr == "D#" || noteStr == "Eb") {
        note = 3;
    }
    else if (noteStr == "E") {
        note = 4;
    }
    else if (noteStr == "F") {
        note = 5;
    }
    else if (noteStr == "F#" || noteStr == "Gb") {
        note = 6;
    }
    else if (noteStr == "G") {
        note = 7;
    }
    else if (noteStr == "G#" || noteStr == "Ab") {
        note = 8;
    }
    else if (noteStr == "A") {
        note = 9;
    }
    else if (noteStr == "A#" || noteStr == "Bb") {
        note = 10;
    }
    else if (noteStr == "B") {
        note = 11;
    }

    // Invalid
    else {
        return -1;
    }

    // Shift by octave
    return note + 12 + octave * 12;
}

// ============================================================================

float noteToCv (int a_Note) {
    // https://en.wikipedia.org/wiki/CV/gate
    // 12 notes per octave.
    return (float)(a_Note - 21) / 12.0f;
}

float noteToCv (const std::string& a_Note) {
    return noteToCv(noteToIndex(a_Note));
}

float noteToFrequency (int a_Note) {
    return 27.50f * powf(2.0f, noteToCv(a_Note));
}

float noteToFrequency(const std::string& a_Note) {
    return noteToFrequency(noteToIndex(a_Note));
}

// ============================================================================

float frequencyToCv (float a_Frequency) {
    return log2f(a_Frequency / 27.5f);
}

float cvToFrequency (float a_Cv) {
    return 27.50f * powf(2.0f, a_Cv);
}

// ============================================================================

float parseCvSpec (const std::string& a_String) {

    // A frequency in Hz
    if (strutils::endswith(a_String, "Hz")) {
        float f = std::stof(strutils::replace(a_String, "Hz", ""));
        return frequencyToCv(f);
    }

    // A frequency in kHz
    if (strutils::endswith(a_String, "kHz")) {
        float f = std::stof(strutils::replace(a_String, "kHz", ""));
        return frequencyToCv(f * 1e3f);
    }

    // A note in english notation
    int note = noteToIndex(a_String);
    if (note != -1) {
        return noteToCv(note);
    }

    // Direct CV value
    return Utils::stof(a_String);
}

// ============================================================================
}; // Utils

