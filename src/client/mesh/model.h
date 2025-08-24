#pragma once

#include "meshbuffer.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class Skeleton;
class BoneAnimation;
class Bone;
class LayeredMesh;
class LayeredMeshPart;
class AnimationManager;
class ResourceCache;
class TileLayer;

typedef std::pair<std::shared_ptr<TileLayer>, LayeredMeshPart> MeshLayer;

class Model
{
    std::unique_ptr<LayeredMesh> mesh;

    Skeleton *skeleton = nullptr;
    BoneAnimation *animation = nullptr;
    
    std::vector<std::pair<aiNode *, Bone *>> bone_mappings;
    std::vector<Bone *> bones;

    AnimationManager *mgr;
public:
    Model(AnimationManager *_mgr)
        : mgr(_mgr)
    {}
    Model(v3f pos, const std::vector<MeshLayer> &layers, MeshBuffer *buffer);
    Model(AnimationManager *_mgr, v3f pos, const std::vector<std::shared_ptr<TileLayer>> &layers,
        const aiScene *scene);

    static Model *load(AnimationManager *_mgr, v3f pos, const std::vector<std::shared_ptr<TileLayer>> &layers,
        const std::string &name, ResourceCache *cache);

    LayeredMesh *getMesh() const
    {
        return mesh.get();
    }

    Skeleton *getSkeleton() const
    {
        return skeleton;
    }

    BoneAnimation *getAnimation() const
    {
        return animation;
    }
private:
    void processMesh(u8 mat_i, aiMesh *m, std::shared_ptr<TileLayer> layer);

    void setBoneRelations(std::vector<Bone *> &bones, u8 &boneID, aiNode *curNode, std::optional<u8> parentID = std::nullopt);
    void setBoneWeights(aiSkeleton *skeleton, aiNode *node, Bone *bone);

    void processBones(const aiScene *scene);
    void processAnimations(const aiScene *scene);
};
