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
    
    std::vector<std::pair<aiNode *, Bone *>> bone_mappings;
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
    void processMesh(aiMesh *m);

    void setBoneRelations(std::vector<Bone *> &bones, u8 &boneID, aiNode *curNode, Bone *parent = nullptr);
    void setBoneWeights(aiSkeleton *skeleton, aiNode *node, Bone *bone);

    void processAnimations(std::vector<Bone *> &bones, const aiScene *scene);
};
