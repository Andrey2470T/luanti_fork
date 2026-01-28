#include "layeredmesh.h"
#include "client/mesh/model.h"
#include "meshbuffer.h"
#include "client/render/tilelayer.h"

v3f LayeredMeshTriangle::getCenter() const
{
    v3f pos1 = buf_ref->getV3FAttr(0, p1);
    v3f pos2 = buf_ref->getV3FAttr(0, p2);
    v3f pos3 = buf_ref->getV3FAttr(0, p3);

    return (pos1 + pos2 + pos3) / 3.0f;
}

bool LayeredMesh::TransparentTrianglesSorter::operator()(const LayeredMeshTriangle &trig1, const LayeredMeshTriangle &trig2)
{
    v3f trig1_c = trig1.getCenter();
    v3f trig2_c = trig2.getCenter();

    f32 dist1 = trig1_c.getDistanceFromSQ(camera_pos);
    f32 dist2 = trig2_c.getDistanceFrom(camera_pos);

    return dist1 > dist2;
}

LayeredMesh::LayeredMesh(const v3f &_center_pos, const v3f &_abs_pos)
    : center_pos(_center_pos), abs_pos(_abs_pos)
{}

LayeredMeshPart *LayeredMesh::getBufferLayer(
    MeshBuffer *buf,
    const TileLayer &layer)
{
    auto foundLayer = std::find_if(layers[buf].begin(), layers[buf].end(),
        [layer] (auto &curLayer) { return curLayer.first == layer; });

    if (foundLayer == layers[buf].end())
        return nullptr;
    return &(foundLayer->second);
}

std::vector<BufferLayer> LayeredMesh::getAllLayers() const
{
    std::vector<BufferLayer> all_layers;

    for (auto &buf_layers : layers)
        for (auto &layer : buf_layers.second)
            all_layers.emplace_back(layer);

    return all_layers;
}

void LayeredMesh::recalculateBoundingRadius()
{
    radius_sq = 0.0f;

    for (auto &buffer : buffers) {
        for (u32 k = 0; k < buffer->getVertexCount(); k++) {
            v3f p = buffer->getV3FAttr(0, k);

            radius_sq = std::max(radius_sq, (p - center_pos).getLengthSQ());
        }
    }
}

void LayeredMesh::splitTransparentLayers()
{
	transparent_triangles.clear();
	
	for (u8 buf_i = 0; buf_i < getBuffersCount(); buf_i++) {
        auto buffer = getBuffer(buf_i);
        for (auto &layer : layers[buffer]) {
            if (!(layer.first.material_flags & MATERIAL_FLAG_TRANSPARENT))
			    continue;
			
			auto &layer_indices = layer.second.indices;
			transparent_triangles.reserve(transparent_triangles.capacity() + layer_indices.size() / 3);
			
			for (u32 index = 0; index < layer_indices.size() / 3; index+=3) {
			    transparent_triangles.emplace_back(
                    buffer, layer.first,
                    layer_indices.at(index), layer_indices.at(index+1), layer_indices.at(index+2));
            }
        }
    }
}

void LayeredMesh::transparentSort(const v3f &cam_pos)
{
	trig_sorter.camera_pos = cam_pos;
	std::sort(transparent_triangles.begin(), transparent_triangles.end(), trig_sorter);
}

bool LayeredMesh::updateIndexBuffers()
{
    if (transparent_triangles.empty()) return false;

    bufs_indices.clear();

	// Firstly render solid layers
	for (u8 buf_i = 0; buf_i < getBuffersCount(); buf_i++) {
        auto buffer = getBuffer(buf_i);
        auto indices = buffer->getIndexData();
        auto &buf_indices = bufs_indices[buffer];
        for (auto &layer : layers[buffer]) {
            if (!(layer.first.material_flags & MATERIAL_FLAG_TRANSPARENT)) {
                u32 buf_indices_count = buf_indices.size();
                buf_indices.resize(buf_indices_count+layer.second.count);
                memcpy(
                    buf_indices.data()+buf_indices_count*sizeof(u32),
                    indices+layer.second.offset*sizeof(u32),
                    layer.second.count*sizeof(u32)
                );
            }
        }
	}
	
	// Then transparent ones
	partial_layers.clear();
	
	MeshBuffer *last_buf = nullptr;
    TileLayer last_layer;
	for (auto &trig : transparent_triangles) {
		bool add_partial_layer = true;
		if (!last_buf) {
			last_buf = trig.buf_ref;
			last_layer = trig.layer;
		}
        else if (last_buf == trig.buf_ref && last_layer == trig.layer) {
			partial_layers.back().count += 3;
			add_partial_layer = false;
        }
        
	    if (add_partial_layer) {
		    partial_layers.emplace_back(
                trig.buf_ref, trig.layer, bufs_indices[trig.buf_ref].size(), 3);
        }
        
        bufs_indices[trig.buf_ref].push_back(trig.p1);
        bufs_indices[trig.buf_ref].push_back(trig.p2);
        bufs_indices[trig.buf_ref].push_back(trig.p3);
        
        last_buf = trig.buf_ref;
        last_layer = trig.layer;
    }

    for (u8 buf_i = 0; buf_i < getBuffersCount(); buf_i++) {
        auto buffer = getBuffer(buf_i);
        for (u32 index_i = 0; index_i < bufs_indices[buffer].size(); index_i++)
            buffer->setIndexAt(bufs_indices[buffer].at(index_i), index_i);
    }
    
    return true;
}

bool LayeredMesh::isHardwareHolorized(u8 buf_i) const
{
    return buffers.at(buf_i)->getVertexType().Name == "TwoColorNode3D";
}

	
