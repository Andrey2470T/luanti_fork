#pragma once

#include <BasicIncludes.h>
#include <Utils/Matrix4.h>
#include <Utils/Quaternion.h>
#include <optional>
#include <memory>

enum class TransformNodeType
{
    BONE,
    OBJECT
};

class TransformNodeTree;

struct TransformNode
{
    TransformNodeType Type;

    // Relative transforms
    v3f Position;
    Quaternion Rotation;
    v3f Scale {1.0f};

    matrix4 AbsoluteTransform;
    matrix4 RelativeTransform;

    std::optional<u8> Parent;

    std::vector<u8> Children;

    TransformNodeTree *Tree;

    TransformNode() = default;
    virtual ~TransformNode();

    bool isRoot() const
    {
        return !Parent.has_value();
    }
    TransformNode *getParent() const;

    v3f getAbsolutePosition() const;
    Quaternion getAbsoluteRotation() const;

    virtual void updateNode();

    void updateNodeAndChildren();
};

class TransformNodeTree
{
protected:
    std::vector<std::unique_ptr<TransformNode>> Nodes;

    std::vector<u8> RootNodes;

    u8 maxAcceptedNodeCount = 255u;
public:
    TransformNodeTree() = default;

    u8 getNodesCount() const
    {
        return Nodes.size();
    }

    TransformNode *getNode(u8 id) const;

    void addNode(TransformNode *newNode, std::optional<u8> parent=std::nullopt);
    void removeNode(u8 id);

    void updateNodes();
};

class TransformNodeManager
{
    std::vector<std::unique_ptr<TransformNodeTree>> Trees;
public:
    TransformNodeManager() = default;

    u8 getNodeTreeCount() const
    {
        return Trees.size();
    }
    void addNodeTree(TransformNodeTree *tree)
    {
        Trees.emplace_back(tree);
    }
    void removeNodeTree(u8 id);
    TransformNode *getNode(u8 treeID, u8 nodeID) const;
};
