#include "../graph.hh"
#include "../exception.hh"
#include "../parameter.hh"
#include "../processing/waveform.hh"

#include "vco.hh"

#include <utils/utils.hh>
#include <utils/exception.hh>
#include <utils/math.hh>
#include <stringf.hh>

#include <cmath>

namespace Graph {
namespace Modules {

// ============================================================================

VCO::VCO (const std::string& a_Name,
          const Module::Attributes& a_Attributes) :
    Module ("vco", a_Name, a_Attributes)
{
    // Input ports
    m_CvIn   = addPort(new Port(this, "cv" , Port::Direction::INPUT, 0.0f));
    m_AmIn   = addPort(new Port(this, "am" , Port::Direction::INPUT, 0.0f));
    m_FmIn   = addPort(new Port(this, "fm" , Port::Direction::INPUT, 0.0f));
    m_PwmIn  = addPort(new Port(this, "pwm", Port::Direction::INPUT, 0.5f));

    // Output ports
    m_Output = addPort(new Port(this, "out", Port::Direction::OUTPUT));

    // Define parameters
    m_Parameters.set("waveform",  Parameter("sine", {
        "sine",
        "half_sine",
        "abs_sine",
        "pulse_sine",
        "even_sine",
        "even_abs_sine",
        "square",
        "derived_square",
        "triangle",
        "sawtooth",
    }, "Waveform"));

    const float semitone = 1.0 / 12.0;

    m_Parameters.set("amplitude", Parameter(-6.0f, -30.0,      0.0f,     0.1f,   "Amplitude [dB]"));
    m_Parameters.set("phase",     Parameter( 0.0f, -180.0,    +180.0f,   1.0f,   "Phase [deg]"));
    m_Parameters.set("detune",    Parameter( 0.0f, -semitone, +semitone, 0.001f, "Detune"));
    m_Parameters.set("amGain",    Parameter( 0.5f,  0.0f,      1.0f,     0.05f,  "AM modulation index"));
    m_Parameters.set("fmGain",    Parameter( 0.1f,  0.0f,      1.0f,     0.05f,  "FM modulation index"));

    // Apply overrides
    applyParameterOverrides(a_Attributes);
}

Module* VCO::create (
    const std::string& a_Type,
    const std::string& a_Name,
    const Module::Attributes& a_Attributes)
{
    (void)a_Type;
    return new VCO(a_Name, a_Attributes);
}

// ============================================================================

void VCO::prepare (float a_SampleRate, size_t a_BufferSize) {

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

void VCO::start () {
    m_Phase = 0.0f;
}

// ============================================================================

void VCO::process () {

    // Scaling factor - HZ to cycles
    const float k = 1.0f / m_SampleRate;

    // Waveform
    float (*waveFunc)(float, float) = nullptr;
    int32_t wave = m_Parameters.get("waveform").get().asNumber();

    switch (wave)
    {
    case 0:  waveFunc = Processing::Waveform::sine;          break;
    case 1:  waveFunc = Processing::Waveform::half_sine;     break;
    case 2:  waveFunc = Processing::Waveform::abs_sine;      break;
    case 3:  waveFunc = Processing::Waveform::pulse_sine;    break;
    case 4:  waveFunc = Processing::Waveform::even_sine;     break;
    case 5:  waveFunc = Processing::Waveform::even_abs_sine; break;
    case 6:  waveFunc = Processing::Waveform::square;        break;
    case 7:  waveFunc = Processing::Waveform::derived_square;break;
    case 8:  waveFunc = Processing::Waveform::triangle;      break;
    case 9:  waveFunc = Processing::Waveform::sawtooth;      break;
    default: THROW(ProcessingError, "Invalid waveform id %d", wave);
    }

    // Amplitude
    float A = m_Parameters.get("amplitude").get().asNumber();
    A = Utils::Math::log2lin(A);

    // Phase
    float phaseOffset = m_Parameters.get("phase").get().asNumber();
    phaseOffset /= 360.0f;

    // Detune amount
    float detune = m_Parameters.get("detune").get().asNumber();

    // Amplitude modulation index
    float alpha = m_Parameters.get("amGain").get().asNumber();
    // Frequency modulation index
    float beta  = m_Parameters.get("fmGain").get().asNumber();

    // Get pointers
    float* ptrOut         = m_Output->getBuffer().data();
    const float* ptrCvIn  = m_CvIn->process().data();
    const float* ptrAmIn  = m_AmIn->process().data();
    const float* ptrFmIn  = m_FmIn->process().data();
    const float* ptrPwmIn = m_PwmIn->process().data();

    // Add phase offset
    float phi = m_Phase + phaseOffset;
    while (phi > 1.0f) phi -= 1.0f;
    while (phi < 0.0f) phi += 1.0f;

    // Generate the wave
    for (size_t i=0; i<m_BufferSize; ++i) {

        // Add AM modulation
        float am = *ptrAmIn++;
        float a  = A * (1.0f + alpha * am);        

        // Convert CV to frequency
        float cv = *ptrCvIn++;
        float f  = Utils::cvToFrequency(cv + detune);

        // Add FM modulation
        float fm = *ptrFmIn++;
        f *= (1.0f + beta * fm);

        // Generate the waveform
        float pwm = *ptrPwmIn++;
        *ptrOut++ = a * waveFunc(phi, pwm);

        // Accumulate phase
        phi += f * k;
        while (phi > 1.0f) phi -= 1.0f;
    }

    // Subtract phase offset
    phi -= phaseOffset;
    while (phi > 1.0f) phi -= 1.0f;
    while (phi < 0.0f) phi += 1.0f;

    m_Phase = phi;
}

// ============================================================================

}; // Modules
}; // Graph

