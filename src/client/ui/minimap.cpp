// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2015 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "minimap.h"
#include <cmath>
#include "client/core/client.h"
#include "client/mesh/defaultVertexTypes.h"
#include "client/render/atlas.h"
#include "client/ui/extra_images.h"
#include "nodedef.h"
#include "client/map/clientmap.h"
#include "scripting_client.h"
#include "settings.h"
#include "mapblock.h"
#include "gettext.h"
#include "client/render/renderer.h"
#include "client/media/resource.h"
#include "batcher2d.h"
#include "voxel.h"
#include <Render/DrawContext.h>
#include "client/player/playercamera.h"
#include "client/render/rendersystem.h"


////
//// MinimapUpdateThread
////

MinimapUpdateThread::~MinimapUpdateThread()
{
	for (auto &it : m_blocks_cache) {
		delete it.second;
	}

	for (auto &q : m_update_queue) {
		delete q.data;
	}
}

bool MinimapUpdateThread::pushBlockUpdate(v3s16 pos, MinimapMapblock *data)
{
	MutexAutoLock lock(m_queue_mutex);

	// Find if block is already in queue.
	// If it is, update the data and quit.
	for (QueuedMinimapUpdate &q : m_update_queue) {
		if (q.pos == pos) {
			delete q.data;
			q.data = data;
			return false;
		}
	}

	// Add the block
	QueuedMinimapUpdate q;
	q.pos  = pos;
	q.data = data;
	m_update_queue.push_back(q);

	return true;
}

bool MinimapUpdateThread::popBlockUpdate(QueuedMinimapUpdate *update)
{
	MutexAutoLock lock(m_queue_mutex);

	if (m_update_queue.empty())
		return false;

	*update = m_update_queue.front();
	m_update_queue.pop_front();

	return true;
}

void MinimapUpdateThread::enqueueBlock(v3s16 pos, MinimapMapblock *data)
{
	pushBlockUpdate(pos, data);
	deferUpdate();
}


void MinimapUpdateThread::doUpdate()
{
	QueuedMinimapUpdate update;

	while (popBlockUpdate(&update)) {
		if (update.data) {
			// Swap two values in the map using single lookup
			std::pair<std::map<v3s16, MinimapMapblock*>::iterator, bool>
			    result = m_blocks_cache.insert(std::make_pair(update.pos, update.data));
			if (!result.second) {
				delete result.first->second;
				result.first->second = update.data;
			}
		} else {
			std::map<v3s16, MinimapMapblock *>::iterator it;
			it = m_blocks_cache.find(update.pos);
			if (it != m_blocks_cache.end()) {
				delete it->second;
				m_blocks_cache.erase(it);
			}
		}
	}


	if (data->map_invalidated && (
				data->mode.type == MINIMAP_TYPE_RADAR ||
				data->mode.type == MINIMAP_TYPE_SURFACE)) {
		getMap(data->pos, data->mode.map_size, data->mode.scan_height);
		data->map_invalidated = false;
	}
}

