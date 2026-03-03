/*
Minetest
Copyright (C) 2013 Jonathan Neuschäfer <j.neuschaefer@gmx.net>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#pragma once

#include <string>

static const char hex_chars[] = "0123456789abcdef";

[[nodiscard]]
static inline std::string hex_encode(std::string_view data)
{
    std::string ret;
    ret.reserve(data.size() * 2);
    for (unsigned char c : data) {
        ret.push_back(hex_chars[(c & 0xf0) >> 4]);
        ret.push_back(hex_chars[c & 0x0f]);
    }
    return ret;
}

[[nodiscard]]
static inline std::string hex_encode(const char *data, size_t data_size)
{
    if (!data_size)
            return "";
        return hex_encode(std::string_view(data, data_size));
}

static inline bool hex_digit_decode(char hexdigit, unsigned char &value)
{
	if (hexdigit >= '0' && hexdigit <= '9')
		value = hexdigit - '0';
	else if (hexdigit >= 'A' && hexdigit <= 'F')
		value = hexdigit - 'A' + 10;
	else if (hexdigit >= 'a' && hexdigit <= 'f')
		value = hexdigit - 'a' + 10;
	else
		return false;
	return true;
}
