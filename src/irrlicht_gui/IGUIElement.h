// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "Utils/Rect.h"
#include "GUIEnums.h"
#include "Main/IEventReceiver.h"
#include <cassert>
#include <list>
#include <memory>

class IGUIEnvironment;

//! Base class of all GUI elements.
class IGUIElement : virtual public main::IEventReceiver
{
public:
	//! Constructor
    IGUIElement(GUIElementType type, IGUIEnvironment *environment, std::shared_ptr<IGUIElement> parent,
            s32 id, const recti &rectangle) :
			Parent(0),
			RelativeRect(rectangle), AbsoluteRect(rectangle),
			AbsoluteClippingRect(rectangle), DesiredRect(rectangle),
			MaxSize(0, 0), MinSize(1, 1), IsVisible(true), IsEnabled(true),
			IsSubElement(false), NoClip(false), ID(id), IsTabStop(false), TabOrder(-1), IsTabGroup(false),
            AlignLeft(GUIAlignment::UpperLeft), AlignRight(GUIAlignment::UpperLeft),
            AlignTop(GUIAlignment::UpperLeft), AlignBottom(GUIAlignment::UpperLeft),
			Environment(environment), Type(type)
	{
		// if we were given a parent to attach to
		if (parent) {
            parent->addChildToEnd(std::make_shared<IGUIElement>(this));
			recalculateAbsolutePosition(true);
		}
	}

	//! Destructor
	virtual ~IGUIElement()
    {}

	//! Returns parent of this element.
    std::shared_ptr<IGUIElement> getParent() const
	{
		return Parent;
	}

	//! Returns the relative rectangle of this element.
    recti getRelativePosition() const
	{
		return RelativeRect;
	}

	//! Sets the relative rectangle of this element.
	/** \param r The absolute position to set */
    void setRelativePosition(const recti &r)
	{
		if (Parent) {
            const recti &r2 = Parent->getAbsolutePosition();

            v2f d((f32)(r2.getWidth()), (f32)(r2.getHeight()));

            if (AlignLeft == GUIAlignment::Scale)
                ScaleRect.ULC.X = (f32)r.ULC.X / d.X;
            if (AlignRight == GUIAlignment::Scale)
                ScaleRect.LRC.X = (f32)r.LRC.X / d.X;
            if (AlignTop == GUIAlignment::Scale)
                ScaleRect.ULC.Y = (f32)r.ULC.Y / d.Y;
            if (AlignBottom == GUIAlignment::Scale)
                ScaleRect.LRC.Y = (f32)r.LRC.Y / d.Y;
		}

		DesiredRect = r;
		updateAbsolutePosition();
	}

	//! Sets the relative rectangle of this element, maintaining its current width and height
	/** \param position The new relative position to set. Width and height will not be changed. */
    void setRelativePosition(const v2i &position)
	{
        const v2i mySize = RelativeRect.getSize();
        const recti rectangle(position.X, position.Y,
                position.X + mySize.X, position.Y + mySize.Y);
		setRelativePosition(rectangle);
	}

	//! Sets the relative rectangle of this element as a proportion of its parent's area.
    /** \note This method used to be 'void setRelativePosition(const rectf& r)'
	\param r  The rectangle to set, interpreted as a proportion of the parent's area.
	Meaningful values are in the range [0...1], unless you intend this element to spill
	outside its parent. */
    void setRelativePositionProportional(const rectf &r)
	{
		if (!Parent)
			return;

        const v2i &d = Parent->getAbsolutePosition().getSize();

        DesiredRect = recti(
                std::floor((f32)d.X * r.ULC.X),
                std::floor((f32)d.Y * r.ULC.Y),
                std::floor((f32)d.X * r.LRC.X),
                std::floor((f32)d.Y * r.LRC.Y));

		ScaleRect = r;

		updateAbsolutePosition();
	}

	//! Gets the absolute rectangle of this element
    recti getAbsolutePosition() const
	{
		return AbsoluteRect;
	}

	//! Returns the visible area of the element.
    recti getAbsoluteClippingRect() const
	{
		return AbsoluteClippingRect;
	}

	//! Sets whether the element will ignore its parent's clipping rectangle
	/** \param noClip If true, the element will not be clipped by its parent's clipping rectangle. */
	void setNotClipped(bool noClip)
	{
		NoClip = noClip;
		updateAbsolutePosition();
	}

	//! Gets whether the element will ignore its parent's clipping rectangle
	/** \return true if the element is not clipped by its parent's clipping rectangle. */
	bool isNotClipped() const
	{
		return NoClip;
	}

