#include "element_tree.hh"

namespace ElementTree {

// ============================================================================

Node::Node (const std::string a_Tag, const Attributes& a_Attributes) :
    m_Tag        (a_Tag),
    m_Attributes (a_Attributes)
{
    // Empty
}

Node::Node (const std::string a_Tag, const std::string& a_Text,
            const Attributes& a_Attributes) :
    m_Tag        (a_Tag),
    m_Text       (a_Text),
    m_Attributes (a_Attributes)
{
    // Empty
}

// ============================================================================

const std::shared_ptr<Node> Node::getParent () const {
    return m_Parent.lock();
}

std::shared_ptr<Node> Node::getParent () {
    return m_Parent.lock();
}

// ============================================================================

std::string Node::getTag () const {
    return m_Tag;
}

// ============================================================================

bool Node::hasText () const {
    return !m_Text.empty();
}

std::string Node::getText () const {
    return m_Text;
}


void Node::setText (const std::string& a_Text) {
    m_Text = a_Text;
}

// ============================================================================

bool Node::hasAttributes () const {
    return !m_Attributes.empty();
}


const Node::Attributes& Node::getAttributes () const {
    return m_Attributes;
}

Node::Attributes& Node::getAttributes () {
    const Node& constThis = *this;
    return const_cast<Attributes&>(constThis.getAttributes());
}


bool Node::hasAttribute (const std::string& a_Name) const {
    return m_Attributes.count(a_Name) != 0;
}

std::string Node::getAttribute(const std::string& a_Name,
                               const std::string& a_Default) const
{
    if (hasAttribute(a_Name)) {
        return m_Attributes.at(a_Name);
    }
    else {
        return a_Default;
    }
}

void Node::setAttribute(const std::string& a_Name,
                        const std::string& a_Value)
{
    m_Attributes[a_Name] = a_Value;
}

// ============================================================================

bool Node::hasChildren () const {
    return !m_Children.empty();
}

const std::vector<std::shared_ptr<Node>> Node::getChildren () const {
    return m_Children;
}

std::vector<std::shared_ptr<Node>> Node::getChildren () {
    return m_Children;
}


void Node::addChild (std::shared_ptr<Node> a_Parent,
                     std::shared_ptr<Node> a_Node)
{

    // If the node has a parent then detach from it
    auto parent = a_Node->m_Parent.lock();
    if (parent) {
        parent->removeChild(a_Node);
    }

    // TODO: Check for loop

    // Set parent and add the child
    a_Node->m_Parent = a_Parent;
    a_Parent->m_Children.push_back(a_Node);
}

void Node::removeChild (std::shared_ptr<Node> a_Node) {

    // Find it in the children list and remove it
    for (auto it = m_Children.begin(); it != m_Children.end(); it++) {
        if (*it == a_Node) {
            a_Node->m_Parent.reset();
            m_Children.erase(it);
            return;
        }
    }

    // The child is not of this node
    throw std::runtime_error(
        "The given child node is not on the children list!"
    );
}

// ============================================================================

const std::shared_ptr<Node> Node::find (const std::string& a_Tag) const {

    for (auto& node : m_Children) {
        if (node->getTag() == a_Tag) {
            return node;
        }
    }

    return std::shared_ptr<Node>();
}

std::shared_ptr<Node> Node::find (const std::string& a_Tag) {
    const Node& constThis = *this;
    return constThis.find(a_Tag);
}


const std::vector<std::shared_ptr<Node>> Node::findAll (
    const std::string& a_Tag) const
{
    std::vector<std::shared_ptr<Node>> nodes;

    for (auto& node : m_Children) {
        if (node->getTag() == a_Tag) {
            nodes.push_back(node);
        }
    }

    return nodes;
}

std::vector<std::shared_ptr<Node>> Node::findAll (const std::string& a_Tag) {
    const Node& constThis = *this;
    return constThis.findAll(a_Tag);
}

// ============================================================================

std::string Node::getPath() const {
    std::string path = m_Tag;

    // Go to the root and assemble the path
    for (auto node=getParent(); node; node=node->getParent()) {
        path = node->getTag() + "/" + path;
    }

    return path;
}

// ============================================================================

const std::vector<std::string> dump (const Node* a_Root, size_t a_Indent) {
    std::vector<std::string> lines;

    // Indentation string
    std::string indent;
    indent.resize(a_Indent, ' ');

    // Tag
    lines.push_back(indent + "Tag: '" + a_Root->getTag() + "'");

    // Attributes
    auto& attributes = a_Root->getAttributes();
    for (auto& it : attributes) {
        lines.push_back(indent + " '" + it.first + "' = '" + it.second + "'");
    }

    // Text
    auto text = a_Root->getText();
    if (text.length()) {
        lines.push_back(indent + " '" + text + "'");
    }

    // Children nodes
    for (auto& node : a_Root->getChildren()) {
        auto v = dump(node.get(), a_Indent + 1);
        lines.insert(lines.end(), v.begin(), v.end());
    }
    
    return lines;
}

// ============================================================================

}; // ElementTree