void MinimapUpdateThread::getMap(v3s16 pos, s16 size, s16 height)
{
	v3s16 pos_min(pos.X - size / 2, pos.Y - height / 2, pos.Z - size / 2);
	v3s16 pos_max(pos_min.X + size - 1, pos.Y + height / 2, pos_min.Z + size - 1);
	v3s16 blockpos_min = getNodeBlockPos(pos_min);
	v3s16 blockpos_max = getNodeBlockPos(pos_max);

// clear the map
	for (int z = 0; z < size; z++)
	for (int x = 0; x < size; x++) {
		MinimapPixel &mmpixel = data->minimap_scan[x + z * size];
		mmpixel.air_count = 0;
		mmpixel.height = 0;
		mmpixel.n = MapNode(CONTENT_AIR);
	}

// draw the map
	v3s16 blockpos;
	for (blockpos.Z = blockpos_min.Z; blockpos.Z <= blockpos_max.Z; ++blockpos.Z)
	for (blockpos.Y = blockpos_min.Y; blockpos.Y <= blockpos_max.Y; ++blockpos.Y)
	for (blockpos.X = blockpos_min.X; blockpos.X <= blockpos_max.X; ++blockpos.X) {
		std::map<v3s16, MinimapMapblock *>::const_iterator pblock =
			m_blocks_cache.find(blockpos);
		if (pblock == m_blocks_cache.end())
			continue;
		const MinimapMapblock &block = *pblock->second;

		v3s16 block_node_min(blockpos * MAP_BLOCKSIZE);
		v3s16 block_node_max(block_node_min + MAP_BLOCKSIZE - 1);
		// clip
		v3s16 range_min = componentwise_max(block_node_min, pos_min);
		v3s16 range_max = componentwise_min(block_node_max, pos_max);

		v3s16 pos;
		pos.Y = range_min.Y;
		for (pos.Z = range_min.Z; pos.Z <= range_max.Z; ++pos.Z)
		for (pos.X = range_min.X; pos.X <= range_max.X; ++pos.X) {
			v3s16 inblock_pos = pos - block_node_min;
			const MinimapPixel &in_pixel =
				block.data[inblock_pos.Z * MAP_BLOCKSIZE + inblock_pos.X];

			v3s16 inmap_pos = pos - pos_min;
			MinimapPixel &out_pixel =
				data->minimap_scan[inmap_pos.X + inmap_pos.Z * size];

			out_pixel.air_count += in_pixel.air_count;
			if (in_pixel.n.param0 != CONTENT_AIR) {
				out_pixel.n = in_pixel.n;
				out_pixel.height = inmap_pos.Y + in_pixel.height;
			}
		}
	}
}

////
//// Mapper
////

Minimap::Minimap(Client *_client, UIRects *_rect)
    //: UISprite(nullptr, _rect, _cache, rectf(), rectf(v2f(-1.0f, 1.0f), v2f(1.0f, -1.0f)), {}, false),
    : client(_client), data(new MinimapData()), m_renderer(client->getRenderSystem()->getRenderer()),
    m_cache(client->getResourceCache()), m_rect(_rect),
    m_buffer(std::make_unique<MeshBuffer>(4, 6, true, VType2D)), m_ndef(_client->getNodeDefManager())
{
	// Initialize static settings
    m_surface_mode_scan_height = g_settings->getBool("minimap_double_scan_height") ? 256 : 128;

	// Initialize minimap modes
	addMode(MINIMAP_TYPE_OFF);
	addMode(MINIMAP_TYPE_SURFACE, 256);
	addMode(MINIMAP_TYPE_SURFACE, 128);
	addMode(MINIMAP_TYPE_SURFACE, 64);
	addMode(MINIMAP_TYPE_RADAR,   512);
	addMode(MINIMAP_TYPE_RADAR,   256);
	addMode(MINIMAP_TYPE_RADAR,   128);

	data->minimap_shape_round = g_settings->getBool("minimap_shape_round");

	setModeIndex(0);

    data->minimap_overlay_round = m_cache->get<img::Image>(ResourceType::IMAGE, "minimap_overlay_round.png");
    data->minimap_overlay_square = m_cache->get<img::Image>(ResourceType::IMAGE, "minimap_overlay_square.png");
    data->player_marker = m_cache->get<img::Image>(ResourceType::IMAGE, "player_marker.png");
    data->object_marker_red = m_cache->get<img::Image>(ResourceType::IMAGE, "object_marker_red.png");
    data->textures_initialised = true;

    m_minimap_shader = m_cache->getOrLoad<render::Shader>(ResourceType::SHADER, "minimap");

    m_rect->getShape().updateBuffer(m_buffer.get());

    if (client->modsLoaded()) {
        client->getScript()->on_minimap_ready(this);
    }

	// Initialize and start thread
	m_minimap_update_thread = std::make_unique<MinimapUpdateThread>();
	m_minimap_update_thread->data = data.get();
	m_minimap_update_thread->start();
}

