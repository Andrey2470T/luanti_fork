// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "meshoperations.h"
#include "defaultVertexTypes.h"
#include "constants.h"
#include <cmath>

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

MeshStorage MeshOperations::createCubeMesh(v3f scale)
{
	img::color8 c(img::PF_RGBA8, 255,255,255,255);

	std::array<v3f, 24> positions = {
		v3f(-0.5,+0.5,-0.5),  v3f(-0.5,+0.5,+0.5),  v3f(+0.5,+0.5,+0.5),  v3f(+0.5,+0.5,-0.5), // Up
		v3f(-0.5,-0.5,-0.5),  v3f(+0.5,-0.5,-0.5),  v3f(+0.5,-0.5,+0.5),  v3f(-0.5,-0.5,+0.5), // Down
		v3f(+0.5,-0.5,-0.5),  v3f(+0.5,+0.5,-0.5),  v3f(+0.5,+0.5,+0.5),  v3f(+0.5,-0.5,+0.5), // Right
		v3f(-0.5,-0.5,-0.5),  v3f(-0.5,-0.5,+0.5),  v3f(-0.5,+0.5,+0.5),  v3f(-0.5,+0.5,-0.5), // Left
		v3f(-0.5,-0.5,+0.5),  v3f(+0.5,-0.5,+0.5),  v3f(+0.5,+0.5,+0.5),  v3f(-0.5,+0.5,+0.5), // Back
		v3f(-0.5,-0.5,-0.5),  v3f(-0.5,+0.5,-0.5),  v3f(+0.5,+0.5,-0.5),  v3f(+0.5,-0.5,-0.5)  // Front
	};

	std::array<v3f, 6> normals = {
		// Up			Down			Right			Left			Back			Front
		v3f(0,1,0),		v3f(0,-1,0),	v3f(1,0,0),		v3f(-1,0,0),	v3f(0,0,1),		v3f(0,0,-1)
	};

	std::array<v2f, 24> uvs = {
		v2f(0,1),  v2f(0,0),  v2f(1,0),  v2f(1,1),  // Up
		v2f(0,0),  v2f(1,0),  v2f(1,1),  v2f(0,1),  // Down
		v2f(0,1),  v2f(0,0),  v2f(1,0),  v2f(1,1),  // Right
		v2f(1,1),  v2f(0,1),  v2f(0,0),  v2f(1,0),  // Left
		v2f(1,1),  v2f(0,1),  v2f(0,0),  v2f(1,0),  // Back
		v2f(0,1),  v2f(0,0),  v2f(1,0),  v2f(1,1)   // Front
	};

	u32 indices[6] = {0,1,2,2,3,0};

	MeshStorage mesh;
	for (u8 i=0; i<6; ++i)
	{
		MeshBuffer *buf = new MeshBuffer();

		for (u8 j = 0; j < 4; j++)
			appendSVT(buf, positions[4*i+j], c, normals[i], uvs[4*i+j]);

		for (u8 k = 0; k < 6; k++)
			buf->setIndexAt(indices[k]);

		scaleMesh(buf, scale);
		mesh[i] = buf;
	}

	return mesh;
}

void MeshOperations::scaleMesh(MeshBuffer *mesh, v3f scale)
{
	if (!mesh)
		return;

	for (u32 i = 0; i < mesh->getVertexCount(); i++) {
		svtSetPos(mesh, svtGetPos(mesh, i) * scale, i);
	}

	mesh->uploadData(MeshBufferType::VERTEX);
	mesh->recalculateBoundingBox();
}

void MeshOperations::translateMesh(MeshBuffer *mesh, v3f vec)
{
	if (!mesh)
		return;

	for (u32 i = 0; i < mesh->getVertexCount(); i++) {
		svtSetPos(mesh, svtGetPos(mesh, i) + vec, i);
	}

	mesh->uploadData(MeshBufferType::VERTEX);
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

	mesh->uploadData(MeshBufferType::VERTEX);
}

