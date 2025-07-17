#pragma once

#include <Utils/AABB.h>
#include "client/mesh/meshbuffer.h"
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
    aabbf default_halo_box = aabbf(v3f(100.0f), v3f(-100.0f));

    img::color8 base_color;
    img::color8 light_color;

    v3f pos;
    v3f pos_with_offset;
    v3f rotation;

    v3f face_normal;

    f32 thickness;

    render::Texture2D *halo_tex;
public:
    SelectionMesh(Renderer *_rnd, ResourceCache *_cache);

    void draw() const;
    void updateMesh();

    std::vector<aabbf> *getSelectionBoxes() { return &boxes; }

    void setPos(const v3f &position, const v3s16 &camera_offset);
    v3f getPos() const { return pos; }

    void setRotation(v3f rot) { rotation = rot; }
    v3f getRotation() const { return rotation; }

    void setLightColor(const img::color8 &c)
    {
        light_color = c;
    }

    void setSelectedFaceNormal(const v3f &normal)
    {
        face_normal = normal;
    }
};

class Client;

class BlockBounds
{
    Renderer *rnd;

    std::unique_ptr<MeshBuffer> mesh;

    f32 thickness;
public:
    enum Mode
    {
        BLOCK_BOUNDS_OFF,
        BLOCK_BOUNDS_CURRENT,
        BLOCK_BOUNDS_NEAR,
    } mode = BLOCK_BOUNDS_OFF;

    BlockBounds(Renderer *_rnd);

    Mode toggle(Client *client);
    void disable()
    {
        mode = BLOCK_BOUNDS_OFF;
    }
    void draw() const;
private:
    void updateMesh(Client *client);
};
