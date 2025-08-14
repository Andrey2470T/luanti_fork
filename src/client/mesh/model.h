#pragma once

#include "meshbuffer.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class Skeleton;
class BoneAnimation;
class Bone;
class LayeredMesh;
class AnimationManager;
class ResourceCache;

class Model
{
    std::unique_ptr<LayeredMesh> mesh;

    Skeleton *skeleton;
    std::vector<BoneAnimation *> animations;
    
    std::vector<std::pair<aiNode *, Bone *>> bone_mappings;

    AnimationManager *mgr;
public:
    Model(AnimationManager *_mgr)
        : mgr(_mgr)
    {}
    Model(AnimationManager *_mgr, v3f pos, const aiScene *scene, ResourceCache *cache);

    static Model *load(AnimationManager *_mgr, v3f pos, const std::string &path, ResourceCache *cache);

    LayeredMesh *getMesh() const
    {
        return mesh.get();
    }
private:
    void processMesh(aiMesh *m);

    void setBoneRelations(std::vector<Bone *> &bones, u8 &boneID, aiNode *curNode, std::optional<u8> parentID = std::nullopt);
    void setBoneWeights(aiSkeleton *skeleton, aiNode *node, Bone *bone);

    void processAnimations(std::vector<Bone *> &bones, const aiScene *scene);
};
