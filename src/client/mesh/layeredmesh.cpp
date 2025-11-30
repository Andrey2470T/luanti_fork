#include "layeredmesh.h"
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

LayeredMesh::LayeredMesh(const v3f &center, const v3f &abs_center)
    : center_pos(center), abs_pos(abs_center)
{}

std::vector<MeshLayer> LayeredMesh::getAllLayers() const
{
    std::vector<MeshLayer> all_layers;

    for (auto &buf_layers : layers)
        for (auto &layer : buf_layers)
            all_layers.push_back(layer);

    return all_layers;
}

MeshLayer &LayeredMesh::findLayer(std::shared_ptr<TileLayer> layer, render::VertexTypeDescriptor vType,
    u32 vertexCount, u32 indexCount)
{
    for (u8 i = 0; i < getBuffersCount(); i++) {
        auto buffer = getBuffer(i);

        // The buffer is overfilled or vertex type is different from the required one
        if (((u64)buffer->getVertexCount() + vertexCount > (u64)T_MAX(u32)) ||
                (vType.Name != buffer->getVAO()->getVertexType().Name))
            continue;

        auto &buf_layers = layers.at(i);

        auto layer_found = std::find_if(buf_layers.begin(), buf_layers.end(),
        [layer] (const MeshLayer &mlayer)
        {
            return *layer == *(mlayer.first);
        });

        buffer->reallocateData(buffer->getVertexCount()+vertexCount, buffer->getIndexCount()+indexCount);
        if (layer_found != buf_layers.end())
            return *layer_found;
        else {
            LayeredMeshPart mesh_p;
            mesh_p.buffer_id = i;
            mesh_p.layer_id = buf_layers.size();
            buf_layers.emplace_back(layer, mesh_p);
            return buf_layers.back();
        }
    }

    buffers.emplace_back(std::make_unique<MeshBuffer>(vertexCount, indexCount, true,
        vType, render::MeshUsage::STATIC, false));
    layers.emplace_back();

    LayeredMeshPart mesh_p;
    mesh_p.buffer_id = getBuffersCount()-1;
    mesh_p.layer_id = 0;

    layers.back().emplace_back(layer, mesh_p);

    recalculateBoundingRadius();

    return layers.back().at(0);
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
		for (auto &layer : layers.at(buf_i)) {
			if (!(layer.first->material_flags & MATERIAL_FLAG_TRANSPARENT))
			    continue;
			
			auto &layer_indices = layer.second.indices;
			transparent_triangles.reserve(transparent_triangles.capacity() + layer_indices.size() / 3);
			
			for (u32 index = 0; index < layer_indices.size() / 3; index+=3) {
			    transparent_triangles.emplace_back(
                    buffers.at(buf_i).get(), layer.first, index, index+1, index+2);
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

	std::vector<std::vector<u32>> bufs_indices(buffers.size());
	
	// Firstly render solid layers
	for (u8 buf_i = 0; buf_i < getBuffersCount(); buf_i++) {
		for (auto &layer : layers.at(buf_i)) {
			if (!(layer.first->material_flags & MATERIAL_FLAG_TRANSPARENT)) {
				layer.second.offset = bufs_indices.at(buf_i).size();
				layer.second.count = layer.second.indices.size();
                bufs_indices.at(buf_i).insert(bufs_indices.at(buf_i).end(), layer.second.indices.begin(), layer.second.indices.end());
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
	    
        auto find_buf = std::find_if(buffers.begin(), buffers.end(),
        [trig] (const std::unique_ptr<MeshBuffer> &buf_ptr)
        {
            return *trig.buf_ref == buf_ptr.get();
        });
        
        u8 buf_i = std::distance(buffers.begin(), find_buf);
	    if (add_partial_layer) {
            auto buf_layers = layers.at(buf_i);
		
            auto find_layer = std::find_if(buf_layers.begin(), buf_layers.end(),
	        [trig] (const MeshLayer &cur_l)
	        {
		        return *trig.layer == *cur_l.first;
		    });
		    u8 layer_i = std::distance(buf_layers.begin(), find_layer);
		    
		    partial_layers.emplace_back(
                buf_i, layer_i, bufs_indices.at(buf_i).size(), 3);
        }
        
        bufs_indices.at(buf_i).push_back(trig.p1);
        bufs_indices.at(buf_i).push_back(trig.p2);
        bufs_indices.at(buf_i).push_back(trig.p3);
        
        last_buf = trig.buf_ref;
        last_layer = trig.layer;
    }
    
    // upload new indices lists for each buffer
    for (u8 buf_i = 0; buf_i < getBuffersCount(); buf_i++) {
        for (u32 index_i = 0; index_i < bufs_indices.at(buf_i).size(); index_i++)
            buffers.at(buf_i)->setIndexAt(bufs_indices.at(buf_i).at(index_i), index_i);
        
        buffers.at(buf_i)->uploadIndexData();
    }
}

bool LayeredMesh::isHardwareHolorized(u8 buf_i) const
{
    return buffers.at(buf_i)->getVAO()->getVertexType().Name == "TwoColorNode3D";
}
                
	
