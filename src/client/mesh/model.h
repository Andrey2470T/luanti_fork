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
    Model(AnimationManager *_mgr);
    Model(v3f pos, const std::vector<MeshLayer> &layers, MeshBuffer *buffer);
    Model(AnimationManager *_mgr, const aiScene *scene);

    static Model *load(AnimationManager *_mgr, const std::string &name);
    static Model *loadFromMem(AnimationManager *_mgr, void *mem, s32 size, const std::string &format);

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
    void processMesh(u8 mat_i, const std::vector<aiMesh *> &meshes);

    void setBoneRelations(std::vector<Bone *> &bones, u8 &boneID, aiNode *curNode, std::optional<u8> parentID = std::nullopt);
    void setBoneWeights(aiSkeleton *skeleton, aiNode *node, Bone *bone);

    void processBones(const aiScene *scene);
    void processAnimations(const aiScene *scene);
};
