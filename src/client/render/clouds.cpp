// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "clouds.h"
#include "client/render/sky.h"
#include "constants.h"
#include "debug.h"
#include <Utils/Printing.h>
#include "noise.h"
#include "profiler.h"
#include "settings.h"
#include "client/mesh/meshbuffer.h"
#include "rendersystem.h"
#include "client/media/resource.h"
#include <Image/Converting.h>
#include "batcher3d.h"
#include "renderer.h"
#include "client/mesh/defaultVertexTypes.h"

// Constant for now
static constexpr const float cloud_size = BS * 64.0f;

static void cloud_3d_setting_changed(const std::string &settingname, void *data)
{
	((Clouds *)data)->readSettings();
}

Clouds::Clouds(RenderSystem *rndsys, ResourceCache *cache, u32 seed)
    : m_rndsys(rndsys), m_cache(cache), m_seed(seed)
{
	m_params = SkyboxDefaults::getCloudDefaults();

	readSettings();
	g_settings->registerChangedCallback("enable_3d_clouds",
		&cloud_3d_setting_changed, this);
	g_settings->registerChangedCallback("soft_clouds",
		&cloud_3d_setting_changed, this);

	updateBox();

    m_mesh = std::make_unique<MeshBuffer>(true, SkyboxVType, render::MeshUsage::DYNAMIC);

    m_shader = m_cache->getOrLoad<render::Shader>(ResourceType::SHADER, "skybox");
}

Clouds::~Clouds()
{
	g_settings->deregisterAllChangedCallbacks(this);
}

