#include "factory.hh"
#include "exception.hh"

#include <utils/xml2et.hh>

#include <stringf.hh>

namespace Instrument {

// ============================================================================

Instrument* createInstrument (Graph::Builder* a_Builder,
                              const ElementTree::Node* a_Node,
                              size_t a_SampleRate,
                              size_t a_BufferSize)
{
    Instrument::Attributes attributes;

    // Get name
    if (!a_Node->hasAttribute("name")) {
        throw BuildError("Instrument must have a name!");
    }
    const std::string name = a_Node->getAttribute("name");

    // Get top-level module name
    if (!a_Node->hasAttribute("module")) {
        THROW(BuildError,
            "Instrument '%s' must have a top-level module type provided!",
            name.c_str()
        );
    }
    const std::string module = a_Node->getAttribute("module");

    // TODO: Make attribute collection a function.
    // Collect attributes from the "instrument" tag
    for (auto& it : a_Node->getAttributes()) {
        if (it.first == "name" || it.first == "module") {
            continue;
        }

        attributes.set(it.first, it.second);
    }

    // Collect attributes from "attribute" tags
    for (auto node : a_Node->findAll("attribute")) {
        if (!node->hasAttribute("name")) {
            throw BuildError("An 'attribute' tag must have a 'name'!");
        }
        if (!node->hasAttribute("value")) {
            throw BuildError("An 'attribute' tag must have a 'value'!");
        }

        const std::string name  = node->getAttribute("name");
        const std::string value = node->getAttribute("value");

        if (attributes.has(name)) {
            THROW(BuildError, "Attribute '%s' redefined!", name.c_str());
        }

        attributes.set(name, value);
    }

    // Create the instrument
    return new Instrument(
        name,
        module,
        a_Builder,
        a_SampleRate,
        a_BufferSize,
        attributes
    );
}

// ============================================================================

Instruments loadInstruments (const std::string& a_Config,
                             size_t a_SampleRate,
                             size_t a_BufferSize)
{
    // Load the config
    auto root = xmlToElementTree(a_Config);
    if (root == nullptr) {
        THROW(std::runtime_error, "Error loading file '%s'", a_Config.c_str());
    }

    // Get sections
    auto modules = root->find("modules");
    if (modules == nullptr) {
        throw BuildError("No 'modules' section in the config file!");
    }

    auto instruments = root->find("instruments");
    if (instruments == nullptr) {
        throw BuildError("No 'instruments' section in the config file!");
    }

    // Create graph builder
    Graph::Builder builder;

    builder.registerBuiltinModules();
    builder.registerDefinedModules(modules.get());

    // Create instruments
    Instruments instrumentObjects;
    for (auto node : instruments->findAll("instrument")) {

        // Create the instrument
        auto instrument = std::shared_ptr<Instrument>(
            createInstrument(&builder, node.get(), a_SampleRate, a_BufferSize));

        // Check the name
        auto name = instrument->getName();
        if (instrumentObjects.has(name)) {
            THROW(BuildError, "Duplicate instrument name '%s'! Not adding.",
                name.c_str());
        }

        // Store
        instrumentObjects.set(name, instrument);
    }

    return instrumentObjects;
}

// ============================================================================

}; // Instrument
