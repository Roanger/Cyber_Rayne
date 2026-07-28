#include "../../include/DialogueTree.h"

DialogueTree::DialogueTree() : m_rootId(-1) {}

DialogueTree::DialogueTree(int rootId) : m_rootId(rootId) {}

void DialogueTree::addNode(std::unique_ptr<DialogueNode> node) {
    if (!node) return;
    m_nodes[node->getId()] = std::move(node);
}

const DialogueNode* DialogueTree::getNode(int id) const {
    auto it = m_nodes.find(id);
    if (it == m_nodes.end()) return nullptr;
    return it->second.get();
}

DialogueNode* DialogueTree::getNode(int id) {
    auto it = m_nodes.find(id);
    if (it == m_nodes.end()) return nullptr;
    return it->second.get();
}