	//! Sets the maximum size allowed for this element
	/** If set to 0,0, there is no maximum size */
    void setMaxSize(v2u size)
	{
		MaxSize = size;
		updateAbsolutePosition();
	}

	//! Sets the minimum size allowed for this element
    void setMinSize(v2u size)
	{
		MinSize = size;
        if (MinSize.X < 1)
            MinSize.X = 1;
        if (MinSize.Y < 1)
            MinSize.Y = 1;
		updateAbsolutePosition();
	}

	//! The alignment defines how the borders of this element will be positioned when the parent element is resized.
    void setAlignment(GUIAlignment left, GUIAlignment right, GUIAlignment top, GUIAlignment bottom)
	{
		AlignLeft = left;
		AlignRight = right;
		AlignTop = top;
		AlignBottom = bottom;

		if (Parent) {
            recti r(Parent->getAbsolutePosition());

            v2f d((f32)r.getWidth(), (f32)r.getHeight());

            if (AlignLeft == GUIAlignment::Scale)
                ScaleRect.ULC.X = (f32)DesiredRect.ULC.X / d.X;
            if (AlignRight == GUIAlignment::Scale)
                ScaleRect.LRC.X = (f32)DesiredRect.LRC.X / d.X;
            if (AlignTop == GUIAlignment::Scale)
                ScaleRect.ULC.Y = (f32)DesiredRect.ULC.Y / d.Y;
            if (AlignBottom == GUIAlignment::Scale)
                ScaleRect.LRC.Y = (f32)DesiredRect.LRC.Y / d.Y;
		}
	}

	//! How left element border is aligned when parent is resized
    GUIAlignment getAlignLeft() const
	{
		return AlignLeft;
	}

	//! How right element border is aligned when parent is resized
    GUIAlignment getAlignRight() const
	{
		return AlignRight;
	}

	//! How top element border is aligned when parent is resized
    GUIAlignment getAlignTop() const
	{
		return AlignTop;
	}

	//! How bottom element border is aligned when parent is resized
    GUIAlignment getAlignBottom() const
	{
		return AlignBottom;
	}

	//! Updates the absolute position.
	virtual void updateAbsolutePosition()
	{
		recalculateAbsolutePosition(false);

		// update all children
        for (auto &child : Children) {
			child->updateAbsolutePosition();
		}
	}

	//! Returns the topmost GUI element at the specific position.
	/**
	This will check this GUI element and all of its descendants, so it
	may return this GUI element.  To check all GUI elements, call this
	function on device->getGUIEnvironment()->getRootGUIElement(). Note
	that the root element is the size of the screen, so doing so (with
	an on-screen point) will always return the root element if no other
	element is above it at that point.
	\param point: The point at which to find a GUI element.
	\return The topmost GUI element at that point, or 0 if there are
	no candidate elements at this point.
	*/
    virtual std::shared_ptr<IGUIElement> getElementFromPoint(const v2i &point)
	{
        std::shared_ptr<IGUIElement> target;

		if (isVisible()) {
			// we have to search from back to front, because later children
			// might be drawn over the top of earlier ones.
			auto it = Children.rbegin();
			auto ie = Children.rend();
			while (it != ie) {
				target = (*it)->getElementFromPoint(point);
				if (target)
					return target;

				++it;
			}
		}

		if (isVisible() && isPointInside(point))
            target = std::make_shared<IGUIElement>(this);

		return target;
	}

	//! Returns true if a point is within this element.
	/** Elements with a shape other than a rectangle should override this method */
    virtual bool isPointInside(const v2i &point) const
	{
		return AbsoluteClippingRect.isPointInside(point);
	}

	//! Adds a GUI element as new child of this element.
    virtual void addChild(std::shared_ptr<IGUIElement> child)
	{
        if (child && child.get() != this) {
			addChildToEnd(child);
			child->updateAbsolutePosition();
		}
	}

	//! Removes a child.
    virtual void removeChild(std::list<std::shared_ptr<IGUIElement>>::iterator child_it)
	{
        assert((*child_it)->Parent.get() == this);
        Children.erase(child_it);
	}

	//! Removes all children.
	virtual void removeAllChildren()
	{
        while (!Children.empty())
            removeChild(std::prev(Children.end()));
	}

	//! Removes this element from its parent.
	virtual void remove()
	{
		if (Parent)
            Parent->removeChild(ParentPos);
	}