void Clouds::updateMesh()
{
	// Clouds move from Z+ towards Z-

	v2f camera_pos_2d(m_camera_pos.X, m_camera_pos.Z);
	// Position of cloud noise origin from the camera
	v2f cloud_origin_from_camera_f = m_origin - camera_pos_2d;
	// The center point of drawing in the noise
	v2f center_of_drawing_in_noise_f = -cloud_origin_from_camera_f;
	// The integer center point of drawing in the noise
	v2s16 center_of_drawing_in_noise_i(
		std::floor(center_of_drawing_in_noise_f.X / cloud_size),
		std::floor(center_of_drawing_in_noise_f.Y / cloud_size)
	);

	// Only update mesh if it has moved enough, this saves lots of GPU buffer uploads.
	constexpr float max_d = 5 * BS;

	if (!m_mesh_valid) {
		// mesh was never created or invalidated
	} else if (m_mesh_origin.getDistanceFrom(m_origin) >= max_d) {
		// clouds moved
	} else if (center_of_drawing_in_noise_i != m_last_noise_center) {
		// noise offset changed
		// I think in practice this never happens due to the camera offset
		// being smaller than the cloud size.(?)
	} else {
		return;
	}

	ScopeProfiler sp(g_profiler, "Clouds::updateMesh()", SPT_AVG);
	m_mesh_origin = m_origin;
	m_last_noise_center = center_of_drawing_in_noise_i;
	m_mesh_valid = true;

	const u32 num_faces_to_draw = is3D() ? 6 : 1;

	// The world position of the integer center point of drawing in the noise
	v2f world_center_of_drawing_in_noise_f = v2f(
		center_of_drawing_in_noise_i.X * cloud_size,
		center_of_drawing_in_noise_i.Y * cloud_size
	) + m_origin;

	// Colors with primitive shading

    img::colorf c_top_f(img::PF_RGBA32F, 1, 1, 1, 1);
    img::colorf c_side_1_f(img::PF_RGBA32F, 1, 1, 1, 1);
    img::colorf c_side_2_f(img::PF_RGBA32F, 1, 1, 1, 1);
    img::colorf c_bottom_f(img::PF_RGBA32F, 1, 1, 1, 1);
    const img::colorf shadow = img::color8ToColorf(m_params.color_shadow);

    c_side_1_f.R(c_side_1_f.R() * shadow.R() * 0.25f + 0.75f);
    c_side_1_f.G(c_side_1_f.G() * shadow.G() * 0.25f + 0.75f);
    c_side_1_f.B(c_side_1_f.B() * shadow.B() * 0.25f + 0.75f);
    c_side_2_f.R(c_side_1_f.R() * shadow.R() * 0.5f + 0.5f);
    c_side_2_f.G(c_side_1_f.G() * shadow.G() * 0.5f + 0.5f);
    c_side_2_f.B(c_side_1_f.B() * shadow.B() * 0.5f + 0.5f);
    c_bottom_f.R(c_bottom_f.R() * shadow.R());
    c_bottom_f.G(c_bottom_f.G() * shadow.G());
    c_bottom_f.B(c_bottom_f.B() * shadow.B());

    img::color8 c_top = img::colorfToColor8(c_top_f);
    img::color8 c_side_1 = img::colorfToColor8(c_side_1_f);
    img::color8 c_side_2 = img::colorfToColor8(c_side_2_f);
    img::color8 c_bottom = img::colorfToColor8(c_bottom_f);

	// Read noise

	std::vector<bool> grid(m_cloud_radius_i * 2 * m_cloud_radius_i * 2);

	for(s16 zi = -m_cloud_radius_i; zi < m_cloud_radius_i; zi++) {
		u32 si = (zi + m_cloud_radius_i) * m_cloud_radius_i * 2 + m_cloud_radius_i;

		for (s16 xi = -m_cloud_radius_i; xi < m_cloud_radius_i; xi++) {
			u32 i = si + xi;

			grid[i] = gridFilled(
				xi + center_of_drawing_in_noise_i.X,
				zi + center_of_drawing_in_noise_i.Y
			);
		}
	}

    m_mesh->clear();
	{
		const u32 vertex_count = num_faces_to_draw * 16 * m_cloud_radius_i * m_cloud_radius_i;
		const u32 quad_count = vertex_count / 4;
		const u32 index_count = quad_count * 6;

		// reserve memory
        m_mesh->reallocateData(vertex_count, index_count);
	}

#define GETINDEX(x, z, radius) (((z)+(radius))*(radius)*2 + (x)+(radius))
#define INAREA(x, z, radius) \
	((x) >= -(radius) && (x) < (radius) && (z) >= -(radius) && (z) < (radius))

	for (s16 zi0= -m_cloud_radius_i; zi0 < m_cloud_radius_i; zi0++)
	for (s16 xi0= -m_cloud_radius_i; xi0 < m_cloud_radius_i; xi0++)
	{
		s16 zi = zi0;
		s16 xi = xi0;
		// Draw from back to front for proper transparency
		if(zi >= 0)
			zi = m_cloud_radius_i - zi - 1;
		if(xi >= 0)
			xi = m_cloud_radius_i - xi - 1;

		u32 i = GETINDEX(xi, zi, m_cloud_radius_i);

		if (!grid[i])
			continue;

		v2f p0 = v2f(xi,zi)*cloud_size + world_center_of_drawing_in_noise_f;

        std::array<v3f, 4> positions;
        std::array<v3f, 4> normals;
        std::array<img::color8, 4> colors;

		const f32 rx = cloud_size / 2.0f;
		// if clouds are flat, the top layer should be at the given height
		const f32 ry = is3D() ? m_params.thickness * BS : 0.0f;
		const f32 rz = cloud_size / 2;

		bool soft_clouds_enabled = g_settings->getBool("soft_clouds");
		for (u32 i = 0; i < num_faces_to_draw; i++)
		{
			switch (i)
			{
			case 0:	// top
                for (v3f &n : normals) {
                    n = v3f(0, 1, 0);
				}
                positions[0] = v3f(-rx, ry,-rz);
                positions[1] = v3f(-rx, ry, rz);
                positions[2] = v3f(rx, ry, rz);
                positions[3] = v3f(rx, ry,-rz);
				break;
			case 1: // back
				if (INAREA(xi, zi - 1, m_cloud_radius_i)) {
					u32 j = GETINDEX(xi, zi - 1, m_cloud_radius_i);
					if (grid[j])
						continue;
				}
				if (soft_clouds_enabled) {
                    for (v3f &n : normals) {
                        n = v3f(0, 0, -1);
					}
                    colors[2] = c_bottom;
                    colors[3] = c_bottom;
				} else {
                    for (u32 i = 0; i < 4; i++) {
                        colors[i] = c_side_1;
                        normals[i] = v3f(0, 0, -1);
					}
				}
                positions[0] = v3f(-rx, ry,-rz);
                positions[1] = v3f(rx, ry,-rz);
                positions[2] = v3f(rx,  0,-rz);
                positions[3] = v3f(-rx,  0,-rz);
				break;
			case 2: //right
				if (INAREA(xi + 1, zi, m_cloud_radius_i)) {
					u32 j = GETINDEX(xi + 1, zi, m_cloud_radius_i);
					if (grid[j])
						continue;
				}
				if (soft_clouds_enabled) {
                    for (v3f &n : normals) {
                        n = v3f(1, 0, 0);
					}
                    colors[2] = c_bottom;
                    colors[3] = c_bottom;
				}
				else {
                    for (u32 i = 0; i < 4; i++) {
                        colors[i] = c_side_2;
                        normals[i] = v3f(1, 0, 0);
                    }
				}
                positions[0] = v3f(rx, ry,-rz);
                positions[1] = v3f(rx, ry, rz);
                positions[2] = v3f(rx,  0, rz);
                positions[3] = v3f(rx,  0,-rz);
				break;
			case 3: // front
				if (INAREA(xi, zi + 1, m_cloud_radius_i)) {
					u32 j = GETINDEX(xi, zi + 1, m_cloud_radius_i);
					if (grid[j])
						continue;
				}
				if (soft_clouds_enabled) {
                    for (v3f &n : normals) {
                        n = v3f(0, 0, -1);
					}
                    colors[2] = c_bottom;
                    colors[3] = c_bottom;
				} else {
                    for (u32 i = 0; i < 4; i++) {
                        colors[i] = c_side_1;
                        normals[i] = v3f(0, 0, -1);
                    }
				}
                positions[0] = v3f(rx, ry, rz);
                positions[1] = v3f(-rx, ry, rz);
                positions[2] = v3f(-rx,  0, rz);
                positions[3] = v3f(rx,  0, rz);
				break;
			case 4: // left
				if (INAREA(xi - 1, zi, m_cloud_radius_i)) {
					u32 j = GETINDEX(xi - 1, zi, m_cloud_radius_i);
					if (grid[j])
						continue;
				}
				if (soft_clouds_enabled) {
                    for (v3f &n : normals) {
                        n = v3f(-1, 0, 0);
					}
                    colors[2] = c_bottom;
                    colors[3] = c_bottom;
				} else {
                    for (u32 i = 0; i < 4; i++) {
                        colors[i] = c_side_2;
                        normals[i] = v3f(-1, 0, 0);
                    }
				}
                positions[0] = v3f(-rx, ry, rz);
                positions[1] = v3f(-rx, ry,-rz);
                positions[2] = v3f(-rx,  0,-rz);
                positions[3] = v3f(-rx,  0, rz);
				break;
			case 5: // bottom
                for (u32 i = 0; i < 4; i++) {
                    colors[i] = c_bottom;
                    normals[i] = v3f(0, -1, 0);
                }
                positions[0] = v3f(rx,  0, rz);
                positions[1] = v3f(-rx,  0, rz);
                positions[2] = v3f(-rx,  0,-rz);
                positions[3] = v3f(rx,  0,-rz);
				break;
			}

			v3f pos(p0.X, m_params.height * BS, p0.Y);

            for (u32 i = 0; i < 4; i++) {
                positions[i] += pos;
            }
            Batcher3D::appendFace(m_mesh.get(), positions, colors);
		}
	}

    tracestream << "Cloud::updateMesh(): " << m_mesh->getVertexCount() << " vertices"
		<< std::endl;
}

