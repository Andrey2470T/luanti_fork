#pragma once

#include "meshbuffer.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class Skeleton;
class BoneAnimation;
class Bone;
class LayeredMesh;

class Model
{
    std::unique_ptr<LayeredMesh> mesh;

    std::unique_ptr<Skeleton> skeleton;
    std::vector<std::unique_ptr<BoneAnimation>> animations;
    
    std::vector<std::pair<aiNode *, Bone *>> bone_mappings;
public:
    Model() = default;
    Model(render::VertexTypeDescriptor vType, const aiScene *scene);

    static Model *load(const std::string &path);

    LayeredMesh *getMesh() const
    {
        return mesh.get();
    }
private:
    void processMesh(aiMesh *m);

    void setBoneRelations(std::vector<Bone *> &bones, u8 &boneID, aiNode *curNode, Bone *parent = nullptr);
    void setBoneWeights(aiSkeleton *skeleton, aiNode *node, Bone *bone);

    void processAnimations(std::vector<Bone *> &bones, const aiScene *scene);
};
