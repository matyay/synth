#include "element_parsing.hh"

#include <stdexcept>

namespace ElementTree {

// ============================================================================

void expectNoChildren (const Node* a_Node) {

    if (a_Node->hasChildren()) {

        // Format message
        std::string msg = "The element '" + a_Node->getPath() + \
                          "' must have no children";
        // Throw
        throw std::runtime_error(msg);
    }
}

void expectTag (const Node* a_Node, const std::string& a_Tag) {

    if (!a_Node->find(a_Tag)) {

        // Format message
        std::string msg = "The element '" + a_Node->getPath() + \
                          "' must have a '" + a_Tag + "' child";

        // Throw
        throw std::runtime_error(msg);
    }
}


void expectAllTags (const Node* a_Node, const ItemSet& a_Tags) {

    // Look for all tags from the set
    for (const auto& tag : a_Tags) {

        // A tag was not found
        if (!a_Node->find(tag)) {

            // Format message
            std::string msg = "The element '" + a_Node->getPath() + \
                              "' is missing the '" + tag + \
                              "' child. It must have all of:";

            for (const auto& t : a_Tags) {
                msg += " '" + t + "'";
            }

            msg += " tags";

            // Throw
            throw std::runtime_error(msg);
        }
    }
}


void expectOnlyOfTags (const Node* a_Node, const ItemSet& a_Tags) {

    // Scan all children
    for (auto node : a_Node->getChildren()) {

        // An unexpected tag
        if (!a_Tags.count(node->getTag())) {

            // Format message
            std::string msg = "Unexpected child '" + node->getTag() + \
                              "' of element '" + a_Node->getPath() + \
                              "'. Expected only:";

            for (const auto& t : a_Tags) {
                msg += " '" + t + "'";
            }

            // Throw
            throw std::runtime_error(msg);
        }
    }
}

// ============================================================================


void expectNoAttributes (const Node* a_Node) {

    if (a_Node->hasAttributes()) {

        // Format message
        std::string msg = "The element '" + a_Node->getPath() + \
                          "' must have no attributes";
        // Throw
        throw std::runtime_error(msg);
    }
}

void expectAttribute (const Node* a_Node, const std::string& a_Name) {

    if (!a_Node->hasAttribute(a_Name)) {

        // Format message
        std::string msg = "The element '" + a_Node->getPath() + \
                          "' must have the '" + a_Name + "' attribute";

        // Throw
        throw std::runtime_error(msg);
    }
}

void expectAllAttributes (const Node* a_Node, const ItemSet& a_Names) {

    // Look for all attributes from the set
    for (const auto& name : a_Names) {

        // A tag was not found
        if (!a_Node->hasAttribute(name)) {

            // Format message
            std::string msg = "The element '" + a_Node->getPath() + \
                              "' is missing the '" + name + \
                              "' attribute. It must have all of:";

            for (const auto& n : a_Names) {
                msg += " '" + n + "'";
            }

            msg += " attributes";

            // Throw
            throw std::runtime_error(msg);
        }
    }
}

void expectOnlyOfAttributes (const Node* a_Node, const ItemSet& a_Names) {

    // Scan all attributes
    for (const auto& it : a_Node->getAttributes()) {
        auto name = it.first;

        // An unexpected attribute
        if (!a_Names.count(name)) {

            // Format message
            std::string msg = "Unexpected attribute '" + name + \
                              "' of element '" + a_Node->getPath() + \
                              "'. Expected only:";

            for (const auto& n : a_Names) {
                msg += " '" + n + "'";
            }

            // Throw
            throw std::runtime_error(msg);
        }
    }
}

// ============================================================================

}; // ElementTree
