// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "mapblockmesh.h"
#include "client/client.h"
#include "mapblock.h"
#include "map.h"
#include "profiler.h"
#include "client/mesh/meshoperations.h"
#include "client/ui/minimap.h"
#include "meshgenerator.h"
#include "util/directiontables.h"
#include "util/tracy_wrapper.h"
#include <algorithm>
#include <cmath>
#include "client/render/rendersystem.h"
#include "client/media/resource.h"
#include "client/mesh/defaultVertexTypes.h"
#include "client/render/atlas.h"


/*
	MeshMakeData
*/

MeshMakeData::MeshMakeData(const NodeDefManager *ndef,
		u16 side_length, MeshGrid mesh_grid) :
	m_side_length(side_length),
	m_mesh_grid(mesh_grid),
	m_nodedef(ndef)
{
	assert(m_side_length > 0);
}

void MeshMakeData::fillBlockDataBegin(const v3s16 &blockpos)
{
	m_blockpos = blockpos;

	v3s16 blockpos_nodes = m_blockpos*MAP_BLOCKSIZE;

	m_vmanip.clear();
	// extra 1 block thick layer around the mesh
	VoxelArea voxel_area(blockpos_nodes - v3s16(1,1,1) * MAP_BLOCKSIZE,
			blockpos_nodes + v3s16(1,1,1) * (m_side_length + MAP_BLOCKSIZE) - v3s16(1,1,1));
	m_vmanip.addArea(voxel_area);
}

void MeshMakeData::fillBlockData(const v3s16 &bp, MapNode *data)
{
	v3s16 data_size(MAP_BLOCKSIZE, MAP_BLOCKSIZE, MAP_BLOCKSIZE);
	VoxelArea data_area(v3s16(0,0,0), data_size - v3s16(1,1,1));

	v3s16 blockpos_nodes = bp * MAP_BLOCKSIZE;
	m_vmanip.copyFrom(data, data_area, v3s16(0,0,0), blockpos_nodes, data_size);
}

void MeshMakeData::fillSingleNode(MapNode data, MapNode padding)
{
	m_blockpos = {0, 0, 0};

	m_vmanip.clear();
	// area around 0,0,0 so that this positon has neighbors
	const s16 sz = 3;
	m_vmanip.addArea({v3s16(-sz), v3s16(sz)});

	u32 count = m_vmanip.m_area.getVolume();
	for (u32 i = 0; i < count; i++) {
		m_vmanip.m_data[i] = padding;
		m_vmanip.m_flags[i] &= ~VOXELFLAG_NO_DATA;
	}

	m_vmanip.setNodeNoEmerge({0, 0, 0}, data);
}

/*void MeshMakeData::setCrack(int crack_level, v3s16 crack_pos)
{
	if (crack_level >= 0)
		m_crack_pos_relative = crack_pos - m_blockpos*MAP_BLOCKSIZE;
}*/


/*
	MapBlockMesh
*/

MapBlockMesh::MapBlockMesh(Client *client, MeshMakeData *data)
    : m_mesh(std::make_unique<LayeredMesh>(v3f((data->m_side_length * 0.5f - 0.5f) * BS), NodeVType)),
    m_rndsys(client->getRenderSystem()), m_cache(client->getResourceCache())
{
	ZoneScoped;

	auto mesh_grid = data->m_mesh_grid;
	v3s16 bp = data->m_blockpos;
	// Only generate minimap mapblocks at grid aligned coordinates.
	// FIXME: ^ doesn't really make sense. and in practice, bp is always aligned
	if (mesh_grid.isMeshPos(bp) && data->m_generate_minimap) {
		// meshgen area always fits into a grid cell
		m_minimap_mapblocks.resize(mesh_grid.getCellVolume(), nullptr);
		v3s16 ofs;

		// See also client.cpp for the code that reads the array of minimap blocks.
		for (ofs.Z = 0; ofs.Z < mesh_grid.cell_size; ofs.Z++)
		for (ofs.Y = 0; ofs.Y < mesh_grid.cell_size; ofs.Y++)
		for (ofs.X = 0; ofs.X < mesh_grid.cell_size; ofs.X++) {
			v3s16 p = (bp + ofs) * MAP_BLOCKSIZE;
			if (data->m_vmanip.getNodeNoEx(p).getContent() != CONTENT_IGNORE) {
				MinimapMapblock *block = new MinimapMapblock;
				m_minimap_mapblocks[mesh_grid.getOffsetIndex(ofs)] = block;
				block->getMinimapNodes(&data->m_vmanip, p);
			}
		}
	}

	// algin vertices to mesh grid, not meshgen area
    //offset = intToFloat((data->m_blockpos - mesh_grid.getMeshPos(data->m_blockpos)) * MAP_BLOCKSIZE, BS);
    m_translation = intToFloat(mesh_grid.getMeshPos(data->m_blockpos) * MAP_BLOCKSIZE, BS);

    /*MeshCollector collector(m_bounding_sphere_center, offset);

	{
		// Generate everything
		MapblockMeshGenerator(data, &collector).generate();
    }*/


    auto basicPool = m_rndsys->getPool(true);
    for (u8 buf_i = 0; buf_i < m_mesh->getBuffersCount(); buf_i++) {
        auto buffer = m_mesh->getBuffer(buf_i);
        for (u8 layer_i = 0; layer_i < m_mesh->getBufferLayersCount(buf_i); layer_i++) {
            auto layer = m_mesh->getBufferLayer(buf_i, layer_i).first;

            layer->material_flags |= MATERIAL_FLAG_BACKFACE_CULLING;

            layer->atlas = basicPool->getAtlasByTile(layer->tile_ref);
            auto src_rect = basicPool->getTileRect(layer->tile_ref);

            std::string shadername = "block";
            if (layer->material_flags & MATERIAL_FLAG_HARDWARE_COLORIZED)
                shadername = "block_hw";

            layer->shader = m_cache->getOrLoad<render::Shader>(ResourceType::SHADER, shadername);

            auto img_size = layer->image->getSize();
            u32 atlas_size = layer->atlas->getTextureSize();
            auto atlas_tile = layer->atlas->getTileByImage(layer->image);

            for (u32 k = 0; k < buffer->getVertexCount(); k++) {
                // Apply material type
                svtSetMType(buffer, layer->material_type, k);
                // Apply hw color
                if (layer->material_flags & MATERIAL_FLAG_HARDWARE_COLORIZED)
                    svtSetHWColor(buffer, layer->color, k);

                // Calculate uvs in the given atlas
                u32 rel_x = round32(svtGetUV(buffer, k).X * img_size.X);
                u32 rel_y = round32(svtGetUV(buffer, k).Y * img_size.Y);

                v2f atlas_uv(
                    (atlas_tile->pos.X + rel_x) / atlas_size,
                    (atlas_tile->pos.Y + rel_y) / atlas_size
                );

                svtSetUV(buffer, atlas_uv, k);
            }
        }
    }

    m_mesh->splitTransparentLayers();
}

MapBlockMesh::~MapBlockMesh()
{
    u32 sz = 0;
    for (u8 buf_i = 0; buf_i < m_mesh->getBuffersCount(); buf_i++) {
        sz += m_mesh->getBuffer(buf_i)->getVertexCount();
	}

	porting::TrackFreedMemory(sz);
}

void MapBlockMesh::updateIndexBuffers(v3s16 camera_pos)
{
    m_mesh->transparentSort(intToFloat(camera_pos, BS));
    m_mesh->updateIndexBuffers();
}
