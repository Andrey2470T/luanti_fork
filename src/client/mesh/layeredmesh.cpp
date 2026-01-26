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

std::vector<BufferLayer> LayeredMesh::getAllLayers() const
{
    std::vector<BufferLayer> all_layers;

    for (auto &buf_layers : layers)
        for (auto &layer : buf_layers.second)
            all_layers.emplace_back(layer);

    return all_layers;
}

LayeredMeshPart *LayeredMesh::findLayer(
    std::shared_ptr<TileLayer> layer,
    render::VertexTypeDescriptor vType,
    u32 vertexCount, u32 indexCount)
{
    for (u8 i = 0; i < getBuffersCount(); i++) {
        auto buffer = getBuffer(i);

        // The buffer is overfilled or vertex type is different from the required one
        if (((u64)buffer->getVertexCount() + vertexCount > (u64)T_MAX(u32)) ||
                (vType.Name != buffer->getVAO()->getVertexType().Name))
            continue;

        auto &buf_layers = layers[buffer];
        auto &mesh_part = buf_layers[layer];

        mesh_part.buf_ref = buffer;
        mesh_part.layer = layer;
        mesh_part.count += indexCount;
        mesh_part.vertex_count += vertexCount;

        recalculateBoundingRadius();

        return &mesh_part;
    }

    return nullptr;
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
			if (!(layer.first->material_flags & MATERIAL_FLAG_TRANSPARENT))
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

void LayeredMesh::updateIndexBuffers()
{
    if (transparent_triangles.empty()) return;

    std::unordered_map<MeshBuffer *, std::pair<u32, std::vector<u32>>> bufs_indices;

	// Firstly render solid layers
	for (u8 buf_i = 0; buf_i < getBuffersCount(); buf_i++) {
        auto buffer = getBuffer(buf_i);
        for (auto &layer : layers[getBuffer(buf_i)]) {
			if (!(layer.first->material_flags & MATERIAL_FLAG_TRANSPARENT)) {
                layer.second.offset = bufs_indices[buffer].first;
				layer.second.count = layer.second.indices.size();

                bufs_indices[buffer].first += layer.second.count;
                bufs_indices[buffer].second.insert(bufs_indices[buffer].second.end(),
                    layer.second.indices.begin(), layer.second.indices.end());
            }
        }
	}
	
	// Then transparent ones
	partial_layers.clear();
	
	MeshBuffer *last_buf = nullptr;
	std::shared_ptr<TileLayer> last_layer;
	for (auto &trig : transparent_triangles) {
		bool add_partial_layer = true;
		if (!last_buf) {
			last_buf = trig.buf_ref;
			last_layer = trig.layer;
		}
        else if (*last_buf == trig.buf_ref && *last_layer == *trig.layer) {
			partial_layers.back().count += 3;
			add_partial_layer = false;
        }
        
	    if (add_partial_layer) {
		    partial_layers.emplace_back(
                trig.buf_ref, trig.layer, bufs_indices[trig.buf_ref].first, 3);
        }
        
        bufs_indices[trig.buf_ref].first += 3;
        bufs_indices[trig.buf_ref].second.push_back(trig.p1);
        bufs_indices[trig.buf_ref].second.push_back(trig.p2);
        bufs_indices[trig.buf_ref].second.push_back(trig.p3);
        
        last_buf = trig.buf_ref;
        last_layer = trig.layer;
    }
    
    // upload new indices lists for each buffer
    for (u8 buf_i = 0; buf_i < getBuffersCount(); buf_i++) {
        auto buffer = getBuffer(buf_i);
        for (u32 index_i = 0; index_i < bufs_indices[buffer].second.size(); index_i++)
            buffer->setIndexAt(bufs_indices[buffer].second.at(index_i), index_i);
        
        buffers.at(buf_i)->uploadData();
    }
}

bool LayeredMesh::isHardwareHolorized(u8 buf_i) const
{
    return buffers.at(buf_i)->getVAO()->getVertexType().Name == "TwoColorNode3D";
}

	
