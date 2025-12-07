#pragma once

#include <memory>
#include <BasicIncludes.h>
#include <Render/VertexTypeDescriptor.h>
#include "client/render/camera.h"
#include <Utils/Quaternion.h>

#define HAS_HW(layer) \
    layer->material_flags & MATERIAL_FLAG_HARDWARE_COLORIZED

class MeshBuffer;
struct TileLayer;
class Camera;

// Layer of some mesh buffer
struct LayeredMeshPart
{
	u8 buffer_id;
    u8 layer_id;

    u32 offset = 0; // number of start index in the index buffer
    u32 count = 0; // count of buffer indices starting from "offset"
    u32 vertex_offset = 0; // number of start vertex for this part (used only in the model load)
    u32 vertex_count = 0; // count of buffer vertices for this part (used only in the model load)

    // After 3d vertex batching, new indices are mandatory to be added also here
    std::vector<u32> indices;

    LayeredMeshPart() = default;
    LayeredMeshPart(u8 _buffer_id, u8 _layer_id, u32 _offset, u32 _count)
        : buffer_id(_buffer_id), layer_id(_layer_id), offset(_offset), count(_count)
    {}
};

// Triangle of some mesh buffer consisting from three indices
struct LayeredMeshTriangle
{
    MeshBuffer *buf_ref;
    std::shared_ptr<TileLayer> layer;

    u32 p1, p2, p3;

    v3f getCenter() const;

    LayeredMeshTriangle(MeshBuffer *_buf_ref, std::shared_ptr<TileLayer> _layer, u32 _p1, u32 _p2, u32 _p3)
        : buf_ref(_buf_ref), layer(_layer), p1(_p1), p2(_p2), p3(_p3)
    {}
};

typedef std::pair<std::shared_ptr<TileLayer>, LayeredMeshPart> MeshLayer;

// Mesh buffers each divided into layers with unique tile layer
// Supports transparent auto sorting and frustum culling
// This class used internally in MapBlockMesh, GenericCAO, SelectionMesh and CrackOverlay
class LayeredMesh
{
    std::vector<std::unique_ptr<MeshBuffer>> buffers;
    // Indexed by buffer_id
    std::vector<std::vector<MeshLayer>> layers;

    f32 radius_sq = 0.0f;
    v3f center_pos; // relative BS coords, e.g. a half-side of a mapblock cube
    v3f abs_pos; // absolute BS coords, for mapblock it is a min corner
    v3f abs_rot;

    struct TransparentTrianglesSorter
    {
        v3f camera_pos;
        
        TransparentTrianglesSorter() = default;

        bool operator()(const LayeredMeshTriangle &trig1, const LayeredMeshTriangle &trig2);
    };

    TransparentTrianglesSorter trig_sorter;
    std::vector<LayeredMeshTriangle> transparent_triangles;

    std::vector<LayeredMeshPart> partial_layers;
public:
    LayeredMesh() = default;
    LayeredMesh(const v3f &center, const v3f &abs_center);

    f32 getBoundingSphereRadius() const
    {
        return radius_sq;
    }
    v3f getBoundingSphereCenter() const
    {
        return abs_pos + center_pos;
    }

    v3f &getRotation()
    {
        return abs_rot;
    }

    u8 getBuffersCount() const
    {
        return buffers.size();
    }

    u8 getBufferLayersCount(u8 buffer_id) const
    {
        return layers.at(buffer_id).size();
    }

    MeshBuffer *getBuffer(u8 buffer_id) const
    {
        return buffers.at(buffer_id).get();
    }

    MeshLayer &getBufferLayer(u8 buffer_id, u8 layer_id)
    {
        return layers.at(buffer_id).at(layer_id);
    }
    
    std::vector<MeshLayer> getAllLayers() const;
    std::vector<LayeredMeshPart> getPartialLayers() const
    {
    	return partial_layers;
    }

    void setCenter(const v3f &center, const v3f &abs_center)
    {
        center_pos = center;
        abs_pos = abs_center;
    }

    void addNewBuffer(MeshBuffer *buffer)
    {
        buffers.emplace_back(std::unique_ptr<MeshBuffer>(buffer));
        layers.emplace_back();
        recalculateBoundingRadius();
    }
    // adds the layer in the last buffer
    void addNewLayer(std::shared_ptr<TileLayer> layer, const LayeredMeshPart &mesh_p)
    {
        layers.back().emplace_back(layer, mesh_p);
    }

    MeshLayer &findLayer(std::shared_ptr<TileLayer> layer, render::VertexTypeDescriptor vType,
        u32 vertexCount, u32 indexCount);

    void recalculateBoundingRadius();

    void splitTransparentLayers();
    void transparentSort(const v3f &cam_pos);
    
    void updateIndexBuffers();
    
    bool isFrustumCulled(const Camera *camera, f32 extra_radius)
    {
        return camera->frustumCull(center_pos, radius_sq + extra_radius*extra_radius);
    }
private:
    bool isHardwareHolorized(u8 buf_i) const;
};
