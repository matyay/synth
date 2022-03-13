#ifndef INSTRUMENT_HH
#define INSTRUMENT_HH

#include "voice.hh"

#include <graph/parameter.hh>
#include <graph/builder.hh>

#include <utils/dict.hh>

#include <spdlog/spdlog.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace Instrument {

// ============================================================================

class Instrument
{
public:

    /// Instrument attributes
    typedef Dict<std::string, std::string> Attributes;

    /// Constructor
    Instrument (const std::string& a_Name,
                const std::string& a_Module,
                Graph::Builder* a_Builder,
                size_t a_SampleRate,
                size_t a_BufferSize,
                const Attributes& a_Attributes = Attributes());

    /// Returns the instrument name
    std::string getName () const;

    /// Dumps graph structure of the first voice to a Graphviz DOT file
    void dumpGraphAsDot (const std::string& a_FileName);

    /// Processed MIDI events. Fills the given vector with active voices
    void processEvents (const std::vector<MIDI::Event>& a_Events,
                        std::vector<Voice*>& a_ActiveVoices);

    /// Returns parameters
    const Graph::Module::Parameters getParameters ();
    /// Updates parameters
    void updateParameters (const Graph::Module::ParameterValues& a_Values);

    /// Saves all mutable instrument parameters to a file
    void saveParameters (const std::string& a_FileName = std::string(), bool a_Append = false);
    /// Loads instrument parameters from a file
    void loadParameters (const std::string& a_FileName = std::string());

protected:

    /// Returns a free voice or nullptr if none
    Voice* getFreeVoice ();

    /// Logger
    std::shared_ptr<spdlog::logger> m_Logger;

    /// Name
    const std::string m_Name;
    /// MIDI channel. When 0 means all channels
    size_t m_MidiChannel;
    /// Minimum MIDI note index
    size_t m_MinNote = 0;
    /// Maximum MIDI note index
    size_t m_MaxNote = 127;

    /// Minimal voice level [dB]
    float m_MinLevel = -96.0f;
    /// Minimal silent time [s]
    float m_MinSilentTime = 0.1f;
    /// Max voice play time [s]
    float m_MaxPlayTime = 10.0f;

    /// Voice list
    std::vector<std::shared_ptr<Voice>> m_Voices;
    /// MIDI note to active voice map
    std::unordered_map<uint8_t, Voice*> m_ActiveVoices;

    /// Parameter storage file name
    std::string m_ParametersFile;
};

// ============================================================================

}; // Instrument

#endif // INSTRUMENT_HH
