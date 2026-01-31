#pragma once

#include <BasicIncludes.h>
#include <Utils/Matrix4.h>
#include <Utils/Quaternion.h>
#include <Render/Shader.h>
#include <list>
#include "client/ao/genericCAO.h"

#define BONES_MAX 128
#define BONE_MAX_WEIGHTS 128
#define VERTEX_MAX_BONES 8

class DataTexture;
class LayeredMesh;
struct TileLayer;

struct Weight
{
    u32 vertex_n;
    f32 strength = 0.0f;
};

struct Bone : public TransformNode
{
    std::string Name;

    std::array<Weight, BONE_MAX_WEIGHTS> Weights;
    u8 UsedWeightsCount = 0;

    // The .x and .gltf formats pre-calculate this
    std::optional<matrix4> AbsoluteInversedTransform;

    Bone()
    {
        Type = TransformNodeType::BONE;
    }
    // As the weights count per a bone has the limit, select out the most "affecting" weights from the vector
    void addWeights(std::vector<std::pair<u32, f32>> weights);

    void updateNode() override;
};

class Skeleton : public TransformNodeTree
{
    u8 BoneOffset;

    LayeredMesh *AffectedMesh = nullptr;

    DataTexture *BonesDataTexture;

    bool AnimateNormals = false;
public:
    Skeleton(DataTexture *tex, u8 boneOffset);
    ~Skeleton();

    void addBones(std::vector<TransformNode *> &bones);

    std::vector<Bone *> getAllBones() const;

    LayeredMesh *getAnimatedMesh() const
    {
        return AffectedMesh;
    }
    void setAnimatedMesh(LayeredMesh *mesh)
    {
        AffectedMesh = mesh;
    }
    void animateNormals(bool animate)
    {
        AnimateNormals = animate;
    }
    void updateTileLayer(TileLayer &layer);
    // The method updates the animate normals bool and data texture
    void updateDataTexture();
    // The bones count per a vertex also has the limit, therefore this method selects out the most "affecting" bones ids and their weights
    // This method fills "bones" and "weights" attributes
    void fillMeshAttribs(LayeredMesh *mesh);
};
