#pragma once

#include "frustum.h"

// General camera class used for the player, wieldmesh and guiscene.
// Supports the perspective and orthogonal projection.

class Camera
{
protected:
	// Absolute camera position
	v3f m_position;
	// Absolute camera direction
	v3f m_direction;
    // Up vector
    v3f m_up_vector = v3f(0,1,0);
	// Camera offset
	v3s16 m_offset;

    v3f m_last_position;
    v3f m_last_direction;
    v3s16 m_last_offset;

    Frustum m_frustum;
    matrix4 m_projection_matrix;
    matrix4 m_view_matrix;
public:
    Camera(const v2u &viewportSize=v2u(),
            const v3f &position=v3f(), const v3f &direction=v3f(0,0,1),
            bool isOrthogonal=false);

    virtual ~Camera() = default;

	// Getters

    inline matrix4 getProjectionMatrix() const
    {
        return m_projection_matrix;
    }

    inline matrix4 getViewMatrix() const
    {
        return m_view_matrix;
    }

	inline v3f getPosition() const
	{
		return m_position;
	}

	inline v3f getDirection() const
	{
		return m_direction;
	}

    inline v3f getUpVector() const
    {
        return m_up_vector;
    }

	inline v3s16 getOffset() const
	{
		return m_offset;
	}

	inline f32 getFovX() const
	{
        return m_frustum.Fovx;
	}

	inline f32 getFovY() const
	{
        return m_frustum.Fovy;
	}

	// Get maximum of getFovX() and getFovY()
	inline f32 getFovMax() const
	{
        return std::max(m_frustum.Fovx, m_frustum.Fovy);
	}

    inline f32 getAspectRatio() const
    {
        return m_frustum.Aspect;
    }

    inline f32 getNearValue() const
    {
        return m_frustum.ZNear;
    }

    inline f32 getFarValue() const
    {
        return m_frustum.ZFar;
    }

    inline bool isOrthogonal() const
    {
        return m_frustum.IsOrthogonal;
    }

    void setViewMatrix(const matrix4 &view)
    {
        m_view_matrix = view;
    }

    void setPosition(const v3f &pos)
    {
        m_last_position = m_position;
        m_position = pos;
    }

    void setDirection(const v3f &dir)
    {
        m_last_direction = m_direction;
        m_direction = dir;
        m_direction.normalize();
    }

    void setUpVector(const v3f &up)
    {
        m_up_vector = up;
		m_up_vector.normalize();
    }

    void setOffset(const v3s16 &offset)
	{
        m_last_offset = m_offset;
		m_offset = offset;
	}

	void setFovX(f32 fovx)
	{
        m_frustum.Fovx = fovx;
	}

	void setFovY(f32 fovy)
	{
        m_frustum.Fovy = fovy;
	}

	void setAspectRatio(f32 aspect)
    {
        m_frustum.Aspect = aspect;
    }

    void setNearValue(f32 znear)
    {
        m_frustum.ZNear = znear;
    }

    void setFarValue(f32 zfar)
    {
		m_frustum.ZFar = zfar;
    }

    void recalculateProjectionMatrix();
	void recalculateViewArea();
	void updatePlanes();

    bool isNecessaryUpdateDrawList();

    void updateMatrices();

    bool frustumCull(const v3f &position, f32 radiusSq) const;
};
