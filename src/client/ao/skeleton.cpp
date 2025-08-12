#include "skeleton.h"
#include "client/render/datatexture.h"

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

void Bone::addWeights(std::vector<std::pair<u32, f32> > weights)
{
    u8 accepedWCount = std::min<u8>(weights.size(), BONE_MAX_WEIGHTS-UsedWeightsCount);

    if (accepedWCount == 0)
        return;

    // At first adds the most affecting weights
    std::sort(weights.begin(), weights.end(), [] (const std::pair<u32, f32> &w1, const std::pair<u32, f32> &w2)
    {
        return w1.second > w2.second;
    });

    for (u8 i = 0; i < accepedWCount; i++) {
        Weights[UsedWeightsCount+i].vertex_n = weights.at(i).first;
        Weights[UsedWeightsCount+i].strength = weights.at(i).second;
    }

    UsedWeightsCount += accepedWCount;
}

void Bone::updateBone()
{
    Transform.buildTransform(!isRoot() ? &Parent->Transform : nullptr);

    for (auto &childBone : Children)
        childBone->updateBone();
}

Skeleton::Skeleton()
    : BonesDataTexture(std::make_unique<DataTexture>("BonesData", 4 * 4 * sizeof(f32), 0, 4 * 4))
{

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

        ByteArray bone_arr(4 * 4, 4 * 4 * sizeof(f32));
        bone_arr.setM4x4(Bones.at(i)->Transform.GlobalTransform, 0);

        BonesDataTexture->addSample(bone_arr);
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

void Skeleton::updateShaderAndDataTexture(render::Shader *shader)
{
    shader->setUniformInt("mAnimateNormals", (s32)AnimateNormals);

    for (u8 i = 0; i < UsedBonesCount; i++) {
        ByteArray bone_arr(4 * 4, 4 * 4 * sizeof(f32));
        bone_arr.setM4x4(Bones.at(i)->Transform.GlobalTransform, 0);

        BonesDataTexture->updateSample(i, bone_arr);
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

        mesh->setAttrAt<v2i>(v2i(bones_ids[0], bones_ids[1]), 5, i);
        mesh->setAttrAt<v2i>(v2i(weights[0], weights[1]), 6, i);
    }
}
