#include "graph.hh"
#include "module.hh"
#include "exception.hh"

#include <utils/utils.hh>
#include <utils/exception.hh>

#include <strutils.hh>
#include <stringf.hh>

namespace Graph {

// ============================================================================

Module::Module (const std::string& a_Type, const std::string& a_Name,
                const Attributes& a_Attributes) :
    m_Type       (a_Type),
    m_Name       (a_Name),
    m_Attributes (a_Attributes)
{
    // Empty
}

// ============================================================================

IBaseInterface* Module::queryInterface (iid_t a_Id) {
    (void)a_Id;

    // No interfaces on the base class
    return nullptr;
}

// ============================================================================

float Module::getSampleRate () const {
    return m_SampleRate;
}

size_t Module::getBufferSize () const {
    return m_BufferSize;
}

// ============================================================================

std::string Module::getType () const {
    return m_Type;
}

std::string Module::getName () const {
    return m_Name;
}

std::string Module::getFullName () const {

    if (m_Parent == nullptr) {
        return getName();
    }

    std::string fullName = getName();
    for (auto module = getParent(); module; module = module->getParent()) {
        fullName = module->getName() + "." + fullName;
    }

    return fullName;
}

const Module::Ports& Module::getPorts () {
    return m_Ports;
}

const Module::Ports& Module::getPorts () const {
    return m_Ports;
}

Port* Module::getPort (const std::string& a_Name) {
    return m_Ports.get(a_Name, std::shared_ptr<Port>()).get();
}
    
const Dict<Port*, Port*> Module::getConnections () const {
    return m_Connections;
}

Module* Module::getParent () const {
    return m_Parent;
}

// ============================================================================

bool Module::isLeaf () const {
    return m_Submodules.empty();
}

void Module::addSubmodule (Module* a_Module) {

    // Check if it is not already there
    if (m_Submodules.has(a_Module->getName())) {
        THROW(BuildError, "Module '%s' already has a submodule '%s'",
            getFullName(), a_Module->getName());
    }

    // Add it
    a_Module->m_Parent = this;
    m_Submodules.set(a_Module->getName(), std::shared_ptr<Module>(a_Module));
}

const Module::Submodules& Module::getSubmodules () {
    return m_Submodules;
}

const Module::Submodules& Module::getSubmodules () const {
    return m_Submodules;
}

Module* Module::getSubmodule (const std::string& a_Name) {

    // Don't have that one
    if (!m_Submodules.has(a_Name)) {
        return nullptr;
    }

    return m_Submodules.get(a_Name).get();
}

// ============================================================================

Port* Module::addPort (Port* a_Port) {
    m_Ports.set(a_Port->getName(), std::shared_ptr<Port>(a_Port));
    return a_Port;
}

void Module::connect (Port* a_Src, Port* a_Dst) {

    // Don't connect from input except from a top-level input.
    if (a_Src->getDirection() == Port::Direction::INPUT) {
        if (a_Src->getModule() != this) {
            THROW(ConnectionError, "Cannot connect from input port '%s'!",
                a_Src->getFullName()
            );
        }
    }

    // Don't connect to output except to a top-level output.
    if (a_Dst->getDirection() == Port::Direction::OUTPUT) {
        if (a_Dst->getModule() != this) {
            THROW(ConnectionError, "Cannot connect to output port '%s'!",
                a_Dst->getFullName()
            );
        }
    }

    // Destination is a buffered port
    if (a_Dst->getType() == Port::Type::BUFFERED) {
        THROW(ConnectionError, "Cannot connect to a buffered port '%s'!",
            a_Dst->getFullName()
        );
    }

    // Destination already connected
    if (m_Connections.has(a_Dst)) {
        THROW(ConnectionError, "Destination port '%s' is already connected!",
            a_Dst->getFullName()
        );
    }

    // Make the connection
    m_Connections.set(a_Dst, a_Src);
}

// ============================================================================

void Module::prepare (float a_SampleRate, size_t a_BufferSize) {

    // Store parameters
    m_SampleRate = a_SampleRate;
    m_BufferSize = a_BufferSize;

    // Discover sources and sinks on all ports
    for (auto& itr : m_Ports) {
        auto& port = itr.second;
        port->updateSourcesAndSinks();
    }

    // Set new buffers in all ports
    for (auto& itr : m_Ports) {
        auto& port = itr.second;
        port->setBuffer(Audio::Buffer<float>(a_BufferSize, 1));
    }

    // Call on all submodules
    for (auto& it : m_Submodules) {
        auto child = it.second;
        child->prepare(a_SampleRate, a_BufferSize);
    }
}

void Module::start () {

    // Call on all submodules
    for (auto& it : m_Submodules) {
        auto child = it.second;
        child->start();
    }
}

void Module::stop () {

    // Call on all submodules
    for (auto& it : m_Submodules) {
        auto child = it.second;
        child->stop();
    }
}

void Module::process () {
    // Empty
}

// ============================================================================

const Module::Attributes Module::getAttributes () const {
    Attributes attributes;

    // Recursive collection function
    std::function<void(const Module*, const std::string&)> collect = 
        [&](const Module* module, const std::string& a_Prefix)
    {
        const std::string prefix = a_Prefix.empty() ? "" : (a_Prefix + ".");

        // Append own attributes with prefix
        for (auto& it : module->m_Attributes) {
            const std::string name = prefix +  it.first;
            attributes.set(name, it.second);
        }

        // Walk recursively
        for (auto& it : module->m_Submodules) {
            auto submodule = it.second.get();
            collect(submodule, prefix + submodule->getName());
        }
    };

    // Collect & return
    collect(this, "");
    return attributes;
}

const Module::Parameters Module::getParameters () const {
    Parameters parameters;

    // Recursive collection function
    std::function<void(const Module*, const std::string&)> collect = 
        [&](const Module* module, const std::string& a_Prefix)
    {
        const std::string prefix = a_Prefix.empty() ? "" : (a_Prefix + ".");

        // Append own parameters with prefix
        for (auto& it : module->m_Parameters) {
            const std::string name = prefix +  it.first;
            parameters.set(name, it.second);
        }

        // Walk recursively
        for (auto& it : module->m_Submodules) {
            auto submodule = it.second.get();
            collect(submodule, prefix + submodule->getName());
        }
    };

    // Collect & return
    collect(this, "");
    return parameters;
}

void Module::updateParameters (const Module::ParameterValues& a_Values) {
    Dict<std::string, Module::ParameterValues> values;

    // Process values addressed to this module
    for (auto& itr : a_Values) {
        auto& name  = itr.first;
        auto& value = itr.second;

        // Split fields
        auto fields = strutils::split(name, ".", 1);

        // This value targets this module
        if (fields.size() == 1) {

            Graph::logger->debug("Setting '{}.{}' to '{}'",
                getFullName(), name, value.asString());

            // We have this parameter
            if (m_Parameters.has(name)) {
                auto& parameter = m_Parameters.get(name);
                if (parameter.isLocked()) {
                    THROW(ParameterError,
                        "Tried to set locked parameter '%s' on module '%s'!",
                        name.c_str(),
                        m_Name.c_str()
                    );
                }
                else {
                    parameter = value;
                }
            }

            // We don't have this parameter
            else {
                THROW(ParameterError,
                    "Module '%s' does not have a parameter '%s'!",
                    m_Name.c_str(),
                    name.c_str()
                );
            }
        }

        // This value targets (possibly) a submodule. Store it under a key
        else {

            if (!values.has(fields[0])) {
                values.set(fields[0], Module::ParameterValues());
            }

            auto& subValues = values.get(fields[0]);
            subValues.set(fields[1], value);
        }
    }

    // Dispatch to submodules
    for (auto& itr : values) {
        auto& name = itr.first;
        auto& subValues = itr.second;

        // Find the submodule
        auto module = getSubmodule(name);
        if (module == nullptr) {
            THROW(ParameterError, "Module '%s' does not have a submodule '%s'!",
                m_Name.c_str(), name.c_str()
            );
        }

        // Recurse
        module->updateParameters(subValues);
    }
}


// ============================================================================

void Module::applyParameterOverrides (const Module::Attributes& a_Attributes) {

    // Keywords
    const std::vector<std::string> keywords = 
        {"step", "min", "max", "def", "locked"};

    // Lock all parameters, They may be unlocked selectively by other override
    // attributes.
    if (std::stoi(a_Attributes.get("lockParameters", "0")) != 0) {
        for (auto& it : m_Parameters) {
            it.second.setLock(true);
        }
    }

    // Identify meaningful attributes
    for (auto& it : a_Attributes) {

        // Get fields
        auto fields = strutils::rsplit(it.first, ".", 1);
        if (fields.size() < 2) {
            continue;
        }

        // Check each
        for (auto keyword : keywords) {

            // Not this one
            if (fields[1] != keyword) {
                continue;
            }

            // Check if we have a parameter with that name
            const std::string name = fields[0];
            if (!m_Parameters.has(name)) {
                THROW(ParameterError, 
                    "Module '%s' of type '%s' does not have a parameter '%s'",
                    m_Name.c_str(), m_Type.c_str(), name.c_str()
                );
            }

            // Get the parameter and alter it
            auto& parameter = m_Parameters.get(name);
            Parameter::Type type = parameter.getType();

            // A choice parameter
            if (type == Parameter::Type::CHOICE) {

                // Error
                if (keyword == "min" || keyword == "max" || keyword == "step") {
                    THROW(ParameterError,
                        "Cannot set '%s' of a choice parameter '%s'",
                        keyword.c_str(), name.c_str()
                    );
                }

                // Value override
                if (keyword == "def") {
                    parameter = Parameter::Value(it.second);
                }
            }

            // A numerical parameter
            else if (type == Parameter::Type::NUMBER) {

                if (keyword == "step") {
                    parameter.setStep(Utils::stof(it.second));
                }
                else if (keyword == "min") {
                    parameter.setMin(Utils::stof(it.second));
                }
                else if (keyword == "max") {
                    parameter.setMax(Utils::stof(it.second));
                }
                else if (keyword == "def") {
                    float value = Utils::stof(it.second);

                    if (value < parameter.getMin()) {
                        parameter.setMin(value);
                    }
                    if (value > parameter.getMax()) {
                        parameter.setMax(value);
                    }

                    parameter = Parameter::Value(value);
                }
            }

            // Lock / unlock
            if (keyword == "locked") {
                parameter.setLock(std::stoi(it.second) != 0);
            }
        }
    }
}

// ============================================================================

}; // Grapth