void Clouds::render(v3s16 camera_offset)
{
    if (!m_visible || m_params.density <= 0.0f)
        return; // no need to do anything

	//updateMesh();

    auto rnd = m_rndsys->getRenderer();
    rnd->setRenderState(true);

    v2f off_origin = m_origin - m_mesh_origin;
    v3f rel(off_origin.X, 0, off_origin.Y);
    rel -= intToFloat(camera_offset, BS);
    matrix4 translate;
    translate.setTranslation(rel);

    rnd->setTransformMatrix(TMatrix::World, translate);
    rnd->setShader(m_shader);
    rnd->setUniformBlocks();
    rnd->enableFog(true);

    rnd->draw(m_mesh.get());
    //driver->drawMeshBuffer(m_meshbuffer.get());

	// Restore fog settings
    //driver->setFog(fog_color, fog_type, fog_start, fog_end, fog_density,
    //		fog_pixelfog, fog_rangefog);
}

void Clouds::step(float dtime)
{
    m_origin = m_origin + m_params.speed * dtime * BS;
}

//void Clouds::update(const v3f &camera_p, v3s16 camera_offset, const img::colorf &color_diffuse)
void Clouds::update(f32 dtime, Camera *camera, Sky *sky, f32 &fog_range,
    std::optional<img::color8> color_override)
{
    v3f camera_pos = camera->getPosition();
    v3s16 camera_offset = camera->getOffset();
    if (sky) {
        if (!sky->getCloudsVisible()) {
            m_visible = false;
            return;
        }

        m_visible = true;
        step(dtime);
        // this->camera->getPosition is not enough for third-person camera.
        camera_pos.X   = camera_pos.X + camera_offset.X * BS;
        camera_pos.Y   = camera_pos.Y + camera_offset.Y * BS;
        camera_pos.Z   = camera_pos.Z + camera_offset.Z * BS;
    }

    img::colorf ambient = img::color8ToColorf(m_params.color_ambient);
    img::colorf bright = img::color8ToColorf(m_params.color_bright);

    img::colorf cloud_color = sky->getCloudColor();
    m_color.R(std::clamp(cloud_color.R() * bright.G(), ambient.R(), 1.0f));
    m_color.G(std::clamp(cloud_color.G() * bright.G(), ambient.G(), 1.0f));
    m_color.B(std::clamp(cloud_color.B() * bright.B(), ambient.B(), 1.0f));
    m_color.A(bright.A());

	// is the camera inside the cloud mesh?
    m_camera_pos = camera_pos;
	m_camera_inside_cloud = false; // default

    updateBox();
	if (is3D()) {
        float camera_height = camera_pos.Y - BS * camera_offset.Y;
		if (camera_height >= m_box.MinEdge.Y &&
				camera_height <= m_box.MaxEdge.Y) {
			v2f camera_in_noise;
            camera_in_noise.X = floor((camera_pos.X - m_origin.X) / cloud_size + 0.5);
            camera_in_noise.Y = floor((camera_pos.Z - m_origin.Y) / cloud_size + 0.5);
			bool filled = gridFilled(camera_in_noise.X, camera_in_noise.Y);
			m_camera_inside_cloud = filled;
		}
	}

    updateMesh();

    for (u32 i = 0; i < m_mesh->getVertexCount(); i++)
        svtSetHWColor(m_mesh.get(), m_color, i);

    if (sky) {
        if (isCameraInsideCloud() && m_rndsys->getRenderer()->fogEnabled()) {
            // If camera is inside cloud and fog is enabled, use cloud's colors as sky colors.
            img::color8 clouds_dark = getColor().linInterp(img::black, 0.9);
            sky->overrideColors(clouds_dark, getColor());
            sky->setInClouds(true);
            fog_range = std::fmin(fog_range * 0.5f, 32.0f * BS);
            // Clouds are not drawn in this case.
            m_visible = false;
        }
    }
}

void Clouds::readSettings()
{
	// The code isn't designed to go over 64k vertices so the upper limits were
	// chosen to avoid exactly that.
	// refer to vertex_count in updateMesh()
	m_enable_3d = g_settings->getBool("enable_3d_clouds");
	const u16 maximum = !m_enable_3d ? 62 : 25;
	m_cloud_radius_i = rangelim(g_settings->getU16("cloud_radius"), 8, maximum);

	invalidateMesh();
}

bool Clouds::gridFilled(int x, int y) const
{
	float cloud_size_noise = cloud_size / (BS * 200.f);
	float noise = noise2d_perlin(
			(float)x * cloud_size_noise,
			(float)y * cloud_size_noise,
			m_seed, 3, 0.5);
	// normalize to 0..1 (given 3 octaves)
	static constexpr const float noise_bound = 1.0f + 0.5f + 0.25f;
	float density = noise / noise_bound * 0.5f + 0.5f;
	return (density < m_params.density);
}
