// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2023 Vitaliy Lobachevskiy

#pragma once
#include <array>
#include <vector>
#include <irrlichttypes.h>
#include <Mesh/VertexTypes.h>

<<<<<<< HEAD
#define HW {-NAN, 0, 0}
=======
#define HW {0xffffff00, 0, 0}
>>>>>>> 6807e913e (Fixed TestMapblockMeshGenerator)

/// Represents a triangle as three vertices.
/// “Smallest” (according to <) vertex is expected to be first, others should follow in the counter-clockwise order.
using Triangle = std::array<scene::Vertex3DExt, 3>;

/// Represents a quad as four vertices.
/// Vertices should be in the counter-clockwise order.
using Quad = std::array<scene::Vertex3DExt, 4>;

bool operator==(const scene::Vertex3DExt &v1, const scene::Vertex3DExt &v2);

/// Compare two meshes for equality.
/// @param vertices Vertices of the first mesh. Order doesn’t matter.
/// @param indices Indices of the first mesh. Triangle order doesn’t matter. Vertex order in a triangle only matters for winding.
/// @param expected The second mesh, in an expanded form. Must be sorted.
/// @returns Whether the two meshes are equal.
[[nodiscard]] bool checkMeshEqual(const std::vector<scene::Vertex3DExt> &vertices, const std::vector<u16> &indices, const std::vector<Triangle> &expected);

/// Compare two meshes for equality.
/// @param vertices Vertices of the first mesh. Order doesn’t matter.
/// @param indices Indices of the first mesh. Triangle order doesn’t matter. Vertex order in a triangle only matters for winding.
/// @param expected The second mesh, in a quad form.
/// @returns Whether the two meshes are equal.
/// @note There are two ways to split a quad into 2 triangles; either is allowed.
[[nodiscard]] bool checkMeshEqual(const std::vector<scene::Vertex3DExt> &vertices, const std::vector<u16> &indices, const std::vector<Quad> &expected);
