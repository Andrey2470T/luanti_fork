#pragma once

#include <BasicIncludes.h>
#include <Utils/Matrix4.h>
#include <Utils/Quaternion.h>
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

struct Bone
{
    std::string Name;

    v3f Position;
    Quaternion Rotation;
    v3f Scale {1.0f};

    Bone* Parent = nullptr;
	std::vector<Bone*> Children;

	matrix4 LocalTransform;
    matrix4 GlobalTransform;

    std::array<Weight, BONE_MAX_WEIGHTS> Weights;

    std::optional<matrix4> GlobalInversedTransform;

    void buildTransform();
};

class Skeleton
{
    std::array<Bone, 128> Bones;
    std::vector<u8> ParentBones;

    MeshBuffer *AffectedMesh = nullptr;

    bool AnimateNormals;
public:
    Skeleton() = default;

    u8 getUsedBonesCount() const;

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
};
