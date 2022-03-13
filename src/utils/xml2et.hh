#ifndef XML_TO_ELEMENT_TREE_HH
#define XML_TO_ELEMENT_TREE_HH

#include "element_tree.hh"

// ============================================================================

/// Parses an XML file and returns its content in a form of the element tree.
std::shared_ptr<ElementTree::Node> xmlToElementTree (const std::string& a_FileName);

/// Writes an element tree to XML
void elementTreeToXml (const std::string& a_FileName, const ElementTree::Node* a_Node);

// ============================================================================

#endif // XML_TO_ELEMENT_TREE_HH

