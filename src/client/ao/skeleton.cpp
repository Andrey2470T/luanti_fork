#include "skeleton.h"

void BoneTransform::buildTransform(const BoneTransform *parentTransform)
{
    matrix4 T;
    T.setTranslation(Position);
    matrix4 R;
    Rotation.getMatrix_transposed(R);
    matrix4 S;
    S.setScale(Scale);

    GlobalTransform = T * R * S;

    if (parentTransform)
        GlobalTransform = parentTransform->GlobalTransform * GlobalTransform;

    if (!GlobalInversedTransform) {
        GlobalInversedTransform = GlobalTransform;
        GlobalInversedTransform->makeInverse();
    }
}

void Bone::addWeights(const std::vector<std::pair<u32, f32>> &weights)
{
    assert(UsedWeightsCount + weights.size() <= BONE_MAX_WEIGHTS);

    for (u8 i = 0; i < weights.size(); i++) {
        Weights[UsedWeightsCount+i].vertex_n = weights.at(i).first;
        Weights[UsedWeightsCount+i].strength = weights.at(i).second;
    }

    UsedWeightsCount += weights.size();
}

void Bone::updateBone()
{
    Transform.buildTransform(!isRoot() ? &Parent->Transform : nullptr);

    for (auto &childBone : Children)
        childBone->updateBone();
}

Bone *Skeleton::getBone(u8 n) const
{
    assert(n < UsedBonesCount);

    return Bones.at(n).get();
}

void Skeleton::addBones(std::vector<Bone *> &bones)
{
    assert(UsedBonesCount + bones.size() <= BONES_MAX);

    for (u8 i = 0; i < bones.size(); i++) {
        auto bone = bones.at(i);
        Bones[UsedBonesCount+i] = std::unique_ptr<Bone>(bone);

        if (bone->isRoot())
            RootBones.push_back(UsedBonesCount+i);
    }

    UsedBonesCount += bones.size();
}

std::vector<Bone *> Skeleton::getAllUsedBones() const
{
    std::vector<Bone *> usedBones(Bones.size());

    for (u8 i = 0; i < Bones.size(); i++)
        usedBones[i] = Bones.at(i).get();

    return usedBones;
}

void Skeleton::updateBonesTransforms()
{
    // Traverse through the bones tree starting from the root ones

    for (u8 root_id : RootBones)
        Bones.at(root_id)->updateBone();
}

void Skeleton::updateObjectShader(render::Shader *shader)
{
    shader->setUniformInt("mAnimateNormals", (s32)AnimateNormals);

    for (u8 i = 0; i < UsedBonesCount; i++) {
        std::string boneTName = "mBonesTransforms[";
        boneTName += i;
        boneTName += "]";
        shader->setUniform4x4Matrix(boneTName, Bones.at(i)->Transform.GlobalTransform);
    }
}

void Skeleton::fillMeshAttribs(MeshBuffer *mesh)
{
    assert(mesh->getVAO()->getVertexType().Name != "AnimatedObject3D");

    for (u32 i = 0; i < mesh->getVertexCount(); i++) {
        u8 bones_packed = 0;
        u8 weights_packed = 0;
        std::array<u32, 2> bones_ids;
        std::array<u32, 2> weights;

        for (u8 j = 0; j < Bones.size(); j++) {
            for (auto w : Bones.at(j)->Weights) {
                if (w.vertex_n != i)
                    continue;

                if (weights_packed == 8)
                    break;

                bones_ids[bones_packed / 4] <<= bones_packed % 4 * 8;
                bones_ids[bones_packed / 4] |= j;

                weights[weights_packed / 4] <<= weights_packed % 4 * 8;
                weights[weights_packed / 4] |= (u8)w.strength;

                bones_packed++;
                weights_packed++;
            }

            if (bones_packed == 8)
                break;
        }

        mesh->setAttrAt(bones_ids);
    }
}