	//! Draws the element and its children.
	virtual void draw()
	{
		if (isVisible()) {
            for (auto &child : Children)
				child->draw();
		}
	}

	//! animate the element and its children.
	virtual void OnPostRender(u32 timeMs)
	{
		if (isVisible()) {
            for (auto &child : Children)
				child->OnPostRender(timeMs);
		}
	}

	//! Moves this element.
    virtual void move(v2i absoluteMovement)
	{
		setRelativePosition(DesiredRect + absoluteMovement);
	}

	//! Returns true if element is visible.
	virtual bool isVisible() const
	{
		return IsVisible;
	}

	//! Check whether the element is truly visible, taking into accounts its parents' visibility
	/** \return true if the element and all its parents are visible,
	false if this or any parent element is invisible. */
	virtual bool isTrulyVisible() const
	{
		if (!IsVisible)
			return false;

		if (!Parent)
			return true;

		return Parent->isTrulyVisible();
	}

	//! Sets the visible state of this element.
	virtual void setVisible(bool visible)
	{
		IsVisible = visible;
	}

	//! Returns true if this element was created as part of its parent control
	virtual bool isSubElement() const
	{
		return IsSubElement;
	}

	//! Sets whether this control was created as part of its parent.
	/** For example, it is true when a scrollbar is part of a listbox.
	SubElements are not saved to disk when calling guiEnvironment->saveGUI() */
	virtual void setSubElement(bool subElement)
	{
		IsSubElement = subElement;
	}

	//! If set to true, the focus will visit this element when using the tab key to cycle through elements.
	/** If this element is a tab group (see isTabGroup/setTabGroup) then
	ctrl+tab will be used instead. */
	void setTabStop(bool enable)
	{
		IsTabStop = enable;
	}

	//! Returns true if this element can be focused by navigating with the tab key
	bool isTabStop() const
	{
		return IsTabStop;
	}

	//! Sets the priority of focus when using the tab key to navigate between a group of elements.
	/** See setTabGroup, isTabGroup and getTabGroup for information on tab groups.
	Elements with a lower number are focused first */
	void setTabOrder(s32 index)
	{
		// negative = autonumber
		if (index < 0) {
			TabOrder = 0;
            std::shared_ptr<IGUIElement> el = getTabGroup();
			while (IsTabGroup && el && el->Parent)
				el = el->Parent;

            std::shared_ptr<IGUIElement> first, closest;
			if (el) {
				// find the highest element number
				el->getNextElement(-1, true, IsTabGroup, first, closest, true, true);
				if (first) {
					TabOrder = first->getTabOrder() + 1;
				}
			}

		} else
			TabOrder = index;
	}

	//! Returns the number in the tab order sequence
	s32 getTabOrder() const
	{
		return TabOrder;
	}

	//! Sets whether this element is a container for a group of elements which can be navigated using the tab key.
	/** For example, windows are tab groups.
	Groups can be navigated using ctrl+tab, providing isTabStop is true. */
	void setTabGroup(bool isGroup)
	{
		IsTabGroup = isGroup;
	}

	//! Returns true if this element is a tab group.
	bool isTabGroup() const
	{
		return IsTabGroup;
	}

	//! Returns the container element which holds all elements in this element's tab group.
    std::shared_ptr<IGUIElement> getTabGroup()
	{
        std::shared_ptr<IGUIElement> ret(this);

		while (ret && !ret->isTabGroup())
			ret = ret->getParent();

		return ret;
	}

	//! Returns true if element is enabled
	/** Currently elements do _not_ care about parent-states.
		So if you want to affect children you have to enable/disable them all.
		The only exception to this are sub-elements which also check their parent.
	*/
	virtual bool isEnabled() const
	{
		if (isSubElement() && IsEnabled && getParent())
			return getParent()->isEnabled();

		return IsEnabled;
	}

	//! Sets the enabled state of this element.
	virtual void setEnabled(bool enabled)
	{
		IsEnabled = enabled;
	}

	//! Sets the new caption of this element.
	virtual void setText(const wchar_t *text)
	{
		Text = text;
	}

	//! Returns caption of this element.
	virtual const wchar_t *getText() const
	{
		return Text.c_str();
	}

	//! Sets the new caption of this element.
	virtual void setToolTipText(const wchar_t *text)
	{
		ToolTipText = text;
	}

	//! Returns caption of this element.
    virtual const std::wstring &getToolTipText() const
	{
		return ToolTipText;
	}

	//! Returns id. Can be used to identify the element.
	virtual s32 getID() const
	{
		return ID;
	}

