#include "frustum.h"

Frustum::Frustum(const v2u &viewportSize, bool isOrtogonal)
{
    IsOrthogonal = isOrtogonal;

	if (viewportSize.X != 0 && viewportSize.Y != 0) {
        if (!IsOrthogonal)
			Aspect = viewportSize.X / (f32)viewportSize.Y;
		else {
			WidthOfViewVolume = (f32)viewportSize.X;
			HeightOfViewVolume = (f32)viewportSize.Y;
		}
	}
}

void Frustum::recalculatePlanes(const matrix4 &mat)
{
	// left clipping plane
    Planes[FP_LEFT].Normal.X = mat[3] + mat[0];
    Planes[FP_LEFT].Normal.Y = mat[7] + mat[4];
    Planes[FP_LEFT].Normal.Z = mat[11] + mat[8];
    Planes[FP_LEFT].D = mat[15] + mat[12];

	// right clipping plane
    Planes[FP_RIGHT].Normal.X = mat[3] - mat[0];
    Planes[FP_RIGHT].Normal.Y = mat[7] - mat[4];
    Planes[FP_RIGHT].Normal.Z = mat[11] - mat[8];
    Planes[FP_RIGHT].D = mat[15] - mat[12];

	// top clipping plane
    Planes[FP_TOP].Normal.X = mat[3] - mat[1];
    Planes[FP_TOP].Normal.Y = mat[7] - mat[5];
    Planes[FP_TOP].Normal.Z = mat[11] - mat[9];
    Planes[FP_TOP].D = mat[15] - mat[13];

	// bottom clipping plane
    Planes[FP_BOTTOM].Normal.X = mat[3] + mat[1];
    Planes[FP_BOTTOM].Normal.Y = mat[7] + mat[5];
    Planes[FP_BOTTOM].Normal.Z = mat[11] + mat[9];
    Planes[FP_BOTTOM].D = mat[15] + mat[13];

	// far clipping plane
    Planes[FP_FAR].Normal.X = mat[3] - mat[2];
    Planes[FP_FAR].Normal.Y = mat[7] - mat[6];
    Planes[FP_FAR].Normal.Z = mat[11] - mat[10];
    Planes[FP_FAR].D = mat[15] - mat[14];

	// near clipping plane
    Planes[FP_NEAR].Normal.X = mat[3] + mat[2];
    Planes[FP_NEAR].Normal.Y = mat[7] + mat[6];
    Planes[FP_NEAR].Normal.Z = mat[11] + mat[10];
    Planes[FP_NEAR].D = mat[15] + mat[14];

	// normalize normals
	for (auto &plane : Planes) {
		const f32 len = -1.0f / plane.Normal.getLength();
		plane.Normal *= len;
		plane.D *= len;
	}
}
