#include "layeredmesh.h"
#include "meshbuffer.h"
#include "client/render/tilelayer.h"

v3f LayeredMeshTriangle::getCenter() const
{
    v3f pos1 = buf_ref->getAttrAt<v3f>(0, p1);
    v3f pos2 = buf_ref->getAttrAt<v3f>(0, p2);
    v3f pos3 = buf_ref->getAttrAt<v3f>(0, p3);

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

LayeredMesh::LayeredMesh(const v3f &center, render::VertexTypeDescriptor vtype)
    : center_pos(center), vType(vtype)
{}

MeshLayer &LayeredMesh::findLayer(std::shared_ptr<TileLayer> layer, u32 vertexCount, u32 indexCount)
{
    for (u8 i = 0; i < getBuffersCount(); i++) {
        auto buffer = getBuffer(i);
        auto &buf_layers = layers.at(i);

        auto layer_found = std::find(buf_layers.begin(), buf_layers.end(),
        [layer] (const MeshLayer &mlayer)
        {
            return *layer == *(mlayer.first);
        });

        if (buffer->getVertexCount() + vertexCount <= T_MAX(u32)) {
            buffer->reallocateData(buffer->getVertexCount()+vertexCount, buffer->getIndexCount()+indexCount);
            if (layer_found != buf_layers.end())
                return *layer_found;
            else {
                LayeredMeshPart mesh_p;
                mesh_p.buffer_id = i;
                mesh_p.layer_id = buf_layers.size();
                buf_layers.emplace_back(std::shared_ptr<TileLayer>(layer), mesh_p);
                return buf_layers.back();
            }
        }
    }

    buffers.emplace_back(vertexCount, indexCount, true, vType);
    layers.emplace_back();

    LayeredMeshPart mesh_p;
    mesh_p.buffer_id = getBuffersCount();
    mesh_p.layer_id = 0;

    layers.back().emplace_back(std::shared_ptr<TileLayer>(layer), mesh_p);

    return layers.back().at(0);
}

void LayeredMesh::recalculateBoundingRadius()
{
    radius_sq = 0.0f;

    for (auto &buffer : buffers) {
        for (u32 k = 0; k < buffer->getVertexCount(); k++) {
            v3f p = buffer->getAttrAt<v3f>(0, k);

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
	std::vector<std::vector<u32>> bufs_indices(buffers.size());
	
	// Firstly render solid layers
	for (u8 buf_i = 0; buf_i < getBuffersCount(); buf_i++) {
		for (auto &layer : layers.at(buf_i)) {
			if (!(layer.first->material_flags & MATERIAL_FLAG_TRANSPARENT)) {
				layer.second.offset = bufs_indices.at(buf_i).size();
				layer.second.count = layer.second.indices.size();
				bufs_indices.at(buf_i).insert(layer.second.indices.begin(), layer.second.indices.end());
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
		else if (*last_buf == *trig.buf_ref && *last_layer == *trig.layer) {
			partial_layers.back().count += 3;
			add_partial_layer = false;
	    }
	    
	    auto find_buf = std::find(buffers.begin(), begin.end(),
        [trig] (const std::unique_ptr<MeshBuffer> &buf_ptr)
        {
        	return *trig.buf_ref == *buf_ptr;
        });
        
        u8 buf_i = std::distance(buffers.begin(), find_buf);
	    if (add_partial_layer) {
		    auto buf_layers = buffers.at(buf_i);
		
	        auto find_layer = std::find(buf_layers.begin(), buf_layers.end(),
	        [trig] (const MeshLayer &cur_l)
	        {
		        return *trig.layer == *cur_l.first;
		    });
		    u8 layer_i = std::distance(buf_layers.begin(), find_layer);
		    
		    partial_layers.emplace_back(
                buf_i, layer_i, buf_indices.at(buf_i).size(), 3);
        }
        
        buf_indices.at(buf_i).push_back(trig.p1);
        buf_indices.at(buf_i).push_back(trig.p2);
        buf_indices.at(buf_i).push_back(trig.p3);
        
        last_buf = trig.buf_ref;
        last_layer = trig.layer;
    }
    
    // upload new indices lists for each buffer
    for (u8 buf_i = 0; buf_i < getBuffersCount(); buf_i++) {
    	for (u32 index_i = 0; index_i < buf_indices.at(buf_i); index_i++)
            buffers.at(buf_i)->setIndexAt(buf_indices.at(buf_i).at(index_i), index_i);
        
        buffers.at(buf_i)->uploadIndexData();
    }
}
        
                
	
