#ifndef GRAPH_DOT_WRITER_HH
#define GRAPH_DOT_WRITER_HH

#include "module.hh"

#include <string>
#include <cstdio>

namespace Graph {

// ============================================================================

class DotWriter {
public:

    /// Constructor
    DotWriter (Module* a_Module) :
        m_Root (a_Module)
    {};

    /// Writes the module hierarchy and connection to graphviz dot format
    void writeDot (const std::string& a_FileName);

protected:

    /// Fixes a module name for graphviz
    static const std::string fixupName (const std::string& a_Name);
    /// Writes an indented line
    void write(const std::string& a_Line, size_t a_Indent = 0);

    /// Writes a non-leaf module
    void writeNonLeafModule (const Module* a_Module, size_t a_Level = 0);
    /// Writes a leaf module
    void writeLeafModule (const Module* a_Module, size_t a_Level = 0);

    /// Root module
    const Module* m_Root;
    /// File
    FILE* m_File = nullptr;
};

}; // namespace Graph

#endif // GRAPH_DOT_WRITER_HH
