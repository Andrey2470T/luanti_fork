#pragma once

#include "BasicIncludes.h"
#include "Utils/Plane3D.h"
#include "Utils/Matrix4.h"
#include <array>

enum FrustumPlanes
{
    FP_FAR = 0,
    FP_NEAR,
    FP_LEFT,
    FP_RIGHT,
    FP_BOTTOM,
    FP_TOP,
    FP_COUNT
};

struct Frustum
{
public:
    f32 Fovx = PI / 2.5f;       // Horizontal field of view, in radians.
    f32 Fovy = PI / 2.5;        // Vertical field of view, in radians.
    f32 Aspect = 4.0f / 3.0f;   // Aspect ratio.
    f32 ZNear = 1.0f;           // value of the near view-plane.
    f32 ZFar = 3000.0f;         // Z-value of the far view-plane.

    // Used for the orthogonal matrix
    f32 WidthOfViewVolume = 1.0f;
    f32 HeightOfViewVolume = 1.0f;

    bool IsOrthogonal = false;

	//! all planes enclosing the view frustum.
    std::array<plane3f, FP_COUNT> Planes;

	Frustum(const v2u &viewportSize=v2u(), bool IsOrtogonal=false);
    void recalculatePlanes(const matrix4 &mat);
};
