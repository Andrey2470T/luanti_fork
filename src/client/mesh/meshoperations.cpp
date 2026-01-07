// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "meshoperations.h"
#include "defaultVertexTypes.h"
#include <cmath>
#include "client/render/batcher3d.h"
#include <Utils/Plane3D.h>
#include <set>

inline static void applyShadeFactor(img::color8& color, float factor)
{
	color.R(static_cast<u8>(round32(color.R()*factor)));
	color.G(static_cast<u8>(round32(color.G()*factor)));
	color.B(static_cast<u8>(round32(color.B()*factor)));
}

void MeshOperations::applyFacesShading(img::color8 &color, const v3f normal)
{
	/*
		Some drawtypes have normals set to (0, 0, 0), this must result in
		maximum brightness: shade factor 1.0.
		Shade factors for aligned cube faces are:
		+Y 1.000000 sqrt(1.0)
		-Y 0.447213 sqrt(0.2)
		+-X 0.670820 sqrt(0.45)
		+-Z 0.836660 sqrt(0.7)
	*/
	float x2 = normal.X * normal.X;
	float y2 = normal.Y * normal.Y;
	float z2 = normal.Z * normal.Z;
	if (normal.Y < 0)
		applyShadeFactor(color, 0.670820f * x2 + 0.447213f * y2 + 0.836660f * z2);
	else if ((x2 > 1e-3) || (z2 > 1e-3))
		applyShadeFactor(color, 0.670820f * x2 + 1.000000f * y2 + 0.836660f * z2);
}

void MeshOperations::scaleMesh(MeshBuffer *mesh, v3f scale)
{
	if (!mesh)
		return;

	for (u32 i = 0; i < mesh->getVertexCount(); i++) {
		svtSetPos(mesh, svtGetPos(mesh, i) * scale, i);
	}

	mesh->recalculateBoundingBox();
}

void MeshOperations::translateMesh(MeshBuffer *mesh, v3f vec)
{
	if (!mesh)
		return;

	for (u32 i = 0; i < mesh->getVertexCount(); i++) {
		svtSetPos(mesh, svtGetPos(mesh, i) + vec, i);
	}

	mesh->recalculateBoundingBox();
}

void MeshOperations::colorizeMesh(MeshBuffer *mesh, const img::color8 &color,
    bool apply_face_shading)
{
	if (!mesh)
		return;

	for (u32 i = 0; i < mesh->getVertexCount(); i++) {
        img::color8 res_c = color;

		if (apply_face_shading)
			applyFacesShading(res_c, svtGetNormal(mesh, i));
		svtSetColor(mesh, res_c, i);
	}
}

template <typename F>
static void applyToMesh(MeshBuffer *mesh, const F &fn)
{
	if (!mesh)
		return;

	for (u32 i = 0; i < mesh->getVertexCount(); i++) {
		fn(i);
	}
}

void MeshOperations::setMeshColorByNormalXYZ(MeshBuffer *mesh,
		const img::color8 &colorX,
		const img::color8 &colorY,
		const img::color8 &colorZ)
{
	if (!mesh)
		return;
    auto colorizator = [=] (u32 i) {
		v3f normal = svtGetNormal(mesh, i);
        normal.X = fabs(normal.X);
        normal.Y = fabs(normal.Y);
        normal.Z = fabs(normal.Z);

		if (normal.X >= normal.Y && normal.X >= normal.Z)
            svtSetColor(mesh, colorX, i);
        else if (normal.Y >= normal.Z)
            svtSetColor(mesh, colorY, i);
		else
            svtSetColor(mesh, colorZ, i);

		svtSetNormal(mesh, normal, i);
	};
	applyToMesh(mesh, colorizator);
}

void MeshOperations::setMeshColorByNormal(MeshBuffer *mesh, const v3f &normal,
        const img::color8 &color)
{
	if (!mesh)
		return;
    auto colorizator = [normal, color, mesh] (u32 i) {
		v3f n = svtGetNormal(mesh, i);
		if (n == normal)
            svtSetColor(mesh, color, i);
	};
	applyToMesh(mesh, colorizator);
}

