#include "adsr.hh"
#include "../parameter.hh"

#include <strutils.hh>
#include <stringf.hh>
#include <utils/exception.hh>

#include <cmath>

namespace Graph {
namespace Modules {

// ============================================================================

ADSR::ADSR (const std::string& a_Name,
            const Attributes& a_Attributes) :
    Envelope ("adsr", a_Name, a_Attributes)
{
    // Remove points that may have been added in the Envelope's constructor
    m_Points.clear();

    // Parameters
    const float lvlMin  = -96.0f;
    const float lvlMax  = 0.0f;
    const float lvlSus  = -6.0f;
    const float lvlStep = 0.1f;

    m_Parameters.set("attackTime",    Parameter(0.010f, 0.001f, 0.250f, 0.001f,  "Attack time [s]"));
    m_Parameters.set("decayTime",     Parameter(0.040f, 0.001f, 0.250f, 0.001f,  "Decay time [s]"));
    m_Parameters.set("sustainTime",   Parameter(0.050f, 0.001f, 0.500f, 0.001f,  "Sustain time [s]"));
    m_Parameters.set("releaseTime",   Parameter(1.500f, 0.001f, 5.000f, 0.001f,  "Release time [s]"));

    m_Parameters.set("attackLevel",   Parameter(lvlMax, lvlMin, lvlMax, lvlStep, "Attack level"));
    m_Parameters.set("sustainLevel",  Parameter(lvlSus, lvlMin, lvlMax, lvlStep, "Sustain level"));
    m_Parameters.set("releaseLevel",  Parameter(lvlMin, lvlMin, lvlMax, lvlStep, "Release level"));

    m_Parameters.set("sustainEnable", Parameter("yes", {"no", "yes"}, "Sustain enable"));

    // Apply overrides
    applyParameterOverrides(a_Attributes);

    // Update the envelope points
    updateEnvelope();
}

Module* ADSR::create (
    const std::string& a_Type,
    const std::string& a_Name,
    const Attributes& a_Attributes)
{
    (void)a_Type;
    return new ADSR(a_Name, a_Attributes);
}

// ============================================================================

void ADSR::updateEnvelope () {

    // Clear, build the ADSR envelope
    m_Points.clear();
    float time = 0.0f;

    // 0
    m_Points.push_back(Point(
        time,
        m_Parameters.get("releaseLevel").get().asNumber()
    ));

    // A
    time += m_Parameters.get("attackTime").get().asNumber();
    m_Points.push_back(Point(
        time,
        m_Parameters.get("attackLevel").get().asNumber()
    ));

    // D
    time += m_Parameters.get("decayTime").get().asNumber();
    m_Points.push_back(Point(
        time,
        m_Parameters.get("sustainLevel").get().asNumber()
    ));

    // S
    bool sustainEnable = m_Parameters.get("sustainEnable").get().asNumber() > 0.5f;
    time += m_Parameters.get("sustainTime").get().asNumber();
    m_Points.push_back(Point(
        time,
        m_Parameters.get("sustainLevel").get().asNumber(),
        sustainEnable
    ));

    // R
    time += m_Parameters.get("releaseTime").get().asNumber();
    m_Points.push_back(Point(
        time,
        m_Parameters.get("releaseLevel").get().asNumber()
    ));

    // Check
    sanityCheckPoints();
}

void ADSR::updateParameters (const ParameterValues& a_Values) {

    // Update parameters
    Module::updateParameters(a_Values);
    // Update the envelope points
    updateEnvelope();
}

// ============================================================================

}; // Modules
}; // Graph

