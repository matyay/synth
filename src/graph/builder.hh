#ifndef GRAPH_BUILDER_HH
#define GRAPH_BUILDER_HH

#include "module.hh"

#include <utils/dict.hh>
#include <utils/element_tree.hh>

#include <spdlog/spdlog.h>

#include <functional>
#include <string>
#include <vector>
#include <memory>

namespace Graph {

// ============================================================================

class Builder {
public:

    /// Module creation function
    typedef std::function<Module* (const std::string&, const std::string&, 
                                   const Module::Attributes&)> CreateFunc;

    /// Constructor
    Builder ();

    /// Registers built-in module types
    void registerBuiltinModules ();
    /// Registers defined module types
    void registerDefinedModules (const ElementTree::Node* a_Defs);

    /// List available module types
    const std::vector<std::string> listModuleTypes () const;

    /// Builds a top-level module given its type and name
    Module* build (const std::string& a_Type, const std::string& a_Name);

protected:

    /// Creates a module
    Module* createModule (const std::string& a_Type, const std::string& a_Name,
                          const Module::Attributes& a_Attributes = Module::Attributes());

    /// Collects module attributes
    static Module::Attributes collectAttributes (const ElementTree::Node* a_Node);
    /// Collects module parameters override
    static Module::Attributes collectParameters (const ElementTree::Node* a_Node);

    /// Logger
    std::shared_ptr<spdlog::logger> m_Logger;

    /// Module creators
    Dict<std::string, CreateFunc> m_Creators;
    /// Module definitions
    Dict<std::string, std::shared_ptr<ElementTree::Node>> m_ModuleDefs;
};

// ============================================================================

}; // Graph

#endif // GRAPH_BUILDER_HH