	//! Sets the id of this element
	virtual void setID(s32 id)
	{
		ID = id;
	}

	//! Called if an event happened.
    bool OnEvent(const main::Event &event) override
	{
		return Parent ? Parent->OnEvent(event) : false;
	}

	//! Brings a child to front
	/** \return True if successful, false if not. */
    virtual bool bringToFront(std::shared_ptr<IGUIElement> child)
	{
        if (child->Parent.get() != this)
			return false;
		if (std::next(child->ParentPos) == Children.end()) // already there
			return true;
		Children.erase(child->ParentPos);
        child->ParentPos = Children.insert(Children.end(), child);
		return true;
	}

	//! Moves a child to the back, so it's siblings are drawn on top of it
	/** \return True if successful, false if not. */
    virtual bool sendToBack(std::shared_ptr<IGUIElement> child)
	{
        if (child->Parent.get() != this)
			return false;
		if (child->ParentPos == Children.begin()) // already there
			return true;
		Children.erase(child->ParentPos);
        child->ParentPos = Children.insert(Children.begin(), child);
		return true;
	}

	//! Returns list with children of this element
    virtual const std::list<std::shared_ptr<IGUIElement>> &getChildren() const
	{
		return Children;
	}

	//! Finds the first element with the given id.
	/** \param id: Id to search for.
	\param searchchildren: Set this to true, if also children of this
	element may contain the element with the searched id and they
	should be searched too.
	\return Returns the first element with the given id. If no element
	with this id was found, 0 is returned. */
    virtual std::shared_ptr<IGUIElement> getElementFromId(s32 id, bool searchchildren = false) const
	{
        std::shared_ptr<IGUIElement> e;

        for (auto &child : Children) {
			if (child->getID() == id)
                return child;

			if (searchchildren)
				e = child->getElementFromId(id, true);

			if (e)
				return e;
		}

		return e;
	}

	//! returns true if the given element is a child of this one.
	//! \param child: The child element to check
    bool isMyChild(std::shared_ptr<IGUIElement> child) const
	{
		if (!child)
			return false;
		do {
			if (child->Parent)
				child = child->Parent;

        } while (child->Parent && child.get() != this);

        return child.get() == this;
	}

	//! searches elements to find the closest next element to tab to
	/** \param startOrder: The TabOrder of the current element, -1 if none
	\param reverse: true if searching for a lower number
	\param group: true if searching for a higher one
	\param first: element with the highest/lowest known tab order depending on search direction
	\param closest: the closest match, depending on tab order and direction
	\param includeInvisible: includes invisible elements in the search (default=false)
	\param includeDisabled: includes disabled elements in the search (default=false)
	\return true if successfully found an element, false to continue searching/fail */
	bool getNextElement(s32 startOrder, bool reverse, bool group,
            std::shared_ptr<IGUIElement> &first, std::shared_ptr<IGUIElement> &closest, bool includeInvisible = false,
			bool includeDisabled = false) const
	{
		// we'll stop searching if we find this number
		s32 wanted = startOrder + (reverse ? -1 : 1);
		if (wanted == -2)
			wanted = 1073741824; // maximum s32

		auto it = Children.begin();

		s32 closestOrder, currentOrder;

		while (it != Children.end()) {
			// ignore invisible elements and their children
			if (((*it)->isVisible() || includeInvisible) &&
					(group == true || (*it)->isTabGroup() == false)) {
				// ignore disabled, but children are checked (disabled is currently per element ignoring parent states)
				if ((*it)->isEnabled() || includeDisabled) {
					// only check tab stops and those with the same group status
					if ((*it)->isTabStop() && ((*it)->isTabGroup() == group)) {
						currentOrder = (*it)->getTabOrder();

						// is this what we're looking for?
						if (currentOrder == wanted) {
                            closest = *it;
							return true;
						}

						// is it closer than the current closest?
						if (closest) {
							closestOrder = closest->getTabOrder();
							if ((reverse && currentOrder > closestOrder && currentOrder < startOrder) || (!reverse && currentOrder < closestOrder && currentOrder > startOrder)) {
                                closest = *it;
							}
						} else if ((reverse && currentOrder < startOrder) || (!reverse && currentOrder > startOrder)) {
                            closest = *it;
						}

						// is it before the current first?
						if (first) {
							closestOrder = first->getTabOrder();

							if ((reverse && closestOrder < currentOrder) || (!reverse && closestOrder > currentOrder)) {
                                first = *it;
							}
						} else {
                            first = *it;
						}
					}
				}
				// search within children
				if ((*it)->getNextElement(startOrder, reverse, group, first, closest, includeInvisible, includeDisabled)) {
					return true;
				}
			}
			++it;
		}
		return false;
	}

