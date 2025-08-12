#include "model.h"
#include "log.h"
#include "defaultVertexTypes.h"
#include "client/ao/skeleton.h"
#include "client/ao/animation.h"
#include <Utils/Matrix4.h>
#include "layeredmesh.h"
#include "client/render/tilelayer.h"
#include "client/media/resource.h"

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

Model::Model(AnimationManager *_mgr, v3f pos, const aiScene *scene, ResourceCache *cache)
    : mgr(_mgr)
{
    // aka Material Groups
    // The material groups order defines the meshes groups one
    assert(scene->mNumMaterials <= scene->mNumMeshes);

    render::VertexTypeDescriptor vType =
        scene->hasSkeletons() && scene->HasAnimations() ?
        AOVType : NodeVType;

    mesh = std::make_unique<LayeredMesh>(v3f(), pos, vType);

    bool animated = vType.Name == "AnimatedObject3D";
    if (animated) {
        skeleton = new Skeleton(mgr->getBonesTexture(), mgr->getBonesCount());
        skeleton->setAnimatedMesh(mesh.get());
        animations.reserve(scene->mNumAnimations);

        u32 skeleton_id = mgr->getSkeletonCount();
        mgr->addSkeleton(skeleton);

        for (u8 i = 0; i < scene->mNumAnimations; i++)
            animations.emplace_back();
        mgr->addAnimations(skeleton_id, animations);
    }

    std::shared_ptr<TileLayer> layer;
    layer->alpha_discard = 1;
    layer->material_flags = MATERIAL_FLAG_TRANSPARENT;
    layer->use_default_shader = false;

    std::string shadername = animated ? "object_skinned" : "object";
    layer->shader = cache->getOrLoad<render::Shader>(ResourceType::SHADER, shadername);

    // Assume that this is extremely low possible for the model to have > 2 milliards vertices
    mesh->addNewBuffer(layer, new MeshBuffer(true, vType));

    // process meshes
    u32 curOffset = 0;
    for (u8 i = 0; i < scene->mNumMaterials; i++)
        processMesh(scene->mMeshes[i]);

    // process bones (indexes of "bones" and "skeleton->mBones" coincide)
    std::vector<Bone *> bones;

    if (animated) {
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
            
        skeleton->addBones(bones);
            
        // process animations
        processAnimations(bones, scene);

        skeleton->fillMeshAttribs(mesh.get());
    }

    mesh->getBuffer(0)->uploadData();
}

Model *Model::load(AnimationManager *_mgr, v3f pos, const std::string &path, ResourceCache *cache)
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

    return new Model(_mgr, pos, scene, cache);
}

void Model::processMesh(aiMesh *m)
{
    auto buf = mesh->getBuffer(0);
    u32 vertexCount = buf->getVertexCount();
    u32 indexCount = buf->getIndexCount();

    buf->reallocateData(vertexCount + m->mNumVertices, indexCount + m->mNumFaces * 3);

    auto vType = buf->getVAO()->getVertexType();
    for (u32 i = 0; i < m->mNumVertices; i++) {
        auto pos = m->mVertices[i];
        auto color = m->mColors[i];
        auto normal = m->mNormals[i];
        auto uv = m->HasTextureCoords(0) ? m->mTextureCoords[0][i] : aiVector3D(0, 0, 0);

        if (vType.Name == "Node3D") {
            appendNVT(buf,
                v3f(pos.x, pos.y, pos.z), img::color8(img::PF_RGBA8, color->r, color->g, color->b, color->a),
                v3f(normal.x, normal.y, normal.z), v2f(uv.x, uv.y));
        }
        else if (vType.Name == "TwoColoredNode3D") {
            appendTCNVT(buf,
                v3f(pos.x, pos.y, pos.z), img::color8(img::PF_RGBA8, color->r, color->g, color->b, color->a),
                v3f(normal.x, normal.y, normal.z), v2f(uv.x, uv.y));
        }
        else {
            appendAOVT(buf,
                v3f(pos.x, pos.y, pos.z), img::color8(img::PF_RGBA8, color->r, color->g, color->b, color->a),
                v3f(normal.x, normal.y, normal.z), v2f(uv.x, uv.y));
        }
    }

    for (u32 f = 0; f < m->mNumFaces; f++) {
        for (u8 k = 0; k < 3; k++)
            appendIndex(buf, m->mFaces[f].mIndices[k]);
    }
}

void Model::setBoneRelations(std::vector<Bone *> &bones, u8 &boneID, aiNode *curNode, Bone *parent)
{
	Bone *curBone = bones.at(boneID);
    if (parent) {
        curBone->Parent = parent;
        parent->Children.push_back(curBone);
    }
    
    curBone->Name = curNode->mName.data;

    matrix4 localTransform = convertFromAssimpMatrix(curNode->mTransformation);
    matrix4 parentTransform;

    if (curNode->mParent)
        parentTransform = convertFromAssimpMatrix(curNode->mParent->mTransformation);
    curBone->Transform.GlobalTransform = parentTransform * localTransform;
    
    bone_mappings.emplace_back(curNode, curBone);

    for (u8 i = 0; i < curNode->mNumChildren; i++) {
        boneID++;
        setBoneRelations(bones, boneID, curNode->mChildren[i], curBone);
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

void Model::processAnimations(std::vector<Bone *> &bones, const aiScene *scene)
{
    std::map<f32, std::map<u8, v3f>> posKeys;
    std::map<f32, std::map<u8, Quaternion>> rotKeys;
    std::map<f32, std::map<u8, v3f>> scaleKeys;
    KeyChannelInterpMode posInterp, rotInterp, scaleInterp;

    auto getBoneID = [&bones] (std::string name) -> s32 {
        auto boneIt = std::find(bones.begin(), bones.end(), [&name] (const Bone *bone)
        {
            return name == bone->Name;
        });

        if (boneIt == bones.end())
            return -1;

        return std::distance(bones.begin(), boneIt);
    };

    for (u8 i = 0; i < scene->mNumAnimations; i++) {
        animations[i]->setBase(skeleton);
        auto anim = scene->mAnimations[i];

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
                animations[i]->appendPositionKey(p->first, p->second);
            auto r = rotIt++;
            if (r != rotKeys.end())
                animations[i]->appendRotationKey(r->first, r->second);
            auto s = scaleIt++;
            if (s != scaleKeys.end())
                animations[i]->appendScaleKey(s->first, s->second);
        }

        animations[i]->setInterpModes(posInterp, rotInterp, scaleInterp);

        posKeys.clear();
        rotKeys.clear();
        scaleKeys.clear();
    }
}
