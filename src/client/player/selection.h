#pragma once

#include <Utils/AABB.h>
#include "client/render/meshbuffer.h"
#include <Render/Texture2D.h>

class Renderer;
class ResourceCache;

class SelectionMesh
{
    Renderer *rnd;
    ResourceCache *cache;

    enum
    {
        HIGHLIGHT_BOX,
        HIGHLIGHT_HALO,
        HIGHLIGHT_NONE
    } mode;

    std::unique_ptr<MeshBuffer> mesh;

    std::vector<aabbf> boxes;
    std::vector<aabbf> halos;

    img::color8 color;

    v3f pos;
    v3s16 camera_offset;
    v3f rotation;

    v3f face_normal;

    f32 thickness;

    render::Texture2D *halo_tex;
public:
    SelectionMesh(Renderer *_rnd, ResourceCache *_cache);

    void draw();
    void updateSelectionMesh(const v3s16 &camera_offset);

    std::vector<aabbf> *getSelectionBoxes() { return &boxes; }

    void setSelectionPos(const v3f &pos, const v3s16 &camera_offset);

    v3f getSelectionPos() const { return pos; }

    void setSelectionRotation(v3f rot) { rotation = rot; }

    v3f getSelectionRotation() const { return rotation; }

    void setSelectionMeshColor(const img::color8 &c)
    {
        color = c;
    }

    void setSelectedFaceNormal(const v3f &normal)
    {
        face_normal = normal;
    }
};
