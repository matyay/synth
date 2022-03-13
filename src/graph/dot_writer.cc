#include "dot_writer.hh"

#include <utils/exception.hh>

#include <strutils.hh>
#include <stringf.hh>

namespace Graph {

// ============================================================================

void DotWriter::writeDot (const std::string& a_FileName) {

    // Open the file
    m_File = fopen(a_FileName.c_str(), "w");
    if (m_File == nullptr) {
        THROW(std::runtime_error, "Error opening file '%s'", a_FileName.c_str());
    }

    // Header
    write("digraph {");
    write("rankdir=LR;", 1);
    write("node [shape=record];", 1);

    // Write modules
    writeNonLeafModule(m_Root, 1);

    // Footer
    write("}");

    // Close the file
    fclose(m_File);
}

// ============================================================================

const std::string DotWriter::fixupName (const std::string& a_Name) {
    std::string fixed (a_Name);
    fixed = strutils::replace(fixed, "#", "_");
    fixed = strutils::replace(fixed, ".", "_");
    return fixed;
}

void DotWriter::write(const std::string& a_Line, size_t a_Indent) {

    // Indentation
    std::string str;
    for (size_t i=0; i<a_Indent; ++i) {
        str += " ";
    }

    // Write
    str += a_Line + "\n";
    fputs(str.c_str(), m_File);
}

void DotWriter::writeNonLeafModule (const Module* a_Module, size_t a_Level) {

    // List input and output ports
    std::vector<const Port*> inputs;
    std::vector<const Port*> outputs;

    for (const auto& it : a_Module->getPorts()) {
        const auto& port = it.second;

        if (port->getDirection() == Port::Direction::INPUT) {
            inputs.push_back(port.get());
        }
        if (port->getDirection() == Port::Direction::OUTPUT) {
            outputs.push_back(port.get());
        }
    }

    // List source and non-source modules
    std::vector<const Module*> sources;
    std::vector<const Module*> nonSources;

    for (const auto& it : a_Module->getSubmodules()) {
        const auto child = it.second;

        // A source module does not have any inputs connected
        bool isSource = true;
        for (const auto& it : child->getPorts()) {
            const auto& port = it.second;
            if (port->getDirection() == Port::Direction::INPUT) {
                if (port->isConnected()) {
                    isSource = false;
                    break;
                }
            }
        }

        // Non-leaf modules cannot be sources
        if (!child->isLeaf()) {
            isSource = false;
        }

        if (isSource) {
            sources.push_back(child.get());
        }
        else {
            nonSources.push_back(child.get());
        }
    }

    // Define cluster
    write(stringf("subgraph \"cluster_%s\" {", a_Module->getName().c_str()), a_Level);
    write(stringf("label = \"%s: \\\"%s\\\"\"",
            a_Module->getType().c_str(),
            a_Module->getName().c_str()
        ), a_Level + 1);

    // Ports / source modules
    auto writeFreePort = [&](const Port* a_Port) {
        std::string spec = stringf("%s_%s",
            fixupName(a_Module->getFullName()).c_str(),
            fixupName(a_Port->getName()).c_str()
        );

        write(stringf("%s [label=\"%s\"]",
            spec.c_str(),
            a_Port->getName().c_str()
        ), a_Level + 2);
    };

    // Input port nodes / modules
    write("subgraph {", a_Level + 1);
    write("rank=\"source\";", a_Level + 2);

    for (const auto port : inputs) {
        writeFreePort(port);
    }

    for (const auto child : sources) {
        if (child->isLeaf()) {
            writeLeafModule(child, a_Level + 2);
        }
        else {
            writeNonLeafModule(child, a_Level + 2);
        }
    }

    write("}", a_Level + 1);

    // Output port nodes
    write("subgraph {", a_Level + 1);
    write("rank=\"sink\";", a_Level + 2);
    for (const auto port : outputs) {
        writeFreePort(port);
    }
    write("}", a_Level + 1);

    // Recurse non-source child modules
    for (const auto child : nonSources) {
        if (child->isLeaf()) {
            writeLeafModule(child, a_Level + 1);
        }
        else {
            writeNonLeafModule(child, a_Level + 1);
        }
    }

    // Connections
    auto endpointSpec = [&](const Port* port) {
        char sep = port->getModule()->isLeaf() ? ':' : '_';

        return stringf("%s%c%s",
            fixupName(port->getModule()->getFullName()).c_str(),
            sep,
            fixupName(port->getName()).c_str()
        );
    };

    for (const auto& conn : a_Module->getConnections()) {
        write(stringf("%s -> %s",
            endpointSpec(conn.second).c_str(),
            endpointSpec(conn.first).c_str()),
            a_Level + 1
        );
    }

    // End cluster
    write("}", a_Level);
}

void DotWriter::writeLeafModule (const Module* a_Module, size_t a_Level) {

    // List input and output ports
    std::vector<const Port*> inputs;
    std::vector<const Port*> outputs;

    for (const auto& it : a_Module->getPorts()) {
        const auto& port = it.second;

        if (port->getDirection() == Port::Direction::INPUT) {
            inputs.push_back(port.get());
        }
        if (port->getDirection() == Port::Direction::OUTPUT) {
            outputs.push_back(port.get());
        }
    }

    // Build node label
    std::vector<std::string> labels;
    std::string label;

    labels.clear();
    for (const auto& port : inputs) {
        labels.push_back(stringf("<%s> %s",
            fixupName(port->getName()).c_str(),
            port->getName().c_str()
        ));
    }

    label += stringf("{%s}|", strutils::join("|", labels).c_str());
    labels.clear();

    for (const auto& port : outputs) {
        labels.push_back(stringf("<%s> %s",
            fixupName(port->getName()).c_str(),
            port->getName().c_str()
        ));
    }

    label += stringf("{%s}", strutils::join("|", labels).c_str());
    labels.clear();

    // Define the node
    std::string line = stringf("%s [label=\"{%s}|{\\\"%s\\\"}|{%s}\"]",
        fixupName(a_Module->getFullName()).c_str(),
        a_Module->getType().c_str(),
        a_Module->getName().c_str(),
        label.c_str()
    );
    write(line, a_Level);
}

// ============================================================================

}; // namespace Graph
