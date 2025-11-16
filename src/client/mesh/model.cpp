#include "model.h"
#include "client/render/batcher3d.h"
#include "log.h"
#include "defaultVertexTypes.h"
#include "client/ao/skeleton.h"
#include "client/ao/animation.h"
#include <Utils/Matrix4.h>
#include "layeredmesh.h"
#include "client/render/tilelayer.h"
#include "client/media/resource.h"
#include "client/mesh/layeredmesh.h"

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

KeyChannelInterpMode convertFromAssimpInterp(aiAnimBehaviour b)
{
    switch(b) {
    case aiAnimBehaviour_CONSTANT:
        return KeyChannelInterpMode::CONSTANT;
    case aiAnimBehaviour_LINEAR:
        return KeyChannelInterpMode::LINEAR;
    default:
        return KeyChannelInterpMode::CONSTANT;
    };
}

Model::Model(AnimationManager *_mgr)
    : mgr(_mgr)
{}

Model::Model(v3f pos, const std::vector<MeshLayer> &layers, MeshBuffer *buffer)
{
    mesh = std::make_unique<LayeredMesh>(v3f(), pos);
    mesh->addNewBuffer(buffer);

    for (auto layer : layers)
        mesh->addNewLayer(layer.first, layer.second);
    mesh->splitTransparentLayers();
}

Model::Model(AnimationManager *_mgr, const aiScene *scene)
    : mgr(_mgr)
{
    // aka Material Groups
    // The material groups order defines the meshes groups one

    bool has_skeleton = scene->hasSkeletons();
    bool has_anim = scene->HasAnimations();

    mesh = std::make_unique<LayeredMesh>();
    mesh->addNewBuffer(new MeshBuffer(true, has_skeleton ? AOVType : NodeVType));

    // process meshes
    std::map<u32, std::vector<aiMesh *>> sortedMeshes;

    for (u8 i = 0; i < scene->mNumMeshes; i++) {
        auto mesh = scene->mMeshes[i];
        sortedMeshes[mesh->mMaterialIndex].emplace_back(mesh);
    }

    for (u8 i = 0; i < sortedMeshes.size(); i++)
        processMesh(i, sortedMeshes[i]);

    // Adds only the first skeleton and first animation
    if (has_skeleton) {
        processBones(scene);

        if (has_anim)
            processAnimations(scene);
    }

    mesh->splitTransparentLayers();
}

Model *Model::load(AnimationManager *_mgr, const std::string &name)
{
    Assimp::Importer importer;

    std::string path = fallbackPathFinder(name);

    if (path.empty()) {
        warningstream << "Model::load() Couldn't find any path to the resource with name " << name << std::endl;
        return nullptr;
    }

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

    return new Model(_mgr, scene);
}

Model *Model::loadFromMem(AnimationManager *_mgr, void *mem, s32 size, const std::string &format)
{
    Assimp::Importer importer;

    const aiScene *scene = importer.ReadFileFromMemory(mem, size,
        aiProcess_JoinIdenticalVertices |
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_LimitBoneWeights |
        aiProcess_FlipWindingOrder |
        aiProcess_PopulateArmatureData,
        format.c_str());

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        errorstream << "Model::loadFromMem() Failed to load the model from memory" << std::endl;
        return nullptr;
    }

    return new Model(_mgr, scene);
}

void Model::processMesh(u8 mat_i, const std::vector<aiMesh *> &meshes)
{
    if (meshes.empty())
        return;
    auto buf = mesh->getBuffer(0);
    u32 vertexCount = buf->getVertexCount();
    u32 indexCount = buf->getIndexCount();

    u32 vertices_put = 0;
    u32 indices_put = 0;

    for (u32 i = 0; i < meshes.size(); i++) {
        vertices_put += meshes[i]->mNumVertices;
        indices_put += meshes[i]->mNumFaces * 3;
    }
    buf->reallocateData(vertexCount + vertices_put, indexCount + indices_put);

    LayeredMeshPart mesh_part;
    mesh_part.buffer_id = 0;
    mesh_part.layer_id = mat_i;
    mesh_part.offset = indexCount;
    mesh_part.count = indices_put;
    mesh_part.vertex_offset = vertexCount;
    mesh_part.vertex_count = vertices_put;

    std::shared_ptr<TileLayer> tilelayer = std::make_shared<TileLayer>();
    mesh->addNewLayer(tilelayer, mesh_part);

    auto vType = buf->getVAO()->getVertexType();

    for (u32 i = 0; i < meshes.size(); i++) {
        auto m = meshes[i];
        for (u32 i = 0; i < m->mNumVertices; i++) {
            auto pos = m->mVertices[i];
            auto color = m->HasVertexColors(0) ? m->mColors[0][i] : aiColor4D();
            auto normal = m->HasNormals() ? m->mNormals[i] : aiVector3D();
            auto uv = m->HasTextureCoords(0) ? m->mTextureCoords[0][i] : aiVector3D();

            Batcher3D::appendVertex(buf, v3f(pos.x, pos.y, pos.z),
                img::color8(img::PF_RGBA8, color.r, color.g, color.b, color.a),
                v3f(normal.x, normal.y, normal.z), v2f(uv.x, uv.y));
        }

        for (u32 f = 0; f < m->mNumFaces; f++) {
            for (u8 k = 0; k < 3; k++)
                appendIndex(buf, m->mFaces[f].mIndices[k]);
        }
    }
}