Minimap::~Minimap()
{
	m_minimap_update_thread->stop();
	m_minimap_update_thread->wait();

	if (data->minimap_mask_round)
        m_cache->clearResource<img::Image>(ResourceType::IMAGE, data->minimap_mask_round, true);
	if (data->minimap_mask_square)
        m_cache->clearResource<img::Image>(ResourceType::IMAGE, data->minimap_mask_square, true);

    // Dynamic textures are not cached
    delete data->texture;
    delete data->heightmap_texture;

	m_minimap_update_thread.reset();
}

void Minimap::addBlock(v3s16 pos, MinimapMapblock *data)
{
	m_minimap_update_thread->enqueueBlock(pos, data);
}

void Minimap::toggleMinimapShape()
{
	MutexAutoLock lock(m_mutex);

	data->minimap_shape_round = !data->minimap_shape_round;
	g_settings->setBool("minimap_shape_round", data->minimap_shape_round);
	m_minimap_update_thread->deferUpdate();
}

void Minimap::setMinimapShape(MinimapShape shape)
{
	MutexAutoLock lock(m_mutex);

	if (shape == MINIMAP_SHAPE_SQUARE)
		data->minimap_shape_round = false;
	else if (shape == MINIMAP_SHAPE_ROUND)
		data->minimap_shape_round = true;

	g_settings->setBool("minimap_shape_round", data->minimap_shape_round);
	m_minimap_update_thread->deferUpdate();
}

MinimapShape Minimap::getMinimapShape()
{
    return data->minimap_shape_round ? MINIMAP_SHAPE_ROUND : MINIMAP_SHAPE_SQUARE;
}

void Minimap::setModeIndex(size_t index)
{
	MutexAutoLock lock(m_mutex);

	if (index < m_modes.size()) {
		data->mode = m_modes[index];
		m_current_mode_index = index;
	} else {
		data->mode = {MINIMAP_TYPE_OFF, gettext("Minimap hidden"), 0, 0, "", 0};
		m_current_mode_index = 0;
	}

	data->map_invalidated = true;

	if (m_minimap_update_thread)
		m_minimap_update_thread->deferUpdate();
}

void Minimap::addMode(MinimapModeDef mode)
{
	// Check validity
	if (mode.type == MINIMAP_TYPE_TEXTURE) {
		if (mode.texture.empty())
			return;
		if (mode.scale < 1)
			mode.scale = 1;
	}

	int zoom = -1;

	// Build a default standard label
	if (mode.label.empty()) {
		switch (mode.type) {
			case MINIMAP_TYPE_OFF:
				mode.label = gettext("Minimap hidden");
				break;
			case MINIMAP_TYPE_SURFACE:
				mode.label = gettext("Minimap in surface mode, Zoom x%d");
				if (mode.map_size > 0)
					zoom = 256 / mode.map_size;
				break;
			case MINIMAP_TYPE_RADAR:
				mode.label = gettext("Minimap in radar mode, Zoom x%d");
				if (mode.map_size > 0)
					zoom = 512 / mode.map_size;
				break;
			case MINIMAP_TYPE_TEXTURE:
				mode.label = gettext("Minimap in texture mode");
				break;
			default:
				break;
		}
	}
	// else: Custom labels need mod-provided client-side translation

	if (zoom >= 0) {
		char label_buf[1024];
		porting::mt_snprintf(label_buf, sizeof(label_buf),
			mode.label.c_str(), zoom);
		mode.label = label_buf;
	}

	m_modes.push_back(mode);
}

