// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include <BasicIncludes.h>
#include "constants.h"
#include "skyparams.h"
#include <iostream>
#include <Render/Shader.h>

class RenderSystem;
class ResourceCache;
class MeshBuffer;
class Camera;
class Sky;

// Menu clouds
// The mainmenu and the loading screen use the same Clouds object so that the
// clouds don't jump when switching between the two.

class Clouds
{
public:
    Clouds(RenderSystem *rndsys, ResourceCache *cache, u32 seed);

	~Clouds();

	/*
		ISceneNode methods
	*/

	/*virtual void OnRegisterSceneNode();

	virtual void render();

	virtual const aabb3f &getBoundingBox() const
	{
		return m_box;
	}

	virtual u32 getMaterialCount() const
	{
		return 1;
	}

	virtual video::SMaterial& getMaterial(u32 i)
	{
		return m_material;
	}*/

	/*
		Other stuff
	*/

	void step(float dtime);

    void update(f32 dtime, Camera *camera, Sky *sky, f32 &fog_range,
        std::optional<img::color8> color_override=std::nullopt);

    void render(v3s16 camera_offset);

	void readSettings();

	void setDensity(float density)
	{
		if (m_params.density == density)
			return;
		m_params.density = density;
		invalidateMesh();
	}

	void setColorBright(img::color8 color_bright)
	{
		m_params.color_bright = color_bright;
	}

	void setColorAmbient(img::color8 color_ambient)
	{
		m_params.color_ambient = color_ambient;
	}

	void setColorShadow(img::color8 color_shadow)
	{
		if (m_params.color_shadow == color_shadow)
			return;
		m_params.color_shadow = color_shadow;
		invalidateMesh();
	}

	void setHeight(float height)
	{
		if (m_params.height == height)
			return;
		m_params.height = height;
		updateBox();
		invalidateMesh();
	}

	void setSpeed(v2f speed)
	{
		m_params.speed = speed;
	}

	void setThickness(float thickness)
	{
		if (m_params.thickness == thickness)
			return;
		m_params.thickness = thickness;
		updateBox();
		invalidateMesh();
	}

	bool isCameraInsideCloud() const { return m_camera_inside_cloud; }

	const img::color8 getColor() const { return m_color; }

private:
	void updateBox()
	{
		float height_bs    = m_params.height    * BS;
		float thickness_bs = m_params.thickness * BS;
		m_box = aabbf(-BS * 1000000.0f, height_bs, -BS * 1000000.0f,
				BS * 1000000.0f, height_bs + thickness_bs, BS * 1000000.0f);
	}

	void updateMesh();
	void invalidateMesh()
	{
		m_mesh_valid = false;
	}

	bool gridFilled(int x, int y) const;

	// Are the clouds 3D?
	inline bool is3D() const {
		return m_enable_3d && m_params.thickness >= 0.01f;
	}

    RenderSystem *m_rndsys;
    ResourceCache *m_cache;

    render::Shader *m_shader;

	//video::SMaterial m_material;
    std::unique_ptr<MeshBuffer> m_mesh;
	// Value of m_origin at the time the mesh was last updated
	v2f m_mesh_origin;
	// Value of center_of_drawing_in_noise_i at the time the mesh was last updated
	v2s16 m_last_noise_center;
	// Was the mesh ever generated?
	bool m_mesh_valid = false;

	aabbf m_box{{0.0f, 0.0f, 0.0f}};
	v2f m_origin;
	u16 m_cloud_radius_i;
	u32 m_seed;
	v3f m_camera_pos;

    bool m_visible = true;
	bool m_camera_inside_cloud = false;

	bool m_enable_3d;
	img::color8 m_color = img::white;
	CloudParams m_params;
};
