#include "../exception.hh"

#include "vcf.hh"

#include <utils/utils.hh>
#include <utils/exception.hh>
#include <stringf.hh>

#include <cmath>

namespace Graph {
namespace Modules {

// ============================================================================

VCF::VCF (const std::string& a_Name,
          const Module::Attributes& a_Attributes) :
    Module ("vcf", a_Name, a_Attributes)
{
    // Input ports
    m_Input  = addPort(new Port(this, "in",   Port::Direction::INPUT, 0.0f));
    m_Freq   = addPort(new Port(this, "freq", Port::Direction::INPUT, 0.0f));
    m_Gain   = addPort(new Port(this, "gain", Port::Direction::INPUT, 0.0f));
    m_Q      = addPort(new Port(this, "q",    Port::Direction::INPUT, 1.0f));

    // Output port
    m_Output = addPort(new Port(this, "out",  Port::Direction::OUTPUT));

    // Bypass
    m_Parameters.set("bypass", Parameter("off", {"off", "on"}, "Bypass"));
    // Filter type
    m_Parameters.set("type", Parameter("lpf", {
        "lpf",
        "hpf",
        "bpf",
        "notch",
        "apf",
        "peaking",
        "lowShelf",
        "highShelf"
    }, "Filter type"));

    // Apply overrides
    applyParameterOverrides(a_Attributes);
}

Module* VCF::create (
    const std::string& a_Type,
    const std::string& a_Name,
    const Module::Attributes& a_Attributes)
{
    (void)a_Type;
    return new VCF(a_Name, a_Attributes);
}

// ============================================================================

void VCF::start () {

    // Reset state
    m_InputState.type = -1;
    m_InputState.cv   = 0.0f;
    m_InputState.gain = 0.0f;
    m_InputState.q    = 0.0f;

    m_Filter.reset();
}

void VCF::process () {

    // Bypass
    bool bypass = m_Parameters.get("bypass").get().asNumber();
    if (bypass) {
        m_InputState.type = -1;
        m_Filter.reset();
    }

    // Set coefficient computation function pointer
    const Processing::BiquadIIR::Coeffs (*compute)(float, float, float, float) = nullptr;
    int32_t type = -1;

    if (!bypass) {
        type = m_Parameters.get("type").get().asNumber();

        switch(type)
        {
        case 0: compute = Processing::BiquadIIR::computeLPF;       break;
        case 1: compute = Processing::BiquadIIR::computeHPF;       break;
        case 2: compute = Processing::BiquadIIR::computeBPF;       break;
        case 3: compute = Processing::BiquadIIR::computeNotch;     break;
        case 4: compute = Processing::BiquadIIR::computeAPF;       break;
        case 5: compute = Processing::BiquadIIR::computePeak;      break;
        case 6: compute = Processing::BiquadIIR::computeLowShelf;  break;
        case 7: compute = Processing::BiquadIIR::computeHighShelf; break;
        default: THROW(ProcessingError, "Invalid filter type %d!", type);
        }
    }

    // Get pointers
    const float* ptrIn   = m_Input->process().data();
    const float* ptrFreq = m_Freq->process().data();
    const float* ptrGain = m_Gain->process().data();
    const float* ptrQ    = m_Q->process().data();
    float*       ptrOut  = m_Output->getBuffer().data();

    // Bypass
    if (bypass) {
        size_t size = m_BufferSize * sizeof(float);
        memcpy(ptrOut, ptrIn, size);
    }
    
    // Process
    else {
        for (size_t i=0; i<m_BufferSize; ++i) {

            // Get control state
            float cv   = *ptrFreq++;
            float gain = *ptrGain++;
            float q    = *ptrQ++;

            // Something changed, recompute
            if (m_InputState.type != type ||
                m_InputState.cv   != cv   ||
                m_InputState.gain != gain ||
                m_InputState.q    != q)
            {
                float f = Utils::cvToFrequency(cv);

                // Limit
                if (q <  0.1f) q =  0.1f; // FIXME: Arbitrary!
                if (q > 20.0f) q = 20.0f;

                // Compute
                m_Filter.setCoeffs(compute(f, gain, q, m_SampleRate));

                // Store state
                m_InputState.type = type;
                m_InputState.cv   = cv;
                m_InputState.gain = gain;
                m_InputState.q    = q;
            }

            // Filter
            *ptrOut++ = m_Filter.process(*ptrIn++);
        }
    }
}

// ============================================================================

}; // Modules
}; // Graph