void Minimap::addMode(MinimapType type, u16 size, const std::string &label,
		const std::string &texture, u16 scale)
{
	MinimapModeDef mode;
	mode.type = type;
	mode.label = label;
	mode.map_size = size;
	mode.texture = texture;
	mode.scale = scale;
	switch (type) {
		case MINIMAP_TYPE_SURFACE:
			mode.scan_height = m_surface_mode_scan_height;
			break;
		case MINIMAP_TYPE_RADAR:
			mode.scan_height = 32;
			break;
		default:
			mode.scan_height = 0;
	}
	addMode(mode);
}

void Minimap::nextMode()
{
	if (m_modes.empty())
		return;
	m_current_mode_index++;
	if (m_current_mode_index >= m_modes.size())
		m_current_mode_index = 0;

	setModeIndex(m_current_mode_index);
}

void Minimap::setPos(v3s16 pos)
{
	bool do_update = false;

	{
		MutexAutoLock lock(m_mutex);

		if (pos != data->old_pos) {
			data->old_pos = data->pos;
			data->pos = pos;
			do_update = true;
		}
	}

	if (do_update)
		m_minimap_update_thread->deferUpdate();
}

void Minimap::setAngle(f32 angle)
{
	m_angle = angle;
}

void Minimap::blitMinimapPixelsToImageRadar(img::Image *map_image)
{
    img::color8 c(img::PF_RGBA8, 0, 0, 0, 240);
	for (s16 x = 0; x < data->mode.map_size; x++)
	for (s16 z = 0; z < data->mode.map_size; z++) {
		MinimapPixel *mmpixel = &data->minimap_scan[x + z * data->mode.map_size];

		if (mmpixel->air_count > 0)
            c.G(32 + mmpixel->air_count * 8);
		else
            c.G(0);

        g_imgmodifier->setPixelDirect(map_image, x, data->mode.map_size - z - 1, c);
	}
}

void Minimap::blitMinimapPixelsToImageSurface(
    img::Image *map_image, img::Image *heightmap_image)
{
	// This variable creation/destruction has a 1% cost on rendering minimap
    img::color8 tilecolor(img::PF_RGBA8, 0, 0, 0, 240);
	for (s16 x = 0; x < data->mode.map_size; x++)
	for (s16 z = 0; z < data->mode.map_size; z++) {
		MinimapPixel *mmpixel = &data->minimap_scan[x + z * data->mode.map_size];

		const ContentFeatures &f = m_ndef->get(mmpixel->n);
		const TileDef *tile = &f.tiledef[0];

		// Color of the 0th tile (mostly this is the topmost)
		if(tile->has_color)
			tilecolor = tile->color;
		else
			mmpixel->n.getColor(f, &tilecolor);

        tilecolor.R(tilecolor.R() * f.minimap_color.R());
        tilecolor.G(tilecolor.G() * f.minimap_color.G());
        tilecolor.B(tilecolor.B() * f.minimap_color.B());

        g_imgmodifier->setPixelDirect(map_image, x, data->mode.map_size - z - 1, tilecolor);
        img::color8 height(img::PF_R8, (u8)mmpixel->height);
        g_imgmodifier->setPixelDirect(heightmap_image, x, data->mode.map_size - z - 1, height);
	}
}

img::Image *Minimap::getMinimapMask()
{
	if (data->minimap_shape_round) {
		if (!data->minimap_mask_round) {
			// Get round minimap textures
            data->minimap_mask_round = m_cache->get<img::Image>(ResourceType::IMAGE, "minimap_mask_round.png");
		}
		return data->minimap_mask_round;
	}

	if (!data->minimap_mask_square) {
		// Get square minimap textures
        data->minimap_mask_square = m_cache->get<img::Image>(ResourceType::IMAGE, "minimap_mask_square.png");
	}
	return data->minimap_mask_square;
}

