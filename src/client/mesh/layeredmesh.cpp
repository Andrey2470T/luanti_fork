#include "layeredmesh.h"
#include "client/ao/animation.h"
#include "client/mesh/model.h"
#include "client/render/renderer.h"
#include "client/render/rendersystem.h"
#include "meshbuffer.h"
#include "client/render/tilelayer.h"
#include "client/core/client.h"
#include "client/player/playercamera.h"

/*!
 * Two layers are equal if they can be merged.
 */
bool LayeredMeshMaterial::operator==(const LayeredMeshMaterial &other) const
{
    return (
        textures == other.textures &&
        shader == other.shader &&
        alpha_discard == other.alpha_discard &&
        material_flags == other.material_flags &&
        line_thickness == other.line_thickness &&
        thing == other.thing &&
        bone_offset == other.bone_offset &&
        animate_normals == other.animate_normals);
}

void LayeredMeshMaterial::setupRenderState(Client *client) const
{
    auto rndsys = client->getRenderSystem();
    auto rnd = rndsys->getRenderer();
    rnd->setRenderState(true);

    rnd->setClipRect(recti());

    auto ctxt = rnd->getContext();

    ctxt->enableCullFace(material_flags & MATERIAL_FLAG_BACKFACE_CULLING);
    ctxt->setCullMode(render::CM_BACK);
    ctxt->setLineWidth(line_thickness);

    if (!use_default_shader) {
        rnd->setBlending(material_flags & MATERIAL_FLAG_TRANSPARENT);
        rnd->setShader(shader);
        shader->setUniformInt("mAlphaDiscard", alpha_discard);
    }
    else {
        rnd->setDefaultShader(material_flags & MATERIAL_FLAG_TRANSPARENT, true);
        rnd->setDefaultUniforms(1.0f, alpha_discard, 0.5f, img::BM_COUNT);
    }

    for (u8 i = 0; i < textures.size(); i++)
        ctxt->setActiveUnit(i, textures.at(i));

    // Workaround
    if (shader && (thing == RenderThing::NODE || thing == RenderThing::OBJECT)) {
        u32 daynight_ratio = (f32)client->getEnv().getDayNightRatio();
        shader->setUniformFloat("mDayNightRatio", (f32)daynight_ratio);

        u32 animation_timer = client->getEnv().getFrameTime() % 1000000;
        f32 animation_timer_f = (f32)animation_timer / 100000.f;
        shader->setUniformFloat("mAnimationTimer", animation_timer_f);

        auto camera = client->getEnv().getLocalPlayer()->getCamera();
        v3f offset = intToFloat(camera->getOffset(), BS);
        shader->setUniform3Float("mCameraOffset", offset);

        if (thing == RenderThing::NODE) {
            v3f camera_position = camera->getPosition();
            shader->setUniform3Float("mCameraPosition", camera_position);
        }
        else {
            shader->setUniformInt("mBonesOffset", bone_offset);
            shader->setUniformInt("mAnimateNormals", (s32)animate_normals);

            auto bonetex = rndsys->getAnimationManager()->getBonesTexture();
            rnd->setDataTexParams(bonetex);
        }
    }
}

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
    radius = 0.0f;

    for (auto &buffer : buffers) {
        for (u32 k = 0; k < buffer->getVertexCount(); k++) {
            v3f p = buffer->getV3FAttr(0, k);

            radius = std::max(radius, (p - center_pos).getLengthSQ());
        }
    }

    radius = std::sqrt(radius);
}

void LayeredMesh::splitTransparentLayers()
{
	transparent_triangles.clear();
	
	for (u8 buf_i = 0; buf_i < getBuffersCount(); buf_i++) {
        auto buffer = getBuffer(buf_i);
        auto indices = buffer->getIndexData();
        for (auto &layer : layers[buffer]) {
            if (!(layer.first.material_flags & MATERIAL_FLAG_TRANSPARENT))
			    continue;
			
            u32 offset = layer.second.offset;
            u32 count = layer.second.count;
            transparent_triangles.reserve(transparent_triangles.capacity() + count / 3);
			
            for (u32 index = 0; index < count / 3; index+=3) {
			    transparent_triangles.emplace_back(
                    buffer, layer.first,
                    *(indices+(offset+index)),
                    *(indices+(offset+index+1)),
                    *(indices+(offset+index+2)));
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

LayeredMesh *LayeredMesh::copy()
{
    auto new_mesh = new LayeredMesh(center_pos, abs_pos);

    for (u32 buf_i = 0; buf_i < buffers.size(); buf_i++) {
        auto original_mesh = getBuffer(buf_i);
        auto copied_mesh = original_mesh->copy();
        new_mesh->buffers.emplace_back(copied_mesh);

        new_mesh->layers[copied_mesh] = getBufferLayers(original_mesh);

        for (auto &layer : new_mesh->layers[copied_mesh])
            layer.second.buf_ref = copied_mesh;
    }

    return new_mesh;
}

	
