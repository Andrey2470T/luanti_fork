#pragma once

#include <memory>
#include <BasicIncludes.h>
#include <Render/VertexTypeDescriptor.h>
#include "client/render/frustum.h"

#define HAS_HW(layer) \
    layer->material_flags & MATERIAL_FLAG_HARDWARE_COLORIZED

class MeshBuffer;
struct TileLayer;
struct Frustum;

struct LayeredMeshPart
{
	u8 buffer_id;
    u8 layer_id;

	u32 offset = 0;
	u32 count;

    // After 3d vertex batching, new indices are mandatory to be added also here
    std::vector<u32> indices;
};

struct LayeredMeshTriangle
{
    MeshBuffer *buf_ref;
    std::shared_ptr<TileLayer> layer;

    u32 p1, p2, p3;

    v3f getCenter() const;
};

typedef std::pair<std::shared_ptr<TileLayer>, LayeredMeshPart> MeshLayer;

class LayeredMesh
{
    std::vector<std::unique_ptr<MeshBuffer>> buffers;
    // Indexed by buffer_id
    std::vector<std::vector<MeshLayer>> layers;

    f32 radius_sq = 0.0f;
	v3f center_pos;
	
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
    LayeredMesh(const v3f &center, render::VertexTypeDescriptor basicVType);

    f32 getBoundingSphereRadius() const
    {
        return radius_sq;
    }
    v3f getBoundingSphereCenter() const
    {
        return center_pos;
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

    MeshLayer &findLayer(std::shared_ptr<TileLayer> layer, u32 vertexCount, u32 indexCount);

    void recalculateBoundingRadius();

    void splitTransparentLayers();
    void transparentSort(const v3f &cam_pos);
    
    void updateIndexBuffers();
    
    bool isFrustumCulled(const Frustum &frustum)
    {
        return frustum.frustumCull(center_pos, radius_sq);
    }
private:
    bool isHardwareHolorized(u8 buf_i) const;
};
