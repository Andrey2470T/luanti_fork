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

bool LayeredMesh::TransparentVerticesSorter::operator()(const LayeredMeshTriangle &trig1, const LayeredMeshTriangle &trig2)
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
                mesh_p.layer_id = std::distance(buf_layers.begin(), layer_found);
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
