// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2021 Liso <anlismon@gmail.com>

#pragma once

#include <BasicIncludes.h>
#include <Utils/Matrix4.h>
#include <Image/Color.h>
#include <Image/Converting.h>
#include "util/basic_macros.h"
#include "constants.h"

class Camera;
class Client;
class DistanceSortedDrawList;

struct shadowFrustum
{
	f32 zNear{0.0f};
	f32 zFar{0.0f};
	f32 length{0.0f};
	f32 radius{0.0f};
    matrix4 ProjOrthMat;
    matrix4 ViewMat;
	v3f position;
	v3f player;
	v3s16 camera_offset;
};

class DirectionalLight
{
public:
	DirectionalLight(const u32 shadowMapResolution,
			const v3f &position,
            img::colorf lightColor = img::color8ToColorf(img::white),
			f32 farValue = 100.0f);
	~DirectionalLight() = default;

	//DISABLE_CLASS_COPY(DirectionalLight)

    void update_frustum(const Camera *cam, Client *client,
        DistanceSortedDrawList *drawlist, bool force = false);

	// when set direction is updated to negative normalized(direction)
	void setDirection(v3f dir);
	v3f getDirection() const{
		return direction;
	};
	v3f getPosition() const;
	v3f getPlayerPos() const;
	v3f getFuturePlayerPos() const;

	/// Gets the light's matrices.
    const matrix4 &getViewMatrix() const;
    const matrix4 &getProjectionMatrix() const;
    const matrix4 &getFutureViewMatrix() const;
    const matrix4 &getFutureProjectionMatrix() const;
    matrix4 getViewProjMatrix();

	/// Gets the light's maximum far value, i.e. the shadow boundary
	f32 getMaxFarValue() const
	{
		return farPlane * BS;
	}

	/// Gets the current far value of the light
	f32 getFarValue() const
	{
		return shadow_frustum.zFar;
	}


	/// Gets the light's color.
    const img::colorf &getLightColor() const
	{
		return diffuseColor;
	}

	/// Sets the light's color.
    void setLightColor(const img::colorf &lightColor)
	{
		diffuseColor = lightColor;
	}

	/// Gets the shadow map resolution for this light.
	u32 getMapResolution() const
	{
		return mapRes;
	}

	bool should_update_map_shadow{true};

	void commitFrustum();

private:
	void createSplitMatrices(const Camera *cam);

    img::colorf diffuseColor;

	f32 farPlane;
	u32 mapRes;

	v3f pos;
	v3f direction{0};

	v3f last_cam_pos_world{0,0,0};
	v3f last_look{0,1,0};

	shadowFrustum shadow_frustum;
	shadowFrustum future_frustum;
	bool dirty{false};
};
