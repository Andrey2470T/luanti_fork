// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2023 Vitaliy Lobachevskiy

#pragma once
#include <ostream>
#include "BasicIncludes.h"

namespace utils {

	template <class T>
    std::ostream &operator<< (std::ostream &os, Vector2D<T> vec)
	{
		return os << "(" << vec.X << "," << vec.Y << ")";
	}

	template <class T>
    std::ostream &operator<< (std::ostream &os, Vector3D<T> vec)
	{
		return os << "(" << vec.X << "," << vec.Y << "," << vec.Z << ")";
	}

}
