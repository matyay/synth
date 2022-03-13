#include "instrument.hh"
#include "exception.hh"

#include <graph/dot_writer.hh>

#include <utils/utils.hh>
#include <utils/exception.hh>
#include <stringf.hh>

#include <strutils.hh>

#include <spdlog/sinks/stdout_color_sinks.h>

#include <algorithm>

namespace Instrument {

// ============================================================================

Instrument::Instrument (const std::string& a_Name,
                        const std::string& a_Module,
                        Graph::Builder* a_Builder,
                        size_t a_SampleRate,
                        size_t a_BufferSize,
                        const Attributes& a_Attributes) :
    m_Name (a_Name)
{
    // Create the logger
    std::string loggerName = stringf("instrument [%s]", a_Name.c_str());
    m_Logger = spdlog::get(loggerName);
    if (!m_Logger) m_Logger = spdlog::stderr_color_mt(loggerName);

    // Decode attributes
    size_t maxVoices = std::stoi(a_Attributes.get("maxVoices",   "1"));
    m_MidiChannel    = std::stoi(a_Attributes.get("midiChannel", "0"));

    if (a_Attributes.has("minNote")) {
        auto str  = a_Attributes.get("minNote");
        int  note = Utils::noteToIndex(str);
        if (note < 0) {
            THROW(BuildError, "Invalid node specification '%s'",
                str.c_str()
            );
        }

        m_MinNote = (size_t)note;
    }

    if (a_Attributes.has("maxNote")) {
        auto str  = a_Attributes.get("maxNote");
        int  note = Utils::noteToIndex(str);
        if (note < 0) {
            THROW(BuildError, "Invalid node specification '%s'",
                str.c_str()
            );
        }

        m_MaxNote = (size_t)note;
    }

    m_MinLevel       = std::stof(a_Attributes.get("minLevel",      "-96.0"));
    m_MinSilentTime  = std::stof(a_Attributes.get("minSilentTime", "0.1"));
    m_MaxPlayTime    = std::stof(a_Attributes.get("maxPlayTime",   "60.0"));

    /// Build a top-level module for each voice
    for (size_t i=0; i<maxVoices; ++i) {
        const std::string name = stringf("%s#%d", m_Name.c_str(), i);

        // Build the module        
        auto module = a_Builder->build(a_Module, name);
        module->prepare(a_SampleRate, a_BufferSize);

        // DEBUG - dump attributes
        m_Logger->debug("Attributes:");
        for (auto it : module->getAttributes()) {
            m_Logger->debug(" '{}' = '{}'", it.first, it.second);
        }

        // DEBUG - dump parameters
        m_Logger->debug("Parameters:");
        for (auto it : module->getParameters()) {
            m_Logger->debug(" '{}'", it.first);
        }

        // Create a voice
        std::shared_ptr<Voice> voice (new Voice(module, m_MinLevel));
        m_Voices.push_back(voice);
    }

    // Get parameters file name
    m_ParametersFile = a_Attributes.get("paramsFile",
        stringf("%s_params.txt", a_Name.c_str()));
}

// ============================================================================

std::string Instrument::getName () const {
    return m_Name;
}

void Instrument::dumpGraphAsDot (const std::string& a_FileName) {

    auto module = m_Voices.at(0)->getModule();

    // Dump the graph
    Graph::DotWriter writer(module);
    writer.writeDot(a_FileName);
}

Voice* Instrument::getFreeVoice () {

    // Find a free voice
    for (auto& candidate : m_Voices) {
        if (!candidate->isActive()) {
            return candidate.get();
        }
    }

    // No free voice, get the one that has been playing longest
    // TODO:

    return nullptr;
}

// ============================================================================

void Instrument::processEvents (const std::vector<MIDI::Event>& a_Events,
                                std::vector<Voice*>& a_ActiveVoices)
{
    // Process MIDI events, activate new voices
    for (auto& event : a_Events) {

        // Note on
        if (event.type == MIDI::Event::Type::NOTE_ON) {

            // Filter channel
            if (m_MidiChannel != 0) {
                if (event.data.note.channel != (m_MidiChannel - 1))
                    continue;
            }

            // Filter note
            uint8_t note = event.data.note.note;
            if (note < m_MinNote || note > m_MaxNote) {
                continue;
            }

            Voice* voice = nullptr;

            // Check if we already have an active voice for that note
            if (m_ActiveVoices.count(note) != 0) {
                voice = m_ActiveVoices[note];
            }
            // We don't. Get a new voice
            else {

                // Find
                voice = getFreeVoice();

                // No free voices
                if (voice == nullptr) {
                    m_Logger->warn("No free voice for note {}", note);
                    continue;
                }

                // Activate the new voice
                m_Logger->debug("New voice for note {}", note);
                voice->activate();

                // Store in the active voice map
                m_ActiveVoices[note] = voice;
            }

            // Dispatch the event
            voice->pushEvent(event);
        }

        // Note off
        else if (event.type == MIDI::Event::Type::NOTE_OFF) {

            // Filter channel
            if (m_MidiChannel != 0) {
                if (event.data.note.channel != (m_MidiChannel - 1))
                    continue;
            }

            // Filter note
            uint8_t note = event.data.note.note;
            if (note < m_MinNote || note > m_MaxNote) {
                continue;
            }

            // We do not have that note active
            if (m_ActiveVoices.count(note) == 0) {
                m_Logger->warn("Note {} was not playing", note);
                continue;
            }

            // Dispatch the event
            Voice* voice = m_ActiveVoices[note];
            voice->pushEvent(event);
        }

        // Controller
        else if (event.type == MIDI::Event::Type::CONTROLLER) {

            // Filter channel
            if (m_MidiChannel != 0) {
                if (event.data.ctrl.channel != (m_MidiChannel - 1))
                    continue;
            }

            // All sounds off
            if (event.data.ctrl.param == 120 || event.data.ctrl.param == 123) {

                // Deactivate all immediately
                for (auto itr : m_ActiveVoices) {
                    Voice* voice = itr.second;
                    voice->deactivate();
                }

                m_Logger->debug("All voices off");
                m_ActiveVoices.clear();
            }
            // Dispatch to all voices
            else {

                for (auto voice : m_Voices) {
                    voice->pushEvent(event);
                }
            }
        }
    }

    // Deactivate voices
    int64_t minTime = (int64_t)(m_MinSilentTime * 1e3f);
    int64_t maxTime = (int64_t)(m_MaxPlayTime   * 1e3f);

    for (auto itr = m_ActiveVoices.begin(); itr != m_ActiveVoices.end(); ) {
        auto note  = itr->first;
        auto voice = itr->second;

        // Deactivate the voice if silent for too long
        if (voice->getSilentTime() > minTime || voice->getActiveTime() > maxTime) {
            m_Logger->debug("Deactivating note {}", note);

            voice->deactivate();

            itr = m_ActiveVoices.erase(itr);
            continue;
        }

        // Next
        itr++;
    }

    // Fill in the vector of active voices
    for (auto& it : m_ActiveVoices) {
        a_ActiveVoices.push_back(it.second);
    }
}

// ============================================================================

const Graph::Module::Parameters Instrument::getParameters () {

    // All voices have identical pipelines, their parameters are also identical
    Voice* voice = m_Voices[0].get();
    return voice->getModule()->getParameters();
}

void Instrument::updateParameters (const Graph::Module::ParameterValues& a_Values) {

    // Update parameters in all modules
    for (auto& voice : m_Voices) {
        voice->getModule()->updateParameters(a_Values);
    }
}

// ============================================================================

void Instrument::saveParameters (const std::string& a_FileName, bool a_Append) {

    std::string fileName = (!a_FileName.empty()) ? a_FileName : m_ParametersFile;
    m_Logger->info("Saving '{}' parameters to '{}'", getName(), fileName);

    // Open the file
    FILE* fp = fopen(fileName.c_str(), (a_Append) ? "a+" : "w");
    if (fp == nullptr) {
        THROW(std::runtime_error, "Error writing file '%s'", fileName.c_str());
    }

    // Get parameters
    const auto params = getParameters();

    // Sorted parameter name list
    std::vector<std::string> paramNames;
    for (auto& it : params)
        paramNames.push_back(it.first);
    std::sort(paramNames.begin(), paramNames.end());

    // Write parameters
    for (auto& paramName : paramNames) {
        auto& param = params.get(paramName);

        // Skip locked parameters
        if (param.isLocked()) {
            continue;
        }

        // FIXME: Remove round brackets from choice parameter strings
        std::string value = param.get().asString();
        size_t p = value.rfind("(");
        if (p != std::string::npos) {
            value = value.substr(0, p);
        }

        // name=value
        fprintf(fp, "%s.%s=%s\n",
            getName().c_str(),
            paramName.c_str(),
            value.c_str()
        );
    }

    // Close the file
    fclose(fp);
}

void Instrument::loadParameters (const std::string& a_FileName) {

    std::string fileName = (!a_FileName.empty()) ? a_FileName : m_ParametersFile;
    m_Logger->info("Loading '{}' parameters from '{}'", getName(), fileName);

    // Open the file
    FILE* fp = fopen(fileName.c_str(), "r");
    if (fp == nullptr) {
        THROW(std::runtime_error, "Error reading file '%s'", fileName.c_str());
    }

    // Process lines
    char*   lineBuf = nullptr;
    size_t  lineSize = 0;
    ssize_t lineLen = 0;

    Dict<std::string, Graph::Parameter::Value> params;

    while (1) {

        // Read one line
        lineLen = getline(&lineBuf, &lineSize, fp);
        if (lineLen < 0) {
            break;
        }

        // Strip the line, ignore empty
        std::string line = strutils::strip(lineBuf);
        if (line.empty()) {
            continue;
        }

        // Split parameter and value
        auto fields = strutils::split(line, "=", 1);
        if (fields.size() != 2) {
            m_Logger->error("Malformed line '{}'", line);
            continue;
        }

        auto param = fields[0];
        auto value = fields[1];

        // Get instrument name
        fields = strutils::split(param, ".", 1);
        if (fields.size() != 2) {
            m_Logger->error("Invalid parameter specification '{}'", param);
            continue;
        }

        auto instr = fields[0];
        auto path  = fields[1];

        // This is not for this instrument
        if (instr != getName()) {
            m_Logger->debug("skipping '{}'", line);
            continue;
        }

        // Append the parameter
        if (Utils::isFloat(value)) {
            params.set(path, (float)atof(value.c_str()));
        } else {
            params.set(path, value);
        }
    }

    // Update the parameters
    updateParameters(params);
}


// ============================================================================

}; // Instrument
