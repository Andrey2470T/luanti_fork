// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include "meshbuffer.h"
#include "Image/Color.h"

typedef std::array<MeshBuffer*, 6> MeshStorage;
class MeshOperations
{

	/*!
	* Applies shading to a color based on the surface's
	* normal vector.
	*/
	static void applyFacesShading(img::color8 &color, const v3f normal);

	/*
		Create a new cube mesh.
		Vertices are at (+-scale.X/2, +-scale.Y/2, +-scale.Z/2).

		The resulting mesh has 6 materials (up, down, right, left, back, front)
		which must be defined by the caller.
	*/
	static MeshStorage createCubeMesh(v3f scale);

	/*
		Multiplies each vertex coordinate by the specified scaling factors
		(componentwise vector multiplication).
	*/
	static void scaleMesh(MeshBuffer *mesh, v3f scale);

	/*
		Translate each vertex coordinate by the specified vector.
	*/
	static void translateMesh(MeshBuffer *mesh, v3f vec);

	/*!
	 * Overwrites the color of a mesh buffer.
	 * The color is darkened based on the normal vector of the vertices.
	*/
	static void colorizeMesh(MeshBuffer *mesh, const img::color8 &color,
		bool apply_face_shading=false);

	/*
		Set the color of all vertices in the mesh.
		For each vertex, determine the largest absolute entry in
		the normal vector, and choose one of colorX, colorY or
		colorZ accordingly.
	*/
	static void setMeshColorByNormalXYZ(MeshBuffer *mesh,
		const img::color8 &colorX,
		const img::color8 &colorY,
		const img::color8 &colorZ);

	static void setMeshColorByNormal(MeshBuffer *mesh, const v3f &normal,
		const img::color8 &color);

	/*
		Rotate the mesh by 6d facedir value.
		Method only for meshnodes, not suitable for entities.
	*/
	static void rotateMeshBy6dFacedir(MeshBuffer *mesh, u8 facedir);

	/*
		Rotate the mesh around the axis and given angle in degrees.
	*/
	static void rotateMeshXYby (MeshBuffer *mesh, f64 degrees);
	static void rotateMeshXZby (MeshBuffer *mesh, f64 degrees);
	static void rotateMeshYZby (MeshBuffer *mesh, f64 degrees);

	/*
		Convert nodeboxes to mesh. Each tile goes into a different buffer.
		boxes - set of nodeboxes to be converted into cuboids
		uv_coords[24] - table of texture uv coords for each cuboid face
		expand - factor by which cuboids will be resized
	*/
	static MeshStorage convertNodeboxesToMesh(const std::vector<aabbf> &boxes,
		const f32 *uv_coords = NULL, float expand = 0);


	//! Recalculates all normals of the mesh.
	/** \param mesh: Mesh on which the operation is performed.
	\param smooth: Whether to use smoothed normals. */
	static void recalculateNormals(MeshBuffer *mesh, bool smooth = false, bool angleWeighted = false) const;

	/*
		Check if mesh has valid normals and return true if it does.
		We assume normal to be valid when it's 0 < length < Inf. and not NaN
	*/
	static bool checkMeshNormals(MeshBuffer *mesh);

	/*
		Set the MinFilter, MagFilter and AnisotropicFilter properties of a texture
		layer according to the three relevant boolean values found in the Minetest
		settings.
	*/
	//static void setMaterialFilters(video::SMaterialLayer &tex, bool bilinear, bool trilinear, bool anisotropic);

};
