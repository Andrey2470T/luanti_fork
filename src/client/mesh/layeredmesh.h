#pragma once

#include <memory>
#include <BasicIncludes.h>
#include <Render/VertexTypeDescriptor.h>
#include "client/render/camera.h"
#include "client/render/tilelayer.h"
#include <Utils/Quaternion.h>

#define HAS_HW(layer) \
    layer->material_flags & MATERIAL_FLAG_HARDWARE_COLORIZED

class MeshBuffer;
class Camera;

struct LayeredMeshMaterial
{
    std::vector<render::Texture *> textures;

    bool use_default_shader = true;
    render::Shader *shader = nullptr;

    // '0' = alpha discard, '1' = alpha discard ref, '2' = no discard (solid)
    s32 alpha_discard = 2;

    u8 material_flags =
        //0 // <- DEBUG, Use the one below
        MATERIAL_FLAG_BACKFACE_CULLING;
        //MATERIAL_FLAG_TILEABLE_HORIZONTAL
        //MATERIAL_FLAG_TILEABLE_VERTICAL;

    f32 line_thickness = 1.0f;

    //! NOTE: this is a dirty workaround to handle uniforms setting in the drawlist
    //! When the custom materials support is done, it will be removed
    RenderThing thing;

    s32 bone_offset = 0;
    s32 animate_normals = 0;

    bool operator==(const LayeredMeshMaterial &other) const;

    bool operator!=(const LayeredMeshMaterial &other) const
    {
        return !(*this == other);
    }

    void setupRenderState(Client *client) const;
};

// Layer of some mesh buffer
struct LayeredMeshPart
{
    MeshBuffer *buf_ref;
    TileLayer layer;

    u32 offset = 0; // number of start index in the index buffer
    u32 count = 0; // count of buffer indices starting from "offset"
    u32 vertex_offset = 0; // number of start vertex for this part (used only in the model load)
    u32 vertex_count = 0; // count of buffer vertices for this part (used only in the model load)

    LayeredMeshPart() = default;
    LayeredMeshPart(MeshBuffer *_buf_ref, const TileLayer &_layer, u32 _offset, u32 _count)
        : buf_ref(_buf_ref), layer(_layer), offset(_offset), count(_count)
    {}
};

// Triangle of some mesh buffer consisting from three indices
struct LayeredMeshTriangle
{
    MeshBuffer *buf_ref;
    TileLayer layer;

    u32 p1, p2, p3;

    v3f getCenter() const;

    LayeredMeshTriangle(MeshBuffer *_buf_ref, const TileLayer &_layer, u32 _p1, u32 _p2, u32 _p3)
        : buf_ref(_buf_ref), layer(_layer), p1(_p1), p2(_p2), p3(_p3)
    {}
};

typedef std::pair<TileLayer, LayeredMeshPart> BufferLayer;
typedef std::vector<BufferLayer> BufferLayers;

// Mesh buffers each divided into layers with unique tile layer
// Supports transparent auto sorting and frustum culling
// This class used internally in MapBlockMesh, GenericCAO, SelectionMesh and CrackOverlay
class LayeredMesh
{
    std::vector<std::unique_ptr<MeshBuffer>> buffers;
    // Indexed by buffer_id
    std::unordered_map<MeshBuffer *, BufferLayers> layers;

    f32 radius = 0.0f; // in BS space, maximal distance from the center to the farest mapblock vertex
    v3f center_pos; // relative BS coords, e.g. a half-side of a mapblock cube
    v3f abs_pos; // absolute BS coords, for mapblock it is a min corner
    v3f abs_rot{0.0f, 0.0f, 0.0f};

    struct TransparentTrianglesSorter
    {
        v3f camera_pos;
        
        TransparentTrianglesSorter() = default;

        bool operator()(const LayeredMeshTriangle &trig1, const LayeredMeshTriangle &trig2);
    };

    TransparentTrianglesSorter trig_sorter;
    std::vector<LayeredMeshTriangle> transparent_triangles;

    std::vector<LayeredMeshPart> partial_layers;

    std::unordered_map<MeshBuffer *, std::vector<u32>> bufs_indices;

public:
    LayeredMesh() = default;
    LayeredMesh(const v3f &_center_pos, const v3f &_abs_pos);

    f32 getBoundingSphereRadius() const
    {
        return radius;
    }
    v3f getBoundingSphereCenter() const
    {
        return abs_pos + center_pos;
    }

    v3f getAbsoluteMeshPos() const
    {
        return abs_pos;
    }

    v3f &getRotation()
    {
        return abs_rot;
    }

    u8 getBuffersCount() const
    {
        return buffers.size();
    }

    MeshBuffer *getBuffer(u8 buffer_id) const
    {
        return buffers.at(buffer_id).get();
    }

    BufferLayers &getBufferLayers(MeshBuffer *buf)
    {
        return layers[buf];
    }
    LayeredMeshPart *getBufferLayer(
        MeshBuffer *buf,
        const TileLayer &layer);
    
    std::vector<BufferLayer> getAllLayers() const;
    const std::vector<LayeredMeshPart> &getPartialLayers() const
    {
    	return partial_layers;
    }

    std::unordered_map<MeshBuffer *, std::vector<u32>> &getBuffersIndices()
    {
        return bufs_indices;
    }

    void setCenter(const v3f &center, const v3f &abs_center)
    {
        center_pos = center;
        abs_pos = abs_center;
    }

    void addNewBuffer(MeshBuffer *buffer)
    {
        buffers.emplace_back(std::unique_ptr<MeshBuffer>(buffer));
        layers[buffer];
        recalculateBoundingRadius();
    }
    // adds the layer in the last buffer
    void addNewLayer(
        MeshBuffer *buffer,
        const TileLayer &layer,
        const LayeredMeshPart &mesh_p)
    {
        assert(!layers.empty());
        layers[buffer].emplace_back(layer, mesh_p);
    }

    void recalculateBoundingRadius();

    void splitTransparentLayers();
    void transparentSort(const v3f &cam_pos);
    
    bool updateIndexBuffers();
    
    bool isFrustumCulled(const Camera *camera, f32 extra_radius)
    {
        return camera->frustumCull(getBoundingSphereCenter(), radius + extra_radius);
    }

    LayeredMesh *copy();
private:
    bool isHardwareHolorized(u8 buf_i) const;
};
