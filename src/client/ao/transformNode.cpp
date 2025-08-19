#include "transformNode.h"

TransformNode::~TransformNode()
{
    for (u8 child : Children)
        Tree->removeNode(child);
}

TransformNode *TransformNode::getParent() const
{
    if (!Parent.has_value())
        return nullptr;
    return Tree->getNode(Parent.value());
}

v3f TransformNode::getAbsolutePosition() const
{
    v3f abs_pos = Position;

    auto parent = getParent();

    if (parent)
        abs_pos += parent->getAbsolutePosition();

    return abs_pos;
}
Quaternion TransformNode::getAbsoluteRotation() const
{
    Quaternion abs_rot = Rotation;

    auto parent = getParent();

    if (parent)
        abs_rot *= parent->getAbsoluteRotation();

    return abs_rot;
}

void TransformNode::updateNode()
{
    matrix4 T;
    T.setTranslation(Position);
    matrix4 R;
    Rotation.getMatrix_transposed(R);
    matrix4 S;
    S.setScale(Scale);

    RelativeTransform = T * R * S;

    auto parent = getParent();
    if (parent)
        AbsoluteTransform = parent->AbsoluteTransform * RelativeTransform;
    else
        AbsoluteTransform = RelativeTransform;
}

void TransformNode::updateNodeAndChildren()
{
    updateNode();

    for (auto child : Children)
        Tree->getNode(child)->updateNodeAndChildren();
}

TransformNode *TransformNodeTree::getNode(u8 id) const
{
    assert(id < Nodes.size());
    return Nodes.at(id).get();
}

void TransformNodeTree::addNode(TransformNode *newNode, std::optional<u8> parent)
{
    assert(Nodes.size() + 1 <= maxAcceptedNodeCount);

    newNode->Tree = this;
    Nodes.emplace_back(newNode);

    if (!parent.has_value())
        RootNodes.push_back(Nodes.size()-1);
    else {
        newNode->Parent = parent.value();

        auto parent_node = getNode(parent.value());
        parent_node->Children.push_back(Nodes.size()-1);
    }
}

void TransformNodeTree::removeNode(u8 id)
{
    assert(id < Nodes.size());

    auto node_it = Nodes.begin()+id;
    auto node = node_it->get();

    if (node->isRoot())
        RootNodes.erase(std::find(RootNodes.begin(), RootNodes.end(), id));
    else {
        auto parent_node = node->getParent();
        parent_node->Children.erase(parent_node->Children.begin()+id);
    }

    for (u8 child_id : node->Children)
        removeNode(child_id);
    Nodes.erase(node_it);
}

void TransformNodeTree::updateNodes()
{
    for (u8 root : RootNodes)
        getNode(root)->updateNodeAndChildren();
}

void TransformNodeManager::addNodeTree(TransformNodeTree *tree)
{
    auto found = std::find_if(Trees.begin(), Trees.end(),
    [tree] (const std::unique_ptr<TransformNodeTree> &cur_tree)
    {
        return tree == cur_tree.get();
    });

    if (found == Trees.end())
        Trees.emplace_back(tree);
}

void TransformNodeManager::removeNodeTree(u8 id)
{
    assert(id < Trees.size());
    Trees.erase(Trees.begin()+id);
}

TransformNode *TransformNodeManager::getNode(u8 treeID, u8 nodeID) const
{
    assert(treeID < Trees.size());
    return Trees.at(treeID)->getNode(nodeID);
}
