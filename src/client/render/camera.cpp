#include "camera.h"
#include "client.h"

Camera::Camera(Client *client, const v2u &viewportSize,
		   const v3f &position, const v3f &direction,
           bool isOrthogonal)
	: m_client(client), m_position(position), m_direction(direction),
	  m_frustum(viewportSize, isOrthogonal)
{}

void Camera::recalculateProjectionMatrix()
{
	if (m_frustum.IsOrthogonal)
        m_projection_matrix.buildProjectionMatrixOrthoLH(
			m_frustum.WidthOfViewVolume, m_frustum.HeightOfViewVolume,
			m_frustum.ZNear, m_frustum.ZFar, false);
	else
		m_projection_matrix.buildProjectionMatrixPerspectiveFovLH(
			m_frustum.Fovy, m_frustum.Aspect, m_frustum.ZNear, m_frustum.ZFar, false);
}

void Camera::recalculateViewArea()
{
	// if upvector and vector to the target are the same, we have a
	// problem. so solve this problem:

	v3f up = m_up_vector;
	f32 right = m_direction.dotProduct(up);

	if (equals(std::fabs(right), 1.0f)) {
		up.X += 0.5f;
	}

	m_view_matrix.buildCameraLookAtMatrixLH(m_position, m_position+m_direction, up);
}

void Camera::updatePlanes()
{
	matrix4 m(matrix4::EM4CONST_NOTHING);
	m.setbyproduct_nocheck(m_projection_matrix, m_view_matrix);

	m_frustum.recalculatePlanes(m);
}

void Camera::updateMatrices()
{
    recalculateProjectionMatrix();
    recalculateViewArea();
    updatePlanes();
}