template <typename F>
static void applyToMesh(MeshBuffer *mesh, const F &fn)
{
	if (!mesh)
		return;

	for (u32 i = 0; i < mesh->getVertexCount(); i++) {
		fn(i);
	}

	mesh->uploadData(MeshBufferType::VERTEX);
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

MeshStorage MeshOperations::convertNodeboxesToMesh(const std::vector<aabbf> &boxes,
		const f32 *uv_coords, float expand)
{
	MeshStorage dst_mesh;

	for (u16 j = 0; j < 6; j++)
		dst_mesh[j] = new MeshBuffer();

	img::color8 c(img::PF_RGBA8, 255,255,255,255);

	for (aabbf box : boxes) {
		box.repair();

		box.MinEdge.X -= expand;
		box.MinEdge.Y -= expand;
		box.MinEdge.Z -= expand;
		box.MaxEdge.X += expand;
		box.MaxEdge.Y += expand;
		box.MaxEdge.Z += expand;

		// Compute texture UV coords
		f32 tx1 = (box.MinEdge.X / BS) + 0.5;
		f32 ty1 = (box.MinEdge.Y / BS) + 0.5;
		f32 tz1 = (box.MinEdge.Z / BS) + 0.5;
		f32 tx2 = (box.MaxEdge.X / BS) + 0.5;
		f32 ty2 = (box.MaxEdge.Y / BS) + 0.5;
		f32 tz2 = (box.MaxEdge.Z / BS) + 0.5;

		f32 txc_default[24] = {
			// up
			tx1, 1 - tz2, tx2, 1 - tz1,
			// down
			tx1, tz1, tx2, tz2,
			// right
			tz1, 1 - ty2, tz2, 1 - ty1,
			// left
			1 - tz2, 1 - ty2, 1 - tz1, 1 - ty1,
			// back
			1 - tx2, 1 - ty2, 1 - tx1, 1 - ty1,
			// front
			tx1, 1 - ty2, tx2, 1 - ty1,
		};

		// use default texture UV mapping if not provided
		const f32 *txc = uv_coords ? uv_coords : txc_default;

		v3f min = box.MinEdge;
		v3f max = box.MaxEdge;

		std::array<v3f, 24> positions = {
			v3f(min.X,max.Y,max.Z), v3f(max.X,max.Y,max.Z), v3f(max.X,max.Y,min.Z), v3f(min.X,max.Y,min.Z), // up
			v3f(min.X,min.Y,min.Z), v3f(max.X,min.Y,min.Z), v3f(max.X,min.Y,max.Z), v3f(min.X,min.Y,max.Z), // down
			v3f(max.X,max.Y,min.Z), v3f(max.X,max.Y,max.Z), v3f(max.X,min.Y,max.Z), v3f(max.X,min.Y,min.Z), // right
			v3f(min.X,max.Y,max.Z), v3f(min.X,max.Y,min.Z), v3f(min.X,min.Y,min.Z), v3f(min.X,min.Y,max.Z), // left
			v3f(max.X,max.Y,max.Z), v3f(min.X,max.Y,max.Z), v3f(min.X,min.Y,max.Z), v3f(max.X,min.Y,max.Z), // back
			v3f(min.X,max.Y,min.Z), v3f(max.X,max.Y,min.Z), v3f(max.X,min.Y,min.Z), v3f(min.X,min.Y,min.Z)  // front
		};

		std::array<v3f, 6> normals = {
			v3f(0,1,0), v3f(0,-1,0), v3f(1,0,0), v3f(-1,0,0), v3f(0,0,1), v3f(0,0,-1)
		};

		std::array<v2f, 24> uvs = {
			v2f(txc[0],txc[1]), v2f(txc[2],txc[1]), v2f(txc[2],txc[3]), v2f(txc[0],txc[3]),
			v2f(txc[4],txc[5]), v2f(txc[6],txc[5]), v2f(txc[6],txc[7]), v2f(txc[4],txc[7]),
			v2f(txc[ 8],txc[9]), v2f(txc[10],txc[9]), v2f(txc[10],txc[11]), v2f(txc[ 8],txc[11]),
			v2f(txc[12],txc[13]), v2f(txc[14],txc[13]), v2f(txc[14],txc[15]), v2f(txc[12],txc[15]),
			v2f(txc[16],txc[17]), v2f(txc[18],txc[17]), v2f(txc[18],txc[19]), v2f(txc[16],txc[19]),
			v2f(txc[20],txc[21]), v2f(txc[22],txc[21]), v2f(txc[22],txc[23]), v2f(txc[20],txc[23])
		};

		u16 indices[] = {0,1,2,2,3,0};

		for (u8 i=0; i<6; ++i)
		{
			MeshBuffer *buf = dst_mesh[i];

			for (u8 j = 0; j < 4; j++)
				appendSVT(buf, positions[4*i+j], c, normals[i], uvs[4*i+j]);

			for (u8 k = 0; k < 6; k++)
				buf->setIndexAt(indices[k]);
		}
	}
	return dst_mesh;
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