	//! Returns the type of the gui element.
	/** This is needed for the .NET wrapper but will be used
	later for serializing and deserializing.
	If you wrote your own GUIElements, you need to set the type for your element as first parameter
	in the constructor of IGUIElement. For own (=unknown) elements, simply use EGUIET_ELEMENT as type */
    GUIElementType getType() const
	{
		return Type;
	}

	//! Returns true if the gui element supports the given type.
	/** This is mostly used to check if you can cast a gui element to the class that goes with the type.
	Most gui elements will only support their own type, but if you derive your own classes from interfaces
	you can overload this function and add a check for the type of the base-class additionally.
	This allows for checks comparable to the dynamic_cast of c++ with enabled rtti.
	Note that you can't do that by calling BaseClass::hasType(type), but you have to do an explicit
	comparison check, because otherwise the base class usually just checks for the member variable
	Type which contains the type of your derived class.
	*/
    virtual bool hasType(GUIElementType type) const
	{
		return type == Type;
	}

	//! Returns the name of the element.
	/** \return Name as character string. */
	virtual const c8 *getName() const
	{
		return Name.c_str();
	}

	//! Sets the name of the element.
	/** \param name New name of the gui element. */
	virtual void setName(const c8 *name)
	{
		Name = name;
	}

	//! Sets the name of the element.
	/** \param name New name of the gui element. */
    virtual void setName(const std::string &name)
	{
		Name = name;
	}

	//! Returns whether the element takes input from the IME
	virtual bool acceptsIME()
	{
		return false;
	}

protected:
	// not virtual because needed in constructor
    void addChildToEnd(std::shared_ptr<IGUIElement> child)
	{
		if (child) {
			child->remove(); // remove from old parent
			child->LastParentRect = getAbsolutePosition();
            child->Parent = std::make_shared<IGUIElement>(this);
			child->ParentPos = Children.insert(Children.end(), child);
		}
	}

#ifndef NDEBUG
	template <typename Iterator>
	static size_t _fastSetChecksum(Iterator begin, Iterator end)
	{
		std::hash<typename Iterator::value_type> hasher;
		size_t checksum = 0;
		for (Iterator it = begin; it != end; ++it) {
			size_t h = hasher(*it);
			checksum ^= 966073049 + (h * 3432918353) + ((h >> 16) * 461845907);
		}
		return checksum;
	}
#endif

	// Reorder children [from, to) to the order given by `neworder`
	void reorderChildren(
            std::list<std::shared_ptr<IGUIElement>>::iterator from,
            std::list<std::shared_ptr<IGUIElement>>::iterator to,
            const std::vector<std::shared_ptr<IGUIElement>> &neworder)
	{
		assert(_fastSetChecksum(from, to) == _fastSetChecksum(neworder.begin(), neworder.end()));
		for (auto e : neworder) {
			*from = e;
			e->ParentPos = from;
			++from;
		}
		assert(from == to);
	}

