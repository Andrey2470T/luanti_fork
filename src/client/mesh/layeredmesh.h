#pragma once

#include <memory>
#include <BasicIncludes.h>
#include <Render/VertexTypeDescriptor.h>
#include "client/render/camera.h"

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

	u32 offset = 0;
	u32 count;

    // After 3d vertex batching, new indices are mandatory to be added also here
    std::vector<u32> indices;
};

// Triangle of some mesh buffer consisting from three indices
struct LayeredMeshTriangle
{
    MeshBuffer *buf_ref;
    std::shared_ptr<TileLayer> layer;

    u32 p1, p2, p3;

    v3f getCenter() const;
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
    v3f center_pos; // relative BS coords
    v3f abs_pos; // absolute BS coords
	
	render::VertexTypeDescriptor basicVertexType;

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
    LayeredMesh(const v3f &center, const v3f &abs_center, render::VertexTypeDescriptor basicVType);

    f32 getBoundingSphereRadius() const
    {
        return radius_sq;
    }
    v3f getBoundingSphereCenter() const
    {
        return abs_pos + center_pos;
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
    
    render::VertexTypeDescriptor getBasicVertexType() const
    {
    	return basicVertexType;
    }

    void addNewBuffer(std::shared_ptr<TileLayer> layer, MeshBuffer *buffer);

    MeshLayer &findLayer(std::shared_ptr<TileLayer> layer, u32 vertexCount, u32 indexCount);

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
