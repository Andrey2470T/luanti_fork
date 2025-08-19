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
    BoneAnimation *animation;
    
    std::vector<std::pair<aiNode *, Bone *>> bone_mappings;
    std::vector<Bone *> bones;

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

    void processBones(const aiScene *scene);
    void processAnimations(const aiScene *scene);
};
