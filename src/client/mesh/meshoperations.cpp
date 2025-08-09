// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "meshoperations.h"
#include "defaultVertexTypes.h"
#include <cmath>
#include "client/render/batcher3d.h"

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
    std::array<img::color8, 8> colors = {c, c, c, c, c, c, c, c};

	for (aabbf box : boxes) {
		box.repair();

		box.MinEdge.X -= expand;
		box.MinEdge.Y -= expand;
		box.MinEdge.Z -= expand;
		box.MaxEdge.X += expand;
		box.MaxEdge.Y += expand;
		box.MaxEdge.Z += expand;

        Batcher3D::appendBox(mesh, box, colors, uv_coords);
    }

    return mesh;
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
