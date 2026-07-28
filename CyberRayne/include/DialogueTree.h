#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include "DialogueNode.h"

// A collection of dialogue nodes forming a conversation.
// Nodes are keyed by integer IDs. The tree has a designated root node.
class DialogueTree {
public:
    DialogueTree();
    explicit DialogueTree(int rootId);
    ~DialogueTree() = default;

    void addNode(std::unique_ptr<DialogueNode> node);
    const DialogueNode* getNode(int id) const;
    DialogueNode* getNode(int id);

    int getRootId() const { return m_rootId; }
    void setRootId(int id) { m_rootId = id; }

    bool isEmpty() const { return m_nodes.empty(); }
    std::size_t getNodeCount() const { return m_nodes.size(); }

private:
    std::unordered_map<int, std::unique_ptr<DialogueNode>> m_nodes;
    int m_rootId;
};
