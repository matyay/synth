#include "voice.hh"
#include "exception.hh"

#include <utils/exception.hh>
#include <stringf.hh>

#include <cmath>

namespace Instrument {

// ============================================================================

Voice::Voice (const Graph::Module* a_Module, float a_MinLevel) :
    m_MinLevel (a_MinLevel)
{

    // Store the module
    m_Module.reset(const_cast<Graph::Module*>(a_Module));

    // Get top-level output port(s)
    m_AudioPort[0] = m_Module->getPort("outL");
    m_AudioPort[1] = m_Module->getPort("outR");

    if ((m_AudioPort[0] == nullptr && m_AudioPort[1] != nullptr) || 
        (m_AudioPort[0] != nullptr && m_AudioPort[1] == nullptr))
    {
        THROW(BuildError,
            "The top-level module '%s' of type '%s' must both 'outL' and 'outR' output ports!",
            m_Module->getName().c_str(), m_Module->getType().c_str()
        );
    }

    if (m_AudioPort[0] == nullptr && m_AudioPort[1] == nullptr) {
        m_AudioPort[0] = m_Module->getPort("out");
        m_AudioPort[1] = nullptr;
    }

    if (m_AudioPort[0] == nullptr && m_AudioPort[1] == nullptr) {
        THROW(BuildError,
            "The top-level module '%s' of type '%s' must have either 'out' or 'outL' and 'outR' output ports!",
            m_Module->getName().c_str(), m_Module->getType().c_str()
        );
    }

    // Collect MIDI listeners
    std::function<void(Graph::Module*)> walkAndCollect = [&](Graph::Module* parent) {
        for (auto& it : parent->getSubmodules()) {
            auto& module = it.second;

            // Query interface
            auto iface = (Graph::Modules::IMidiListener*)module->queryInterface(
                Graph::Modules::IMidiListener::ID
            );

            // Store it
            if (iface != nullptr) {
                m_MidiListeners.push_back(iface);
            }

            // Walk recursively
            walkAndCollect(module.get());
        }
    };

    walkAndCollect(const_cast<Graph::Module*>(a_Module));

    // Create the output buffer
    m_Buffer.create(a_Module->getBufferSize(), 2);
}

Graph::Module* Voice::getModule () {
    return m_Module.get();
}

// ============================================================================

bool Voice::isStereo () const {
    return m_AudioPort[1] != nullptr;
}

bool Voice::isActive () const {
    return m_Active;
}

void Voice::activate () {

    // Already active
    if (m_Active) {
        return;
    }

    m_Module->start();

    m_Active     = true;
    m_Playing    = false;
    m_ActiveTime = 0;
    m_SilentTime = 0;
    m_PeakLevel  = -std::numeric_limits<float>::infinity(); 

    m_MidiEvents.clear();
}

void Voice::deactivate () {

    // Not active
    if (!m_Active) {
        return;
    }

    m_Module->stop();

    m_Active  = false;
    m_Playing = false;
}

// ============================================================================

int64_t Voice::getActiveTime () const {
    return m_ActiveTime;
}

int64_t Voice::getSilentTime () const {
    return m_SilentTime;
}

// ============================================================================

void Voice::pushEvent (const MIDI::Event& a_Event) {

    // Not active, pass all controller events immediately
    if (!m_Active) {
        for (auto listener : m_MidiListeners) {
            listener->pushEvent(a_Event);
        }
    }

    // Queue the event
    else {
        m_MidiEvents.push_back(a_Event);
    }
}

// ============================================================================

void Voice::process () {

    // Dispatch all MIDI events to MIDI listeners
    for (auto& event : m_MidiEvents) {
        for (auto listener : m_MidiListeners) {
            listener->pushEvent(event);
        }
    }

    // Cleat the event queue
    m_MidiEvents.clear();

    // Process audio
    for (size_t i=0; i<2; ++i) {
        if (m_AudioPort[i] != nullptr) {
            m_AudioPort[i]->process();
        }
    }

    for (size_t i=0; i<2; ++i) {
        if (m_AudioPort[i] != nullptr) {
            m_AudioPort[i]->setDirty(true);
        }
    }

    // Assemble the stereo buffer
    if (isStereo()) {
        size_t size = m_Buffer.getSize() * sizeof(float);
        memcpy(m_Buffer.data(0), m_AudioPort[0]->getBuffer().data(), size);
        memcpy(m_Buffer.data(1), m_AudioPort[1]->getBuffer().data(), size);
    }
    else {
        size_t size = m_Buffer.getSize() * sizeof(float);
        memcpy(m_Buffer.data(0), m_AudioPort[0]->getBuffer().data(), size);
        memcpy(m_Buffer.data(1), m_AudioPort[0]->getBuffer().data(), size);
    }

    // Compute peak sample value
    float  peak = 0.0f;
    size_t size = m_Buffer.getSize() * m_Buffer.getChannels();
    float* ptr  = m_Buffer.data();

    for (size_t i=0; i<size; ++i) {
        float mag = fabs(*ptr++);
        if (mag > peak) peak = mag;
    }

    // Convert to dB
    if (peak == 0.0f) {
        m_PeakLevel = -std::numeric_limits<float>::infinity();
    } else {
        m_PeakLevel = 20.0f * log10f(peak);
    }

    // Update times
    int64_t periodTime = 
        (1e3f * (float)m_Module->getBufferSize() / m_Module->getSampleRate());

    if (m_PeakLevel > m_MinLevel) {
        m_Playing     = true;
        m_SilentTime  = 0;
    } else {
        m_SilentTime += periodTime;
    }

    if (m_Playing) {
         m_ActiveTime += periodTime;
    }
}

const Audio::Buffer<float> Voice::getBuffer () const {
    return m_Buffer;
}

float Voice::getPeakLevel () const {
    return m_PeakLevel;
}

// ============================================================================

}; // Instrument
