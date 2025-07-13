#pragma once

#include <BasicIncludes.h>
#include <Utils/Matrix4.h>
#include <Utils/Quaternion.h>
#include <Render/Shader.h>
#include "client/render/meshbuffer.h"
#include <list>

#define BONES_MAX 128
#define BONE_MAX_WEIGHTS 128
#define VERTEX_MAX_BONES 8

struct Weight
{
    u32 vertex_n;
    f32 strength = 0.0f;
};

struct BoneTransform
{
    v3f Position;
    Quaternion Rotation;
    v3f Scale {1.0f};

    matrix4 GlobalTransform;
    // The .x and .gltf formats pre-calculate this
    std::optional<matrix4> GlobalInversedTransform;

    void buildTransform(const BoneTransform *parentTransform=nullptr);
};

struct Bone
{
    std::string Name;

    BoneTransform Transform;

    Bone* Parent = nullptr;
	std::vector<Bone*> Children;

    std::array<Weight, BONE_MAX_WEIGHTS> Weights;
    u8 UsedWeightsCount = 0;

    bool isRoot() const
    {
        return !Parent;
    }
    // As the weights count per a bone has the limit, select out the most "affecting" weights from the vector
    void addWeights(const std::vector<std::pair<u32, f32>> &weights);

    void updateBone();
};

class Skeleton
{
    std::array<std::unique_ptr<Bone>, BONES_MAX> Bones;
    u8 UsedBonesCount = 0;

    std::vector<u8> RootBones;

    MeshBuffer *AffectedMesh = nullptr;

    bool AnimateNormals = false;
public:
    Skeleton() = default;

    u8 getUsedBonesCount() const
    {
        return UsedBonesCount;
    }

    Bone *getBone(u8 n) const;
    void addBones(std::vector<Bone *> &bones);

    std::vector<Bone *> getAllUsedBones() const;

    MeshBuffer *getAnimatedMesh() const
    {
        return AffectedMesh;
    }
    void setAnimatedMesh(MeshBuffer *mesh)
    {
        AffectedMesh = mesh;
    }
    void animateNormals(bool animate)
    {
        AnimateNormals = animate;
    }
    void updateBonesTransforms();
    // The method updates the "mAnimateNormals" and "mBonesTransforms" uniform mat4 array
    void updateObjectShader(render::Shader *shader);
    // The bones count per a vertex also has the limit, therefore this method selects out the most "affecting" bones ids and their weights
    // This method fills "bones" and "weights" attributes
    void fillMeshAttribs(MeshBuffer *mesh);
};
