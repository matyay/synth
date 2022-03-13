#include "xml2et.hh"

#include <libxml/parser.h>
#include <libxml/xinclude.h>
#include <libxml/tree.h>

#include <fstream>
#include <algorithm>
#include <stdexcept>

using namespace ElementTree;

// ============================================================================

static std::shared_ptr<Node> xmlToNode (xmlNode* a_Node) {

    // Get attributes
    Node::Attributes attributes;
    for(xmlAttrPtr xmlAttr = a_Node->properties; xmlAttr; xmlAttr = xmlAttr->next) {
        std::string name  = (const char*)xmlAttr->name;
        xmlChar*    value = xmlNodeListGetString(a_Node->doc, xmlAttr->children, 1);

        attributes[name] = std::string((const char*)value);
        xmlFree(value);
    }

    // Get text
    std::string text;
    if (!xmlIsBlankNode(a_Node->children)) {
        xmlChar* xmlText = xmlNodeListGetString(a_Node->doc, a_Node->children, 1);
        if (xmlText != nullptr) {
            text = std::string((const char*)xmlText);
            xmlFree(xmlText);
        }
    }

    // Create the element tree node
    std::shared_ptr<Node> node(new Node(
        (const char*)a_Node->name,
        text,
        attributes
    ));

    // Get and add children recursively
    for (auto xmlChild = a_Node->children; xmlChild; xmlChild = xmlChild->next) {
        if (xmlChild->type == XML_ELEMENT_NODE) {
            Node::addChild(node, xmlToNode(xmlChild));
        }
    }

    return node;
}

std::shared_ptr<Node> xmlToElementTree (const std::string& a_FileName) {

    // Test libxml version
    LIBXML_TEST_VERSION;

    // Read the XML
    xmlDocPtr xml = xmlReadFile(a_FileName.c_str(), nullptr, 0);
    if (xml == nullptr) {
        throw std::runtime_error(
            "Error reading/parsing XML file '" + a_FileName + "'"
        );
    }

    // Apply includes (if any)
    if (xmlXIncludeProcess(xml) < 0) {
        xmlFreeDoc(xml);
        xmlCleanupParser();

        throw std::runtime_error(
            "Error processing xi:include in XML file '" + a_FileName + "'"
        );
    }

    // Find root
    auto xmlRoot = xmlDocGetRootElement(xml);
    if (xmlRoot == nullptr) {
        xmlFreeDoc(xml);
        xmlCleanupParser();

        throw std::runtime_error(
            "Failed to find root element in XML file '" + a_FileName + "'"
        );
    }

    // Convert to element tree
    auto root = xmlToNode(xmlRoot);

    // Cleanup the XML parser
    xmlFreeDoc(xml);
    xmlCleanupParser();

    // Return the root
    return root;
}

// ============================================================================


static void nodeToXml (
    std::ofstream& fp, const ElementTree::Node* a_Node, size_t a_Level = 0
) {

    std::string indent(a_Level, ' ');

    // Tag opening
    fp << indent + "<" + a_Node->getTag();

    // Attributes
    std::string attributes;
    std::vector<std::string> keys;

    for (auto& it : a_Node->getAttributes()) {
        keys.push_back(it.first);
    }
    std::sort(keys.begin(), keys.end());
    
    for (auto& key : keys) {
        const std::string& val = a_Node->getAttribute(key);
        attributes += " " + key + "=\"" + val + "\"";
    }

    fp << attributes;

    // No children and no text
    if (!a_Node->hasChildren() && !a_Node->hasText()) {
        fp << "/>\n";
        return;
    }

    fp << ">\n";

    // Write text
    if (a_Node->hasText()) {
        fp << a_Node->getText() + "\n";
    }

    // Write children
    for (auto& child : a_Node->getChildren()) {
        nodeToXml(fp, child.get(), a_Level + 1);
    }

    // Tag closing
    fp << indent + "</" + a_Node->getTag() + ">\n";
}

void elementTreeToXml (const std::string& a_FileName, const ElementTree::Node* a_Node) {

    // Open the file
    std::ofstream fp(a_FileName);

    // Write XML header
    fp << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    // Write XML content
    nodeToXml(fp, a_Node);
}