template <float v3f::*U, float v3f::*V>
static void rotateMesh(MeshBuffer *mesh, float degrees)
{
	degrees *= M_PI / 180.0f;
	float c = std::cos(degrees);
	float s = std::sin(degrees);
    auto rotator = [c, s, mesh] (u32 i) {
        auto rotate_vec = [c, s] (v3f vec) {
			float u = vec.*U;
			float v = vec.*V;
			vec.*U = c * u - s * v;
			vec.*V = s * u + c * v;
		};
		rotate_vec(svtGetPos(mesh, i));
		rotate_vec(svtGetNormal(mesh, i));
	};
	applyToMesh(mesh, rotator);
}

void MeshOperations::rotateMeshXYby(MeshBuffer *mesh, f64 degrees)
{
	rotateMesh<&v3f::X, &v3f::Y>(mesh, degrees);
}

void MeshOperations::rotateMeshXZby(MeshBuffer *mesh, f64 degrees)
{
	rotateMesh<&v3f::X, &v3f::Z>(mesh, degrees);
}

void MeshOperations::rotateMeshYZby(MeshBuffer *mesh, f64 degrees)
{
	rotateMesh<&v3f::Y, &v3f::Z>(mesh, degrees);
}

void MeshOperations::rotateMeshBy6dFacedir(MeshBuffer *mesh, u8 facedir)
{
	u8 axisdir = facedir >> 2;
	facedir &= 0x03;
	switch (facedir) {
		case 1: rotateMeshXZby(mesh, -90); break;
		case 2: rotateMeshXZby(mesh, 180); break;
		case 3: rotateMeshXZby(mesh, 90); break;
	}
	switch (axisdir) {
		case 1: rotateMeshYZby(mesh, 90); break; // z+
		case 2: rotateMeshYZby(mesh, -90); break; // z-
		case 3: rotateMeshXYby(mesh, -90); break; // x+
		case 4: rotateMeshXYby(mesh, 90); break; // x-
		case 5: rotateMeshXYby(mesh, -180); break;
	}
}

bool MeshOperations::checkMeshNormals(MeshBuffer *mesh)
{
	if (!mesh)
		return false;
	if (!mesh->getVertexCount())
		return false;

	// Here we intentionally check only first normal, assuming that if buffer
	// has it valid, then most likely all other ones are fine too. We can
	// check all of the normals to have length, but it seems like an overkill
	// hurting the performance and covering only really weird broken models.
	f32 length = svtGetNormal(mesh, 0).getLength();

	if (!std::isfinite(length) || length < 1e-10f)
		return false;

	return true;
}

MeshBuffer *MeshOperations::convertNodeboxesToMesh(const std::vector<aabbf> &boxes,
    const std::array<rectf, 6> *uv_coords, float expand)
{
    MeshBuffer *mesh = new MeshBuffer(6 * 4 * boxes.size(), 6 * 6 * boxes.size());

	img::color8 c(img::PF_RGBA8, 255,255,255,255);
    std::array<img::color8, 24> colors;
    colors.fill(c);

	for (aabbf box : boxes) {
		box.repair();

		box.MinEdge.X -= expand;
		box.MinEdge.Y -= expand;
		box.MinEdge.Z -= expand;
		box.MaxEdge.X += expand;
		box.MaxEdge.Y += expand;
		box.MaxEdge.Z += expand;

        Batcher3D::box(mesh, box, colors, uv_coords);
    }

    return mesh;
}

static inline v3f getAngleWeight(const v3f &v1, const v3f &v2, const v3f &v3)
{
    // Calculate this triangle's weight for each of its three vertices
    // start by calculating the lengths of its sides
    const f32 a = v2.getDistanceFromSQ(v3);
    const f32 asqrt = sqrtf(a);
    const f32 b = v1.getDistanceFromSQ(v3);
    const f32 bsqrt = sqrtf(b);
    const f32 c = v1.getDistanceFromSQ(v2);
    const f32 csqrt = sqrtf(c);

    // use them to find the angle at each vertex
    return v3f(
        acosf((b + c - a) / (2.f * bsqrt * csqrt)),
        acosf((-b + c + a) / (2.f * asqrt * csqrt)),
        acosf((b - c + a) / (2.f * bsqrt * asqrt)));
}

