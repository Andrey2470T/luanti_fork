#pragma once

#include <Utils/AABB.h>
#include <Image/Image.h>

class RenderSystem;
class ResourceCache;
class LayeredMesh;
class DistanceSortedDrawList;

class SelectionMesh
{
    RenderSystem *rndsys;
    ResourceCache *cache;

    enum
    {
        HIGHLIGHT_BOX,
        HIGHLIGHT_HALO,
        HIGHLIGHT_NONE
    } mode;

    std::unique_ptr<LayeredMesh> mesh;

    std::vector<aabbf> boxes;
    aabbf default_halo_box = aabbf(v3f(100.0f), v3f(-100.0f));

    img::color8 base_color;
    img::color8 light_color;

    v3f pos;
    v3f pos_with_offset;
    v3f rotation;

    v3f face_normal;

    f32 thickness;

    img::Image *halo_img;
public:
    SelectionMesh(RenderSystem *_rndsys, ResourceCache *_cache);

    void updateMesh(const v3f &new_pos, const v3s16 &camera_offset,
        const std::vector<aabbf> &new_boxes, DistanceSortedDrawList *drawlist);

    bool isDisabled() const
    {
        return mode == HIGHLIGHT_NONE;
    }
    LayeredMesh *getMesh() const
    {
        return mesh.get();
    }
    std::vector<aabbf> *getSelectionBoxes() { return &boxes; }

    v3f getPos() const { return pos; }

    void setRotation(v3f rot) { rotation = rot; }
    v3f getRotation() const { return rotation; }

    void updateCameraOffset(const v3s16 &camera_offset, DistanceSortedDrawList *drawlist)
    {
        updateMesh(pos, camera_offset, boxes, drawlist);
    }

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
    RenderSystem *rndsys;

    std::unique_ptr<LayeredMesh> mesh;

    f32 thickness;
public:
    enum Mode
    {
        BLOCK_BOUNDS_OFF,
        BLOCK_BOUNDS_CURRENT,
        BLOCK_BOUNDS_NEAR,
    } mode = BLOCK_BOUNDS_OFF;

    BlockBounds(RenderSystem *_rndsys);

    bool isDisabled() const
    {
        return mode == BLOCK_BOUNDS_OFF;
    }
    LayeredMesh *getMesh() const
    {
        return mesh.get();
    }
    Mode toggle(Client *client, DistanceSortedDrawList *drawlist);
    void disable()
    {
        mode = BLOCK_BOUNDS_OFF;
    }
private:
    void updateMesh(Client *client, DistanceSortedDrawList *drawlist);
};
