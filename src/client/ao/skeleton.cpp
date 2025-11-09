#include "skeleton.h"
#include "client/render/datatexture.h"
#include "client/mesh/layeredmesh.h"
#include "client/mesh/meshbuffer.h"
#include "client/render/tilelayer.h"


void Bone::addWeights(std::vector<std::pair<u32, f32> > weights)
{
    u8 acceptedWCount = std::min<u8>(weights.size(), BONE_MAX_WEIGHTS-UsedWeightsCount);

    if (acceptedWCount == 0)
        return;

    // At first adds the most affecting weights
    std::sort(weights.begin(), weights.end(), [] (const std::pair<u32, f32> &w1, const std::pair<u32, f32> &w2)
    {
        return w1.second > w2.second;
    });

    for (u8 i = 0; i < acceptedWCount; i++) {
        Weights[UsedWeightsCount+i].vertex_n = weights.at(i).first;
        Weights[UsedWeightsCount+i].strength = weights.at(i).second;
    }

    UsedWeightsCount += acceptedWCount;
}

void Bone::updateNode()
{
    TransformNode::updateNode();

    if (!AbsoluteInversedTransform) {
        AbsoluteInversedTransform = AbsoluteTransform;
        AbsoluteInversedTransform->makeInverse();
    }
}

Skeleton::Skeleton(DataTexture *tex, u8 boneOffset)
    : BoneOffset(boneOffset), BonesDataTexture(tex)
{
    maxAcceptedNodeCount = BONES_MAX;
}

Skeleton::~Skeleton()
{
    for (u8 i = 0; i < getNodesCount(); i++)
        BonesDataTexture->removeSample(BoneOffset+i);
}

void Skeleton::addBones(std::vector<TransformNode *> &bones)
{
    for (u8 i = 0; i < bones.size(); i++) {
        auto bone = bones.at(i);
        addNode(bone);

        ByteArray bone_arr({"", {{"", ByteArrayElementType::MAT4}}}, 1);
        bone_arr.setM4x4(bone->AbsoluteTransform, 0, 0);
        BonesDataTexture->addSample(bone_arr);
    }
}

std::vector<Bone *> Skeleton::getAllBones() const
{
    std::vector<Bone *> bones(Nodes.size());

    for (u8 i = 0; i < Nodes.size(); i++) {
        auto node = getNode(i);

        // NOTE: skeletons can save not just bones, but the object attachments when those are attached to them
        if (node->Type == TransformNodeType::BONE)
            bones[i] = dynamic_cast<Bone *>(node);
    }

    return bones;
}

void Skeleton::updateTileLayer(std::shared_ptr<TileLayer> layer)
{
    layer->bone_offset = BoneOffset;
    layer->animate_normals = (s32)AnimateNormals;
}

void Skeleton::updateDataTexture()
{
    for (u8 i = 0; i < getNodesCount(); i++) {
        ByteArray bone_arr({"", {{"", ByteArrayElementType::MAT4}}}, 1);
        bone_arr.setM4x4(getNode(i)->AbsoluteTransform, 0, 0);

        BonesDataTexture->updateSample(BoneOffset+i, bone_arr);
    }
}

void Skeleton::fillMeshAttribs(LayeredMesh *mesh)
{
    assert(mesh->getBasicVertexType().Name != "AnimatedObject3D");

    for (u32 k = 0; k < mesh->getBuffersCount(); k++) {
        auto buf = mesh->getBuffer(k);
        for (u32 i = 0; i < buf->getVertexCount(); i++) {
            u8 bones_packed = 0;
            u8 weights_packed = 0;
            std::array<u32, 2> bones_ids;
            std::array<u32, 2> weights;

            for (u8 j = 0; j < getNodesCount(); j++) {
                auto node = getNode(j);

                std::array<Weight, BONE_MAX_WEIGHTS> *weights_p;

                if (node->Type != TransformNodeType::BONE) {
                    auto parent = node->getParent();
                    while (parent->Type != TransformNodeType::BONE) {
                        parent = parent->getParent();
                    }
                    weights_p = &(dynamic_cast<Bone *>(parent)->Weights);
                }
                else
                    weights_p = &(dynamic_cast<Bone *>(node)->Weights);
                for (auto w : *weights_p) {
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

            buf->setV2FAttr(v2f(bones_ids[0], bones_ids[1]), 5, i);
            buf->setV2FAttr(v2f(weights[0], weights[1]), 6, i);
        }
    }
}