void MeshOperations::recalculateNormals(MeshBuffer *mesh, bool smooth, bool angleWeighted)
{
    const u32 vcount = mesh->getVertexCount();
    const u32 icount = mesh->getIndexCount();

    if (!smooth) {
        for (u32 i = 0; i < icount; i += 3) {
            u32 i0 = mesh->getIndexAt(i + 0);
            u32 i1 = mesh->getIndexAt(i + 1);
            u32 i2 = mesh->getIndexAt(i + 2);
            const v3f &v1 = svtGetPos(mesh, i0);
            const v3f &v2 = svtGetPos(mesh, i1);
            const v3f &v3 = svtGetPos(mesh, i2);
            const v3f normal = plane3f(v1, v2, v3).Normal;
            svtSetNormal(mesh, normal, i0);
            svtSetNormal(mesh, normal, i1);
            svtSetNormal(mesh, normal, i2);
        }
    } else {
        u32 i;

        for (i = 0; i != vcount; ++i)
            svtSetNormal(mesh, v3f(), i);

        for (i = 0; i < icount; i += 3) {
            u32 i0 = mesh->getIndexAt(i + 0);
            u32 i1 = mesh->getIndexAt(i + 1);
            u32 i2 = mesh->getIndexAt(i + 2);
            const v3f &v1 = svtGetPos(mesh, i0);
            const v3f &v2 = svtGetPos(mesh, i1);
            const v3f &v3 = svtGetPos(mesh, i2);
            const v3f normal = plane3f(v1, v2, v3).Normal;
            v3f weight(1.f, 1.f, 1.f);
            if (angleWeighted)
                weight = getAngleWeight(v1, v2, v3);

            svtSetNormal(mesh, normal * weight.X, i0);
            svtSetNormal(mesh, normal * weight.Y, i1);
            svtSetNormal(mesh, normal * weight.Z, i2);
        }

        for (i = 0; i != vcount; ++i) {
            v3f n = svtGetNormal(mesh, i);
            n.normalize();
            svtSetNormal(mesh, n, i);
        }
    }
}

void MeshOperations::recalculateMeshAtlasUVs(MeshBuffer *mesh, u32 start_index, u32 index_count, u32 newAtlasSize,
    const rectf &newImgRect, std::optional<u32> oldAtlasSize, std::optional<rectf> oldImgRect)
{
    std::map<u32, v2f> passedUVs;

    for (u32 i = start_index; i < start_index + index_count; i++) {
        u32 index = mesh->getIndexAt(i);
        v2f cur_uv = svtGetUV(mesh, index);

        if (oldAtlasSize.has_value() && oldImgRect.has_value()) {
            v2u pixel_coords;
            pixel_coords.X = round32(cur_uv.X * oldAtlasSize.value() - oldImgRect.value().ULC.X);
            pixel_coords.Y = round32(cur_uv.Y * oldAtlasSize.value() - oldImgRect.value().ULC.Y);

            cur_uv.X = (f32)pixel_coords.X / oldImgRect->getWidth();
            cur_uv.Y = (f32)pixel_coords.Y / oldImgRect->getHeight();
        }

        u32 rel_x = round32(cur_uv.X * newImgRect.getWidth());
        u32 rel_y = round32(cur_uv.Y * newImgRect.getHeight());

        v2f new_uv;
        new_uv.X = (f32)(rel_x + newImgRect.ULC.X) / newAtlasSize;
        new_uv.Y = (f32)(rel_y + newImgRect.ULC.Y) / newAtlasSize;

        passedUVs[index] = new_uv;
    }

    for (auto &uv : passedUVs)
        svtSetUV(mesh, uv.second, uv.first);
}

/*void setMaterialFilters(video::SMaterialLayer &tex, bool bilinear, bool trilinear, bool anisotropic)
{
	if (trilinear)
		tex.MinFilter = video::ETMINF_LINEAR_MIPMAP_LINEAR;
	else if (bilinear)
		tex.MinFilter = video::ETMINF_LINEAR_MIPMAP_NEAREST;
	else
		tex.MinFilter = video::ETMINF_NEAREST_MIPMAP_NEAREST;

	tex.MagFilter = (trilinear || bilinear) ? video::ETMAGF_LINEAR : video::ETMAGF_NEAREST;

	tex.AnisotropicFilter = anisotropic ? 0xFF : 0;
}*/
