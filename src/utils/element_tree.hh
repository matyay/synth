#ifndef ELEMENT_TREE_HH
#define ELEMENT_TREE_HH

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>

namespace ElementTree {

// ============================================================================

class Node {
public:

    /// Node attributes type
    typedef std::unordered_map<std::string, std::string> Attributes;

    // ....................................................

    /// Constructor
    Node (const std::string a_Tag,
          const Attributes& a_Attributes = Attributes());
    /// Constructor
    Node (const std::string a_Tag, const std::string& a_Text,
          const Attributes& a_Attributes = Attributes());

    // ....................................................

    /// Returns the parent
    const std::shared_ptr<Node> getParent () const;
    /// Returns the parent
    std::shared_ptr<Node> getParent ();

    // ....................................................

    /// Returns tag
    std::string getTag () const;

    // ....................................................

    /// Returns true when the node has text
    bool hasText () const;

    /// Returns text
    std::string getText () const;
    /// Sets node text
    void setText (const std::string& a_Text);

    // ....................................................

    /// Returns true when the node has attributes
    bool hasAttributes () const;
    /// Returns true if the given attribute is present
    bool hasAttribute (const std::string& a_Name) const;

    /// Returns attributes
    const Attributes& getAttributes () const;
    /// Returns attributes
    Attributes& getAttributes ();

    /// Returns the attribute or a default value if it is not present
    std::string getAttribute(
        const std::string& a_Name,
        const std::string& a_Default = std::string()) const;

    /// Sets the given attribute
    void setAttribute(const std::string& a_Name, const std::string& a_Value);

    // ....................................................

    /// Returns true when the node has children
    bool hasChildren () const;

    /// Returns children nodes
    const std::vector<std::shared_ptr<Node>> getChildren () const;
    /// Returns children nodes
    std::vector<std::shared_ptr<Node>> getChildren ();

    // ....................................................

    /// Attaches a new child node
    static void addChild (std::shared_ptr<Node> a_Parent,
                          std::shared_ptr<Node> a_Node);

    /// Removes (detaches) a child node
    void removeChild (std::shared_ptr<Node> a_Node);

    // ....................................................

    /// Finds the first child node with the given tag
    const std::shared_ptr<Node> find (const std::string& a_Tag) const;
    /// Finds the first child node with the given tag
    std::shared_ptr<Node> find (const std::string& a_Tag);

    /// Finds all child nodes with matching tags
    const std::vector<std::shared_ptr<Node>> findAll (
        const std::string& a_Tag) const;

    /// Finds all child nodes with matching tags
    std::vector<std::shared_ptr<Node>> findAll (const std::string& a_Tag);

    // ....................................................

    /// Returns full path to the node in the tree hierarchy
    std::string getPath () const;

    // ....................................................

protected:

    /// Parent
    std::weak_ptr<Node> m_Parent;

    /// Tag
    const std::string m_Tag;
    /// Text
    std::string m_Text;
    /// Attributes
    Attributes m_Attributes;

    /// Child nodes
    std::vector<std::shared_ptr<Node>> m_Children;
};

// ============================================================================

/// Dumps the element tree to a vector of text lines.
const std::vector<std::string> dump (const Node* a_Root, size_t a_Indent = 0);

// ============================================================================

}; // ElementTree

#endif // ELEMENT_TREE_HH
