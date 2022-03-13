#include "builder.hh"
#include "exception.hh"

#include <utils/exception.hh>
#include <stringf.hh>
#include <strutils.hh>

#include "modules/constant.hh"
#include "modules/adder.hh"
#include "modules/multiplier.hh"
#include "modules/mixer.hh"
#include "modules/midi_source.hh"
#include "modules/midi_ctrl.hh"
#include "modules/noise.hh"
#include "modules/vco.hh"
#include "modules/envelope.hh"
#include "modules/adsr.hh"
#include "modules/vga.hh"
#include "modules/vcf.hh"
#include "modules/soft_clipper.hh"
#include "modules/sampler.hh"

#include <spdlog/sinks/stdout_color_sinks.h>

namespace Graph {

// ============================================================================

Builder::Builder () {

    // Create logger
    m_Logger = spdlog::get("builder");
    if (!m_Logger) m_Logger = spdlog::stderr_color_mt("builder");
}

// ============================================================================

void Builder::registerBuiltinModules () {
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;

    m_Creators.set("constant",       std::bind(&Modules::Constant::create,       _1, _2, _3));
    m_Creators.set("adder",          std::bind(&Modules::Adder::create,          _1, _2, _3));
    m_Creators.set("multiplier",     std::bind(&Modules::Multiplier::create,     _1, _2, _3));
    m_Creators.set("mixer",          std::bind(&Modules::Mixer::create,          _1, _2, _3));
    m_Creators.set("midiSource",     std::bind(&Modules::MidiSource::create,     _1, _2, _3));
    m_Creators.set("midiController", std::bind(&Modules::MidiController::create, _1, _2, _3));
    m_Creators.set("noise",          std::bind(&Modules::Noise::create,          _1, _2, _3));
    m_Creators.set("vco",            std::bind(&Modules::VCO::create,            _1, _2, _3));
    m_Creators.set("envelope",       std::bind(&Modules::Envelope::create,       _1, _2, _3));
    m_Creators.set("adsr",           std::bind(&Modules::ADSR::create,           _1, _2, _3));
    m_Creators.set("vga",            std::bind(&Modules::VGA::create,            _1, _2, _3));
    m_Creators.set("vcf",            std::bind(&Modules::VCF::create,            _1, _2, _3));
    m_Creators.set("softClipper",    std::bind(&Modules::SoftClipper::create,    _1, _2, _3));
    m_Creators.set("sampler",        std::bind(&Modules::Sampler::create,        _1, _2, _3));
}

void Builder::registerDefinedModules (const ElementTree::Node* a_Defs) {

    // Collect module definitions if given
    for (auto node : a_Defs->findAll("module")) {

        // Must have a type
        if (!node->hasAttribute("type")) {
            throw BuildError("A module definition must have a type!"); 
        }
        // Cannot have a name
        if (node->hasAttribute("name")) {
            throw BuildError("A module definition cannot have a name!"); 
        }

        // Check if not already defined
        const std::string type = node->getAttribute("type");
        if (m_Creators.has(type)) {
            THROW(BuildError, "Module type '%s' already defined!", type.c_str());
        }

        m_Logger->info("Registering module type '{}'", type);

        // Store the definition
        m_ModuleDefs.set(type, node);

        // Store the creator
        using std::placeholders::_1;
        using std::placeholders::_2;
        using std::placeholders::_3;
        m_Creators.set(type, std::bind(&Builder::createModule, this, _1, _2, _3));
    }
}

const std::vector<std::string> Builder::listModuleTypes () const {
    std::vector<std::string> types;
    for (auto& it : m_Creators) {
        types.push_back(it.first);
    }

    return types;
}

// ============================================================================

Module::Attributes Builder::collectAttributes (const ElementTree::Node* a_Node) {
    Module::Attributes attributes;

    // Extract attributes, skip "name" and "type"
    for (auto& it : a_Node->getAttributes()) {
        if (it.first == "name" || it.first == "type") {
            continue;
        }
        attributes.set(it.first, it.second);
    }

    return attributes;
}

Module::Attributes Builder::collectParameters (const ElementTree::Node* a_Node) {
    Module::Attributes attributes;

    const std::vector<std::string> keywords = 
        {"def", "min", "max", "step", "locked"};

    // Extract parameter overrides
    for (auto node : a_Node->findAll("parameter")) {

        // Must specify a name
        if (!node->hasAttribute("name")) {
            throw BuildError("Parameter override must have a 'name' attribute");
        }

        // Look for keywords
        const std::string name = node->getAttribute("name");
        for (auto keyword : keywords) {
            if (node->hasAttribute(keyword)) {
                std::string attr = name + "." + keyword;
                attributes.set(attr, node->getAttribute(keyword));
            }
        }
    }

    return attributes;
}

// ============================================================================

Module* Builder::build (const std::string& a_Type, const std::string& a_Name) {

    m_Logger->debug("Building top-level module '{}' of type '{}'...",
        a_Name, a_Type);

    return createModule(a_Type, a_Name, Module::Attributes());
}

Module* Builder::createModule (const std::string& a_Type, const std::string& a_Name,
                               const Module::Attributes& a_Attributes)
{
    // Get the module definition
    auto moduleDesc = m_ModuleDefs.get(a_Type);
    if (!moduleDesc) {
        THROW(BuildError, "No definition for module type '%s'!", a_Type.c_str());
    }

    // Create the module
    auto module = new Module(a_Type, a_Name, a_Attributes);

    // Add ports
    for (auto& node : moduleDesc->findAll("input")) {
        if (!node->hasAttribute("name")) {
            throw BuildError("An input port tag must have a name specified!");            
        }

        const std::string portName = node->getAttribute("name");
        module->addPort(new Port(module, portName, Port::Direction::INPUT, 0.0f));
    }

    for (auto& node : moduleDesc->findAll("output")) {
        if (!node->hasAttribute("name")) {
            throw BuildError("An output port tag must have a name specified!");            
        }

        const std::string portName = node->getAttribute("name");
        module->addPort(new Port(module, portName, Port::Direction::OUTPUT, 0.0f));
    }

    // Create submodules
    for (auto& node : moduleDesc->findAll("module")) {

        // Must have name and type
        if (!node->hasAttribute("type")) {
            throw BuildError("A module instance must have a type!");
        }
        if (!node->hasAttribute("name")) {
            throw BuildError("A module instance must have a name!");
        }

        // Check if the type is legal
        const std::string type = node->getAttribute("type");
        const std::string name = node->getAttribute("name");

        m_Logger->debug("Building sub-module '{}' of type '{}'...",
            name, type);

        if (!m_Creators.has(type)) {
            THROW(BuildError, "Unknown module type '%s'", type.c_str());
        }

        // Gather attributes and parameter overrides
        auto attributes = collectAttributes(node.get());
        attributes.update(collectParameters(node.get()));

        // Overlay overrides specified at the parent module. Do this by
        // identifying ones that have the module name as a prefix. Strip
        // the prefix and pass them further.
        for (auto& it : a_Attributes) {
            const std::string pat = name + ".";
            if (strutils::startswith(it.first, pat)) {
                const std::string attr = strutils::replace(it.first, pat, "");
                attributes.set(attr, it.second);
            }
        }

        // Debug
        for (auto& it : attributes) {
            m_Logger->debug(" '{}' = '{}'", it.first, it.second);
        }

        // Build and add the submodule
        auto creator   = m_Creators.get(type);
        auto submodule = creator(type, name, attributes);

        module->addSubmodule(submodule);
    }

    // Make port connections
    for (auto& node : moduleDesc->findAll("patch")) {

        // Must have "from" and "to"
        if (!node->hasAttribute("from")) {
            throw BuildError("Missing 'from' attribute in patch spec.");
        }
        if (!node->hasAttribute("to")) {
            throw BuildError("Missing 'to' attribute in patch spec.");
        }

        // A helper function. Extracts module and port name.
        auto getModuleAndPortName = [](const std::string& str) {
            auto parts = strutils::split(str, ".");

            // Only port
            if (parts.size() == 1) {
                return std::make_pair(std::string(), parts[0]);
            }
            // Module and port
            else if (parts.size() == 2) {
                return std::make_pair(parts[0], parts[1]);
            }

            // Incorrect
            THROW(BuildError, 
                "Invalid module port specification: '%s'", str.c_str()
            );
        };

        // A helper function. Gets pointer to the port
        auto getPort = [&](std::pair<std::string, std::string>& pair) {

            // No module name, get a top-level port
            if (pair.first.empty()) {

                auto port = module->getPort(pair.second);
                if (port == nullptr) {
                    THROW(BuildError, "Module '%s' doesn't have a port '%s'!",
                        module->getName().c_str(), pair.second.c_str()
                    );
                }

                return port;
            }

            // Find the module and then its port
            else {

                auto submodule = module->getSubmodule(pair.first);
                if (submodule == nullptr) {
                    THROW(BuildError, "Module '%s' doesn't have a submodule '%s'!",
                        module->getName().c_str(), pair.first.c_str()
                    );
                }

                auto port = submodule->getPort(pair.second);
                if (port == nullptr) {
                    THROW(BuildError, "Module '%s' doesn't have a port '%s'!",
                        submodule->getName().c_str(), pair.second.c_str()
                    );
                }

                return port;
            }
        };

        // Get them
        const std::string srcSpec = node->getAttribute("from");
        const std::string dstSpec = node->getAttribute("to");

        // Get module,port pairs
        auto srcPair = getModuleAndPortName(srcSpec);
        auto dstPair = getModuleAndPortName(dstSpec);

        // Get ports
        auto srcPort = getPort(srcPair);
        auto dstPort = getPort(dstPair);

        m_Logger->debug("Connecting from '{}' to '{}'...", 
            srcPort->getFullName(), dstPort->getFullName());

        // Connect ports
        module->connect(srcPort, dstPort);
    }

    // Return the module
    return module;
}

// ============================================================================

}; // Graph
