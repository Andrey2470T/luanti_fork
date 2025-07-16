#include "model.h"
#include "log.h"
#include "defaultVertexTypes.h"
#include "client/ao/skeleton.h"
#include "client/ao/animation.h"
#include <Utils/Matrix4.h>

matrix4 convertFromAssimpMatrix(aiMatrix4x4 m)
{
    matrix4 convM;

    for (u8 i = 0; i < 4; i++) {
        ai_real *row = m[i];

        for (u8 j = 0; j < 4; j++)
            convM[i*4+j] = row[j];
    }

    return convM;
}

Model::Model(const aiScene *scene)
{
    // aka Material Groups
    // The material groups order defines the meshes groups one
    assert(scene->mNumMaterials <= scene->mNumMeshes);

    bool animated = scene->hasSkeletons() && scene->HasAnimations();

    render::VertexTypeDescriptor vtype = animated ? AOVType : render::DefaultVType;
    mesh = std::make_unique<MeshBuffer>(true, vtype);

    if (vtype.Name == "AnimatedObject3D") {
        skeleton = std::make_unique<Skeleton>();
        animations.resize(scene->mNumAnimations);
    }

    // process meshes
    for (u8 i = 0; i < scene->mNumMaterials; i++) {
        aiMesh *m = scene->mMeshes[i];

        u32 vertexCount = mesh->getVertexCount();
        u32 indexCount = mesh->getIndexCount();

        mesh->reallocateData(vertexCount + m->mNumVertices, indexCount + m->mNumFaces * 3);

        for (u32 j = 0; j < m->mNumVertices; j++) {
            auto pos = m->mVertices[i];
            auto color = m->mColors[i];
            auto normal = m->mNormals[i];
            auto uv = m->HasTextureCoords(0) ? m->mTextureCoords[0][i] : aiVector3D(0, 0, 0);

            appendSVT(mesh.get(),
                v3f(pos.x, pos.y, pos.z), img::color8(img::PF_RGBA8, color->r, color->g, color->b, color->a),
                v3f(normal.x, normal.y, normal.z), v2f(uv.x, uv.y));
        }

        for (u32 f = 0; f < m->mNumFaces; f++) {
            for (u8 k = 0; k < 3; k++)
                appendIndex(mesh.get(), m->mFaces[f].mIndices[k]);
        }
    }

    // process bones (indexes of "bones" and "skeleton->mBones" coincide)
    std::vector<Bone *> bones;

    if (scene->hasSkeletons()) {
        // Loads only the first skeleton
        auto skeleton = scene->mSkeletons[0];
        bones.resize(skeleton->mNumBones);

        u8 bone_id = 0;
        for (u8 i = 0; i < skeleton->mNumBones; i++) {
            auto bone = skeleton->mBones[i];

            // start from the root bones
            if (bone->mNode->mParent)
                continue;
            setBoneRelations(bones, bone_id, bone->mNode);
        }
    }

    Keys keys;
}

Model *Model::load(const std::string &path)
{
    Assimp::Importer importer;

    const aiScene *scene = importer.ReadFile(path,
        aiProcess_JoinIdenticalVertices |
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_LimitBoneWeights |
        aiProcess_FlipWindingOrder |
        aiProcess_PopulateArmatureData);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        errorstream << "Model::load() Failed to load the model: " << path << std::endl;
        return nullptr;
    }

    return new Model(scene);
}

MeshPart Model::getMeshPart(u8 n) const
{
    assert(n < parts.size());
    return parts.at(n);
}

void Model::setBoneRelations(std::vector<Bone *> &bones, u8 &boneID, aiNode *curNode, Bone *parent)
{
    if (parent) {
        bones[boneID]->Parent = parent;
        parent->Children.push_back(bones[boneID]);
    }

    for (u8 i = 0; i < curNode->mNumChildren; i++) {
        boneID++;
        setBoneRelations(bones, boneID, curNode->mChildren[i], bones[boneID-1]);
    }
}
Bone *Model::processBone(std::vector<Bone *> &bones, aiSkeletonBone *bone, const aiScene *scene, Bone *parent)
{
    Bone *curBone = bones[curBoneID];
    curBone->Name = bone->mNode->mName.data;

    matrix4 localTransform = convertFromAssimpMatrix(bone->mNode->mTransformation);
    matrix4 parentTransform;

    if (bone->mNode->mParent)
        parentTransform = convertFromAssimpMatrix(bone->mNode->mParent->mTransformation);
    curBone->Transform.GlobalTransform = parentTransform * localTransform;

    std::vector<std::pair<u32, f32>> weights(bone->mNumnWeights);
    for (u8 j = 0; j < bone->mNumnWeights; j++)
        weights[j] = std::make_pair(bone->mWeights[j].mVertexId, bone->mWeights[j].mWeight);

    curBone->addWeights(weights);

    curBone->Parent = parent;

    for (u8 i = 0; i < bone->mNode->mNumChildren; i++) {
        Bone *childBone = processBone(bones, bone->mNode->mChildren[i])
    }
}