render::Texture2D *Minimap::getMinimapTexture()
{
	// update minimap textures when new scan is ready
	if (data->map_invalidated && data->mode.type != MINIMAP_TYPE_TEXTURE)
		return data->texture;

	// create minimap and heightmap images in memory
    v2u size(data->mode.map_size, data->mode.map_size);
    img::Image *map_image       = new img::Image(img::PF_RGBA8, size.X, size.Y, img::black);
    img::Image *heightmap_image = new img::Image(img::PF_R8, size.X, size.Y);
    img::Image *minimap_image   = new img::Image(img::PF_RGBA8, MINIMAP_MAX_SX, MINIMAP_MAX_SY);

	// Blit MinimapPixels to images
	switch(data->mode.type) {
	case MINIMAP_TYPE_OFF:
		break;
	case MINIMAP_TYPE_SURFACE:
		blitMinimapPixelsToImageSurface(map_image, heightmap_image);
		break;
	case MINIMAP_TYPE_RADAR:
		blitMinimapPixelsToImageRadar(map_image);
		break;
	case MINIMAP_TYPE_TEXTURE:
		// FIXME: this is a pointless roundtrip through the gpu
        img::Image* image = m_cache->getOrLoad<img::Image>(ResourceType::IMAGE, data->mode.texture);

        v2u size = image->getSize();
        rectu destRect(
            v2u(((data->mode.map_size - (static_cast<int>(size.X))) >> 1)
                    - data->pos.X / data->mode.scale,
                ((data->mode.map_size - (static_cast<int>(size.Y))) >> 1)
                    + data->pos.Z / data->mode.scale),
            size
        );
        g_imgmodifier->copyTo(image, map_image, nullptr, &destRect);

        m_cache->clearResource<img::Image>(ResourceType::IMAGE, image, true);
	}

    g_imgmodifier->copyTo(map_image, minimap_image, nullptr, nullptr, true);
    delete map_image;

    img::Image *minimap_mask = getMinimapMask();

    img::color8 mask_col;
	for (s16 y = 0; y < MINIMAP_MAX_SY; y++)
	for (s16 x = 0; x < MINIMAP_MAX_SX; x++) {
        g_imgmodifier->getPixelDirect(minimap_mask, x, y, mask_col);
        if (!mask_col.A())
            g_imgmodifier->setPixelDirect(minimap_image, x, y, img::black);
	}

	if (data->texture)
        delete data->texture;
	if (data->heightmap_texture)
        delete data->heightmap_texture;

    render::TextureSettings settings;
    settings.minF = render::TMF_LINEAR_MIPMAP_LINEAR;
    settings.magF = render::TMAGF_LINEAR;
    settings.isRenderTarget = false;
    data->texture = new render::Texture2D("minimap__",
        std::unique_ptr<img::Image>(minimap_image), settings);
    data->heightmap_texture = new render::Texture2D("minimap_heightmap__",
        std::unique_ptr<img::Image>(heightmap_image), settings);

	data->map_invalidated = true;

	return data->texture;
}

v3f Minimap::getYawVec()
{
	if (data->minimap_shape_round) {
		return v3f(
            std::cos(degToRad(m_angle)),
            std::sin(degToRad(m_angle)),
			1.0);
	}

	return v3f(1.0, 0.0, 1.0);
}