	// not virtual because needed in constructor
	void recalculateAbsolutePosition(bool recursive)
	{
        recti parentAbsolute(0, 0, 0, 0);
        recti parentAbsoluteClip;
		f32 fw = 0.f, fh = 0.f;

		if (Parent) {
			parentAbsolute = Parent->AbsoluteRect;

			if (NoClip) {
                std::shared_ptr<IGUIElement> p(this);
				while (p->Parent)
					p = p->Parent;
				parentAbsoluteClip = p->AbsoluteClippingRect;
			} else
				parentAbsoluteClip = Parent->AbsoluteClippingRect;
		}

		const s32 diffx = parentAbsolute.getWidth() - LastParentRect.getWidth();
		const s32 diffy = parentAbsolute.getHeight() - LastParentRect.getHeight();

        if (AlignLeft == GUIAlignment::Scale || AlignRight == GUIAlignment::Scale)
			fw = (f32)parentAbsolute.getWidth();

        if (AlignTop == GUIAlignment::Scale || AlignBottom == GUIAlignment::Scale)
			fh = (f32)parentAbsolute.getHeight();

		switch (AlignLeft) {
        case GUIAlignment::UpperLeft:
			break;
        case GUIAlignment::LowerRight:
            DesiredRect.ULC.X += diffx;
			break;
        case GUIAlignment::Center:
            DesiredRect.ULC.X += diffx / 2;
			break;
        case GUIAlignment::Scale:
            DesiredRect.ULC.X = round32(ScaleRect.ULC.X * fw);
			break;
		}

		switch (AlignRight) {
        case GUIAlignment::UpperLeft:
			break;
        case GUIAlignment::LowerRight:
            DesiredRect.LRC.X += diffx;
			break;
        case GUIAlignment::Center:
            DesiredRect.LRC.X += diffx / 2;
			break;
        case GUIAlignment::Scale:
            DesiredRect.LRC.X = round32(ScaleRect.LRC.X * fw);
			break;
		}

		switch (AlignTop) {
        case GUIAlignment::UpperLeft:
			break;
        case GUIAlignment::LowerRight:
            DesiredRect.ULC.Y += diffy;
			break;
        case GUIAlignment::Center:
            DesiredRect.ULC.Y += diffy / 2;
			break;
        case GUIAlignment::Scale:
            DesiredRect.ULC.Y = round32(ScaleRect.ULC.Y * fh);
			break;
		}

		switch (AlignBottom) {
        case GUIAlignment::UpperLeft:
			break;
        case GUIAlignment::LowerRight:
            DesiredRect.LRC.Y += diffy;
			break;
        case GUIAlignment::Center:
            DesiredRect.LRC.Y += diffy / 2;
			break;
        case GUIAlignment::Scale:
            DesiredRect.LRC.Y = round32(ScaleRect.LRC.Y * fh);
			break;
		}

		RelativeRect = DesiredRect;

		const s32 w = RelativeRect.getWidth();
		const s32 h = RelativeRect.getHeight();

		// make sure the desired rectangle is allowed
        if (w < (s32)MinSize.X)
            RelativeRect.LRC.X = RelativeRect.ULC.X + MinSize.X;
        if (h < (s32)MinSize.Y)
            RelativeRect.LRC.Y = RelativeRect.ULC.Y + MinSize.Y;
        if (MaxSize.X && w > (s32)MaxSize.X)
            RelativeRect.LRC.X = RelativeRect.ULC.X + MaxSize.X;
        if (MaxSize.Y && h > (s32)MaxSize.Y)
            RelativeRect.LRC.Y = RelativeRect.ULC.Y + MaxSize.Y;

		RelativeRect.repair();

        AbsoluteRect = RelativeRect + parentAbsolute.ULC;

		if (!Parent)
			parentAbsoluteClip = AbsoluteRect;

		AbsoluteClippingRect = AbsoluteRect;
		AbsoluteClippingRect.clipAgainst(parentAbsoluteClip);

		LastParentRect = parentAbsolute;

		if (recursive) {
			// update all children
			for (auto child : Children) {
				child->recalculateAbsolutePosition(recursive);
			}
		}
	}

protected:
	//! List of all children of this element
    std::list<std::shared_ptr<IGUIElement>> Children;

	//! Pointer to the parent
    std::shared_ptr<IGUIElement> Parent;

	//! Our position in the parent list. Only valid when Parent != nullptr
    std::list<std::shared_ptr<IGUIElement>>::iterator ParentPos;

	//! relative rect of element
    recti RelativeRect;

	//! absolute rect of element
    recti AbsoluteRect;

	//! absolute clipping rect of element
    recti AbsoluteClippingRect;

	//! the rectangle the element would prefer to be,
	//! if it was not constrained by parent or max/min size
    recti DesiredRect;

	//! for calculating the difference when resizing parent
    recti LastParentRect;

	//! relative scale of the element inside its parent
    rectf ScaleRect;

	//! maximum and minimum size of the element
    v2u MaxSize, MinSize;

	//! is visible?
	bool IsVisible;

	//! is enabled?
	bool IsEnabled;

	//! is a part of a larger whole and should not be serialized?
	bool IsSubElement;

	//! does this element ignore its parent's clipping rectangle?
	bool NoClip;

	//! caption
    std::wstring Text;

	//! tooltip
    std::wstring ToolTipText;

	//! users can set this for identifying the element by string
    std::string Name;

	//! users can set this for identifying the element by integer
	s32 ID;

	//! tab stop like in windows
	bool IsTabStop;

	//! tab order
	s32 TabOrder;

	//! tab groups are containers like windows, use ctrl+tab to navigate
	bool IsTabGroup;

	//! tells the element how to act when its parent is resized
    GUIAlignment AlignLeft, AlignRight, AlignTop, AlignBottom;

	//! GUI Environment
	IGUIEnvironment *Environment;

	//! type of element
    GUIElementType Type;
};