void Model::setBoneRelations(std::vector<Bone *> &bones, u8 &boneID, aiNode *curNode, std::optional<u8> parentID)
{
	Bone *curBone = bones.at(boneID);
    if (parentID) {
        curBone->Parent = parentID;
        curBone->getParent()->Children.push_back(boneID);
    }
    
    curBone->Name = curNode->mName.data;

    matrix4 localTransform = convertFromAssimpMatrix(curNode->mTransformation);
    matrix4 parentTransform;

    if (curNode->mParent)
        parentTransform = convertFromAssimpMatrix(curNode->mParent->mTransformation);
    curBone->AbsoluteTransform = parentTransform * localTransform;
    
    bone_mappings.emplace_back(curNode, curBone);

    for (u8 i = 0; i < curNode->mNumChildren; i++) {
        boneID++;
        setBoneRelations(bones, boneID, curNode->mChildren[i], boneID);
    }
}

void Model::setBoneWeights(aiSkeleton *skeleton, aiNode *node, Bone *bone)
{
	aiSkeletonBone *aibone = nullptr;
	
	for (u8 i = 0; i < skeleton->mNumBones; i++)
	    if (skeleton->mBones[i]->mNode == node)
	        aibone = skeleton->mBones[i];
	
    std::vector<std::pair<u32, f32>> weights(aibone->mNumnWeights);
    for (u8 j = 0; j < aibone->mNumnWeights; j++)
        weights[j] = std::make_pair(aibone->mWeights[j].mVertexId, aibone->mWeights[j].mWeight);

    bone->addWeights(weights);
}

void Model::processBones(const aiScene *scene)
{
    skeleton = new Skeleton(mgr->getBonesTexture(), mgr->getBonesCount());
    skeleton->setAnimatedMesh(mesh.get());
    mgr->addSkeleton(skeleton);

    // process bones (indexes of "bones" and "skeleton->mBones" coincide)
    std::vector<TransformNode *> nodes;

    // Load only the first skeleton
    auto aiskeleton = scene->mSkeletons[0];
    bones.resize(aiskeleton->mNumBones);

    // fill with the info about the each bone' parents and children
    bone_mappings.clear();
    u8 bone_id = 0;
    for (u8 i = 0; i < aiskeleton->mNumBones; i++) {
        auto boneNode = aiskeleton->mBones[i]->mNode;

        // start from the root bones
        if (boneNode->mParent)
            continue;
        setBoneRelations(bones, bone_id, boneNode);
    }

    // copy weights
    for (auto &b_m : bone_mappings)
        setBoneWeights(aiskeleton, b_m.first, b_m.second);

    for (auto bone : bones)
        nodes.push_back(bone);
    skeleton->addBones(nodes);

    skeleton->fillMeshAttribs(mesh.get());
}

void Model::processAnimations(const aiScene *scene)
{
    animation = new BoneAnimation();
    mgr->addAnimation(animation);

    std::map<f32, std::map<u8, v3f>> posKeys;
    std::map<f32, std::map<u8, Quaternion>> rotKeys;
    std::map<f32, std::map<u8, v3f>> scaleKeys;
    KeyChannelInterpMode posInterp, rotInterp, scaleInterp;

    auto getBoneID = [this] (std::string name) -> s32 {
        auto boneIt = std::find_if(bones.begin(), bones.end(), [&name] (const Bone *bone)
        {
            return name == bone->Name;
        });

        if (boneIt == bones.end())
            return -1;

        return std::distance(bones.begin(), boneIt);
    };

    animation->setBase(skeleton);
    auto anim = scene->mAnimations[0];

    for (u8 j = 0; j < anim->mNumChannels; j++) {
        auto boneChannels = anim->mChannels[j];

        s32 boneID = getBoneID(boneChannels->mNodeName.data);

        if (boneID == -1)
            continue;
        for (u8 p_f = 0; p_f < boneChannels->mNumPositionKeys; p_f++) {
            auto pos = boneChannels->mPositionKeys[p_f];
            posKeys[pos.mTime][boneID] = v3f(pos.mValue.x, pos.mValue.y, pos.mValue.z);
        }
        for (u8 r_f = 0; r_f < boneChannels->mNumRotationKeys; r_f++) {
            auto rot = boneChannels->mRotationKeys[r_f];
            rotKeys[rot.mTime][boneID] = Quaternion(rot.mValue.x, rot.mValue.y, rot.mValue.z, rot.mValue.w);
        }
        for (u8 s_f = 0; s_f < boneChannels->mNumScalingKeys; s_f++) {
            auto scale = boneChannels->mScalingKeys[s_f];
            scaleKeys[scale.mTime][boneID] = v3f(scale.mValue.x, scale.mValue.y, scale.mValue.z);
        }

        posInterp = convertFromAssimpInterp(boneChannels->mPreState);
        rotInterp = convertFromAssimpInterp(boneChannels->mPreState);
        scaleInterp = convertFromAssimpInterp(boneChannels->mPreState);
    }

    u32 maxFrameCount = std::max({posKeys.size(), rotKeys.size(), scaleKeys.size()});
    auto posIt = posKeys.begin();
    auto rotIt = rotKeys.begin();
    auto scaleIt = scaleKeys.begin();

    for (u32 j = 0; j < maxFrameCount; j++) {
        auto p = posIt++;
        if (p != posKeys.end())
            animation->appendPositionKey(p->first, p->second);
        auto r = rotIt++;
        if (r != rotKeys.end())
            animation->appendRotationKey(r->first, r->second);
        auto s = scaleIt++;
        if (s != scaleKeys.end())
            animation->appendScaleKey(s->first, s->second);
    }

    animation->setInterpModes(posInterp, rotInterp, scaleInterp);

    posKeys.clear();
    rotKeys.clear();
    scaleKeys.clear();
}