void Minimap::drawMinimap(recti rect)
{
	if (data->mode.type == MINIMAP_TYPE_OFF)
		return;

	// Get textures
    render::Texture2D *minimap_texture = getMinimapTexture();
	if (!minimap_texture)
		return;

    auto ctxt = m_renderer->getContext();
    recti oldViewPort = ctxt->getViewportSize();

    matrix4 oldProjMat = m_renderer->getTransformMatrix(TMatrix::Projection);
    matrix4 oldViewMat = m_renderer->getTransformMatrix(TMatrix::View);

    ctxt->setViewportSize(rect);
    m_renderer->setTransformMatrix(TMatrix::Projection, matrix4());
    m_renderer->setTransformMatrix(TMatrix::View, matrix4());

    m_renderer->setRenderState(false);

    matrix4 matrix;
    matrix.makeIdentity();

    if (data->mode.type == MINIMAP_TYPE_SURFACE) {
        m_renderer->setShader(m_minimap_shader);
        m_minimap_shader->setUniform3Float("mYawVec", getYawVec());
	} else {
        m_renderer->setDefaultShader(true, false);
        m_renderer->setDefaultUniforms(1.0f, 1, 0.5f, img::BM_COUNT);
	}

    m_renderer->setClipRect(recti());
	
	ctxt->setActiveUnit(0, minimap_texture);
    ctxt->setActiveUnit(1, data->heightmap_texture);

	if (data->minimap_shape_round)
        matrix.setRotationDegrees(v3f(0, 0, 360 - m_angle));

	// Draw minimap
    if (data->mode.type == MINIMAP_TYPE_SURFACE)
        m_minimap_shader->setUniform4x4Matrix("mWorld", matrix);
    else
        m_renderer->setTransformMatrix(TMatrix::World, matrix);

    m_renderer->draw(m_buffer.get());

    auto rndsys = client->getRenderSystem();
    auto guiPool = rndsys->getPool(false);

	// Draw overlay
    auto minimap_overlay = data->minimap_shape_round ?
		data->minimap_overlay_round : data->minimap_overlay_square;
    m_renderer->setDefaultShader(true, false);
    m_renderer->setDefaultUniforms(1.0f, 1, 0.5f, img::BM_COUNT);

    rndsys->activateAtlas(minimap_overlay, false);
    MeshOperations::recalculateMeshAtlasUVs(m_buffer.get(), 0, 6,
        guiPool->getAtlasByTile(minimap_overlay)->getTextureSize(), guiPool->getTileRect(minimap_overlay),
        minimap_texture->getWidth(), rectf(v2f(), toV2T<f32>(minimap_texture->getSize())));
    m_buffer->uploadVertexData();

    m_renderer->draw(m_buffer.get());

	// Draw player marker on minimap
	if (data->minimap_shape_round) {
        matrix.setRotationDegrees(v3f(0, 0, 0));
	} else {
        matrix.setRotationDegrees(v3f(0, 0, m_angle));
	}

    rndsys->activateAtlas(data->player_marker, false);
    m_renderer->setTransformMatrix(TMatrix::World, matrix);

    MeshOperations::recalculateMeshAtlasUVs(m_buffer.get(), 0, 6,
        guiPool->getAtlasByTile(data->player_marker)->getTextureSize(), guiPool->getTileRect(data->player_marker),
        guiPool->getAtlasByTile(minimap_overlay)->getTextureSize(), guiPool->getTileRect(minimap_overlay));
    m_buffer->uploadVertexData();

    m_renderer->draw(m_buffer.get());

	// Reset transformations
    m_renderer->setTransformMatrix(TMatrix::Projection, oldProjMat);
    m_renderer->setTransformMatrix(TMatrix::View, oldViewMat);
    ctxt->setViewportSize(oldViewPort);

	// Draw player markers
    m_renderer->draw(m_buffer.get());

    MeshOperations::recalculateMeshAtlasUVs(m_buffer.get(), 0, 6,
        minimap_texture->getWidth(), rectf(v2f(), toV2T<f32>(minimap_texture->getSize())),
        guiPool->getAtlasByTile(data->player_marker)->getTextureSize(), guiPool->getTileRect(data->player_marker));
    m_buffer->uploadVertexData();
}

void Minimap::addMarker(v3f pos)
{
    m_markers.push_back(pos);
}

void Minimap::removeMarker(v3f pos)
{
    auto it = std::find(m_markers.begin(), m_markers.end(), pos);

    if (it != m_markers.end())
        m_markers.erase(it);
}

