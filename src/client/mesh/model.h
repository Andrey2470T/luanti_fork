#pragma once

#include "meshbuffer.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class Skeleton;
class BoneAnimation;
class Bone;

struct MeshPart {
    u32 offset = 0;
    u32 count;
};

class Model
{
    std::unique_ptr<MeshBuffer> mesh;
    std::vector<MeshPart> parts;

    std::unique_ptr<Skeleton> skeleton;
    std::vector<std::unique_ptr<BoneAnimation>> animations;
public:
    Model() = default;
    Model(const aiScene *scene);

    static Model *load(const std::string &path);

    u8 getMeshPartCount() const
    {
        return parts.size();
    }
    MeshPart getMeshPart(u8 n) const;

    MeshBuffer *getMesh() const
    {
        return mesh.get();
    }
private:
    static void processNode(aiNode *node, aiScene *scene);
    void processMesh(aiMesh *mesh, aiScene *scene);

    void setBoneRelations(std::vector<Bone *> &bones, u8 &boneID, aiNode *curNode, Bone *parent = nullptr);
    Bone *processBone(std::vector<Bone *> &bones, aiSkeletonBone *bone, const aiScene *scene, Bone *parent = nullptr);
};
