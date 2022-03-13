#include "../graph.hh"
#include "../exception.hh"
#include "../parameter.hh"

#include "sampler.hh"

#include <utils/utils.hh>
#include <utils/exception.hh>
#include <utils/math.hh>
#include <stringf.hh>

#include <cmath>

namespace Graph {
namespace Modules {

// ============================================================================

Sampler::Sampler (const std::string& a_Name,
                  const Module::Attributes& a_Attributes) :
    Module ("sampler", a_Name, a_Attributes)
{
    // Throw an error if a waveform file is not specified
    if (!a_Attributes.has("file")) {
        THROW(ModuleError, "No 'file' attribute for sampler");
    }

    // Base frequency (note) of the sample waveform
    m_BaseFreq = Utils::noteToFrequency(a_Attributes.get("note", "C4"));

    // Load the waveform
    m_Sampler.load(a_Attributes.get("file"));

    // Input ports
    m_CvIn = addPort(new Port(this, "cv" , Port::Direction::INPUT, 0.0f));
    m_AmIn = addPort(new Port(this, "am" , Port::Direction::INPUT, 0.0f));
    m_FmIn = addPort(new Port(this, "fm" , Port::Direction::INPUT, 0.0f));

    // Output ports
    m_Output = addPort(new Port(this, "out", Port::Direction::OUTPUT));

    m_Parameters.set("amplitude", Parameter(-6.0f, -30.0, 0.0f, 0.1f,  "Amplitude [dB]"));
    m_Parameters.set("amGain",    Parameter( 0.5f,  0.0f, 1.0f, 0.05f, "AM modulation index"));
    m_Parameters.set("fmGain",    Parameter( 0.1f,  0.0f, 1.0f, 0.05f, "FM modulation index"));

    // Apply overrides
    applyParameterOverrides(a_Attributes);
}

Module* Sampler::create (
    const std::string& a_Type,
    const std::string& a_Name,
    const Module::Attributes& a_Attributes)
{
    (void)a_Type;
    return new Sampler(a_Name, a_Attributes);
}

// ============================================================================

void Sampler::prepare (float a_SampleRate, size_t a_BufferSize) {

    // Call the base method
    Module::prepare(a_SampleRate, a_BufferSize);

    // Lock parameters of unconnected ports
    if (!m_AmIn->isConnected()) {
        m_Parameters.get("amGain").setLock(true);
    }
    if (!m_FmIn->isConnected()) {
        m_Parameters.get("fmGain").setLock(true);
    }
}

void Sampler::start () {
    m_Phase = 0.0f;
}

// ============================================================================

void Sampler::process () {

    // Scaling factor - HZ to cycles
    const float k = 1.0f / (m_SampleRate * m_BaseFreq * m_Sampler.getLength());

    // Amplitude
    float A = m_Parameters.get("amplitude").get().asNumber();
    A = Utils::Math::log2lin(A);

    // Amplitude modulation index
    float alpha = m_Parameters.get("amGain").get().asNumber();
    // Frequency modulation index
    float beta  = m_Parameters.get("fmGain").get().asNumber();

    // Get pointers
    float* ptrOut         = m_Output->getBuffer().data();
    const float* ptrCvIn  = m_CvIn->process().data();
    const float* ptrAmIn  = m_AmIn->process().data();
    const float* ptrFmIn  = m_FmIn->process().data();

    // Generate the wave
    float phi = m_Phase;
    for (size_t i=0; i<m_BufferSize; ++i) {

        // Add AM modulation
        float am = *ptrAmIn++;
        float a  = A * (1.0f + alpha * am);        

        // Convert CV to frequency
        float f = Utils::cvToFrequency(*ptrCvIn++);

        // Add FM modulation
        float fm = *ptrFmIn++;
        f *= (1.0f + beta * fm);

        // Generate the waveform
        *ptrOut++ = a * m_Sampler.getSample(phi);

        // Accumulate phase
        phi += f * k;
        while (phi > 1.0f) phi -= 1.0f;
    }

    m_Phase = phi;
}

// ============================================================================

}; // Modules
}; // Graph