void Minimap::updateActiveMarkers(recti rect)
{
    img::Image *minimap_mask = getMinimapMask();

	m_active_markers.clear();

    // Clear all ealier added markers
    m_buffer->reallocateData(4, 6);
    // Then reserve a new storage for current markers
    m_buffer->reallocateData(4+m_markers.size()*4, 6+m_markers.size()*6);

    auto &shape = m_rect->getShape();
    for (u32 k = 1; k < shape.getPrimitiveCount(); k++)
        shape.removePrimitive(k);

    v3f cam_offset = intToFloat(client->getEnv().getLocalPlayer()->getCamera()->getOffset(), BS);
	v3s16 pos_offset = data->pos - v3s16(data->mode.map_size / 2,
			data->mode.scan_height / 2,
			data->mode.map_size / 2);

    v2i s_pos = rect.ULC;
    v2u imgsize = data->object_marker_red->getSize();
    rectf img_rect(0.0f, 0.0f, imgsize.X, imgsize.Y);
    const img::color8 col(img::PF_RGBA8, 255, 255, 255, 255);
    const std::array<img::color8, 4> c = {col, col, col, col};
    f32 sin_angle = std::sin(degToRad(m_angle));
    f32 cos_angle = std::cos(degToRad(m_angle));
    s32 marker_size2 =  0.025 * (f32)rect.getWidth();

    for (v3f marker : m_markers) {
        v3s16 pos = floatToInt(marker + cam_offset, BS) - pos_offset;
		if (pos.X < 0 || pos.X > data->mode.map_size ||
				pos.Y < 0 || pos.Y > data->mode.scan_height ||
				pos.Z < 0 || pos.Z > data->mode.map_size) {
			continue;
		}
        pos.X = ((f32)pos.X / data->mode.map_size) * MINIMAP_MAX_SX;
        pos.Z = ((f32)pos.Z / data->mode.map_size) * MINIMAP_MAX_SY;
        img::color8 mask_col;
        g_imgmodifier->getPixelDirect(minimap_mask, pos.X, pos.Z, mask_col);
        if (!mask_col.A()) {
			continue;
		}

        v2f am(((f32)pos.X / (f32)MINIMAP_MAX_SX) - 0.5,
            (1.0 - (f32)pos.Z / (f32)MINIMAP_MAX_SY) - 0.5);
        m_active_markers.push_back(am);

        if (data->minimap_shape_round) {
            f32 t1 = am.X * cos_angle - am.Y * sin_angle;
            f32 t2 = am.X * sin_angle + am.Y * cos_angle;
            am.X = t1;
            am.Y = t2;
        }
        am.X = (am.X + 0.5) * (f32)rect.getWidth();
        am.Y = (am.Y + 0.5) * (f32)rect.getHeight();
        rectf destRect(
            s_pos.X + am.X - marker_size2,
            s_pos.Y + am.Y - marker_size2,
            s_pos.X + am.X + marker_size2,
            s_pos.Y + am.Y + marker_size2);
        shape.addRectangle(destRect, c, img_rect);
	}

    shape.updateBuffer(m_buffer.get());

    m_buffer->uploadData();
}

////
//// MinimapMapblock
////

void MinimapMapblock::getMinimapNodes(VoxelManipulator *vmanip, const v3s16 &pos)
{

	for (s16 x = 0; x < MAP_BLOCKSIZE; x++)
	for (s16 z = 0; z < MAP_BLOCKSIZE; z++) {
		s16 air_count = 0;
		bool surface_found = false;
		MinimapPixel *mmpixel = &data[z * MAP_BLOCKSIZE + x];

		for (s16 y = MAP_BLOCKSIZE -1; y >= 0; y--) {
			v3s16 p(x, y, z);
			MapNode n = vmanip->getNodeNoEx(pos + p);
			if (!surface_found && n.getContent() != CONTENT_AIR) {
				mmpixel->height = y;
				mmpixel->n = n;
				surface_found = true;
			} else if (n.getContent() == CONTENT_AIR) {
				air_count++;
			}
		}

		if (!surface_found)
			mmpixel->n = MapNode(CONTENT_AIR);

		mmpixel->air_count = air_count;
	}
}
