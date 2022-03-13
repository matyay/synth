#ifndef ELEMENT_PARSING_HH
#define ELEMENT_PARSING_HH

#include "element_tree.hh"

#include <unordered_set>

namespace ElementTree {

// ============================================================================

typedef std::unordered_set<std::string> ItemSet;

// ============================================================================

/// Check if there are no children
void expectNoChildren (const Node* a_Node);

/// Checks if the tag is present
void expectTag        (const Node* a_Node, const std::string& a_Tag);

/// Checks if all listed tags are present
void expectAllTags    (const Node* a_Node, const ItemSet& a_Tags);

/// Checks whether only listed tags are present
void expectOnlyOfTags (const Node* a_Node, const ItemSet& a_Tags);

// ============================================================================

/// Check if there are no attributes
void expectNoAttributes      (const Node* a_Node);

/// Checks if the attribute is present
void expectAttribute         (const Node* a_Node, const std::string& a_Name);

/// Checks if all listed attributes are present
void expectAllAttributes     (const Node* a_Node, const ItemSet& a_Names);

/// Checks whether only listed attributes are present
void expectOnlyOfAttributess (const Node* a_Node, const ItemSet& a_Names);

// ============================================================================

}; // ElementTree

#endif // ELEMENT_PARSING_HH
