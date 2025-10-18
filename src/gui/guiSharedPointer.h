/*
Minetest
Copyright (C) 2018 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

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
#include <memory>
#include <type_traits>
#include <utility>

class IReferenceCounted;

namespace gui
{
	class IGUIElement;

/** Shared pointer for IrrLicht objects.
 *
 * It should only be used for user-managed objects, i.e. those created with
 * the @c new operator or @c create* functions, like:
 * `GUISharedPointer<scene::IMeshBuffer> buf{new scene::SMeshBuffer()};`
 * The reference counting is *not* balanced as new objects have reference
 * count set to one, and the @c GUISharedPointer constructor (and @c reset) assumes
 * ownership of that reference.
 *
 * It shouldn’t be used for engine-managed objects, including those created
 * with @c addTexture and similar methods. Constructing @c GUISharedPointer directly
 * from such object is a bug and may lead to a crash. Indirect construction
 * is possible though; see the @c grab free function for details and use cases.
 */
template <class ReferenceCounted>
class GUISharedPointer
{
	ReferenceCounted *value = nullptr;

public:
    GUISharedPointer() noexcept = default;

    GUISharedPointer(std::nullptr_t) noexcept {}

    GUISharedPointer(const GUISharedPointer &b) noexcept { grab(b.get()); }

    GUISharedPointer(GUISharedPointer &&b) noexcept { reset(b.release()); }

	template <typename B,
			std::enable_if_t<std::is_convertible_v<B *, ReferenceCounted *>, bool> = true>
    GUISharedPointer(const GUISharedPointer<B> &b) noexcept
	{
		grab(b.get());
	}

	template <typename B,
			std::enable_if_t<std::is_convertible_v<B *, ReferenceCounted *>, bool> = true>
    GUISharedPointer(GUISharedPointer<B> &&b) noexcept
	{
		reset(b.release());
	}

	/** Constructs a shared pointer out of a plain one to control object lifetime.
	 * @param object The object, usually returned by some @c create* function.
	 * @note Move semantics: reference counter is *not* increased.
	 * @warning Never wrap any @c add* function with this!
	 */
    explicit GUISharedPointer(ReferenceCounted *object) noexcept { reset(object); }

    ~GUISharedPointer() { reset(); }

    GUISharedPointer &operator=(const GUISharedPointer &b) noexcept
	{
		grab(b.get());
		return *this;
	}

    GUISharedPointer &operator=(GUISharedPointer &&b) noexcept
	{
		reset(b.release());
		return *this;
	}

	template <typename B,
			std::enable_if_t<std::is_convertible_v<B *, ReferenceCounted *>, bool> = true>
    GUISharedPointer &operator=(const GUISharedPointer<B> &b) noexcept
	{
		grab(b.get());
		return *this;
	}

	template <typename B,
			std::enable_if_t<std::is_convertible_v<B *, ReferenceCounted *>, bool> = true>
    GUISharedPointer &operator=(GUISharedPointer<B> &&b) noexcept
	{
		reset(b.release());
		return *this;
	}

	ReferenceCounted &operator*() const noexcept { return *value; }
	ReferenceCounted *operator->() const noexcept { return value; }
	explicit operator ReferenceCounted *() const noexcept { return value; }
	explicit operator bool() const noexcept { return !!value; }

	/** Returns the stored pointer.
	 */
	ReferenceCounted *get() const noexcept { return value; }

	/** Returns the stored pointer, erasing it from this class.
	 * @note Move semantics: reference counter is not changed.
	 */
	ReferenceCounted *release() noexcept
	{
		ReferenceCounted *object = value;
		value = nullptr;
		return object;
	}

	/** Drops stored pointer replacing it with the given one.
	 * @note Move semantics: reference counter is *not* increased.
	 */
	void reset(ReferenceCounted *object = nullptr) noexcept
	{
		static_assert(std::is_base_of_v<IReferenceCounted, ReferenceCounted>,
				"Class is not an IReferenceCounted");
		if (value)
			value->drop();
		value = object;
	}

	/** Drops stored pointer replacing it with the given one.
	 * @note Copy semantics: reference counter *is* increased.
	 */
	void grab(ReferenceCounted *object) noexcept
	{
		static_assert(std::is_base_of_v<IReferenceCounted, ReferenceCounted>,
				"Class is not an IReferenceCounted");
		if (object)
			object->grab();
		reset(object);
	}
};

/** Constructs a shared pointer as a *secondary* reference to an object
 *
 * This function is intended to make a temporary reference to an object which
 * is owned elsewhere so that it is not destroyed too early. To achieve that
 * it does balanced reference counting, i.e. reference count is increased
 * in this function and decreased when the returned pointer is destroyed.
 */
template <class ReferenceCounted>
[[nodiscard]]
GUISharedPointer<ReferenceCounted> grab(ReferenceCounted *object) noexcept
{
    GUISharedPointer<ReferenceCounted> ptr;
	ptr.grab(object);
	return ptr;
}

template <typename ReferenceCounted>
bool operator==(const GUISharedPointer<ReferenceCounted> &a, const GUISharedPointer<ReferenceCounted> &b) noexcept
{
	return a.get() == b.get();
}

template <typename ReferenceCounted>
bool operator==(const GUISharedPointer<ReferenceCounted> &a, const ReferenceCounted *b) noexcept
{
	return a.get() == b;
}

template <typename ReferenceCounted>
bool operator==(const ReferenceCounted *a, const GUISharedPointer<ReferenceCounted> &b) noexcept
{
	return a == b.get();
}

template <typename ReferenceCounted>
bool operator!=(const GUISharedPointer<ReferenceCounted> &a, const GUISharedPointer<ReferenceCounted> &b) noexcept
{
	return a.get() != b.get();
}

template <typename ReferenceCounted>
bool operator!=(const GUISharedPointer<ReferenceCounted> &a, const ReferenceCounted *b) noexcept
{
	return a.get() != b;
}

template <typename ReferenceCounted>
bool operator!=(const ReferenceCounted *a, const GUISharedPointer<ReferenceCounted> &b) noexcept
{
	return a != b.get();
}

/** Same as std::make_unique<T>, but for GUISharedPointer.
 */
template <class T, class... Args>
GUISharedPointer<T> make_gui_shared(Args&&... args)
{
    return GUISharedPointer<T>(new T(std::forward<Args>(args)...));
}

// We cannot use irr_ptr for Irrlicht GUI elements we own.
// Option 1: Pass IGUIElement* returned by IGUIEnvironment::add* into irr_ptr
//           constructor.
//           -> We steal the reference owned by IGUIEnvironment and drop it later,
//           causing the IGUIElement to be deleted while IGUIEnvironment still
//           references it.
// Option 2: Pass IGUIElement* returned by IGUIEnvironment::add* into irr_ptr::grab.
//           -> We add another reference and drop it later, but since IGUIEnvironment
//           still references the IGUIElement, it is never deleted.
// To make IGUIEnvironment drop its reference to the IGUIElement, we have to call
// IGUIElement::remove, so that's what we'll do.
/*template <typename T>
std::shared_ptr<T> grab_gui_element(T *element)
{
	static_assert(std::is_base_of_v<gui::IGUIElement, T>,
			"grab_gui_element only works for IGUIElement");
	return std::shared_ptr<T>(element, [](T *e) {
		e->remove();
	});
}*/

}
