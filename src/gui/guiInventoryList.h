// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include "inventorymanager.h"
#include "IGUIElement.h"
#include "util/string.h"
#include <Render/TTFont.h>

class GUIFormSpecMenu;
class UIRects;

class GUIInventoryList : public gui::IGUIElement
{
public:
	struct ItemSpec
	{
		ItemSpec() = default;

		ItemSpec(const InventoryLocation &a_inventoryloc,
				const std::string &a_listname,
				s32 a_i,
				const v2i slotsize) :
			inventoryloc(a_inventoryloc),
			listname(a_listname),
			i(a_i),
			slotsize(slotsize)
		{
		}

		bool operator==(const ItemSpec& other) const
		{
			return inventoryloc == other.inventoryloc &&
					listname == other.listname && i == other.i;
		}

		bool isValid() const { return i != -1; }

		InventoryLocation inventoryloc;
		std::string listname;
		s32 i = -1;
		v2i slotsize;
	};

	// options for inventorylists that are setable with the lua api
	struct Options {
		// whether a one-pixel border for the slots should be drawn and its color
		bool slotborder = false;
		img::color8 slotbordercolor = img::color8(img::PF_RGBA8, 0, 0, 0, 200);
		// colors for normal and highlighted slot background
		img::color8 slotbg_n = img::color8(img::PF_RGBA8, 128, 128, 128, 255);
		img::color8 slotbg_h = img::color8(img::PF_RGBA8, 192, 192, 192, 255);
	};

	GUIInventoryList(gui::IGUIEnvironment *env,
		gui::IGUIElement *parent,
		s32 id,
		const recti &rectangle,
		InventoryManager *invmgr,
		const InventoryLocation &inventoryloc,
		const std::string &listname,
		const v2i &geom,
		const s32 start_item_i,
		const v2i &slot_size,
		const v2f &slot_spacing,
		GUIFormSpecMenu *fs_menu,
		const Options &options,
		render::TTFont *font);

    void updateMesh() override;
	virtual void draw() override;

	virtual bool OnEvent(const core::Event &event) override;

	const InventoryLocation &getInventoryloc() const
	{
		return m_inventoryloc;
	}

	const std::string &getListname() const
	{
		return m_listname;
	}

	void setSlotBGColors(const img::color8 &slotbg_n, const img::color8 &slotbg_h)
	{
        if (slotbg_n != m_options.slotbg_n || slotbg_h != m_options.slotbg_h)
            Rebuild = true;
		m_options.slotbg_n = slotbg_n;
		m_options.slotbg_h = slotbg_h;
	}

	void setSlotBorders(bool slotborder, const img::color8 &slotbordercolor)
	{
        if (slotborder != m_options.slotborder || slotbordercolor != m_options.slotbordercolor)
            Rebuild = true;
		m_options.slotborder = slotborder;
		m_options.slotbordercolor = slotbordercolor;
	}

	const v2i getSlotSize() const noexcept
	{
		return m_slot_size;
	}

	// returns -1 if not item is at pos p
    s32 getItemIndexAtPos(v2i p);

private:
	InventoryManager *m_invmgr;
	const InventoryLocation m_inventoryloc;
	const std::string m_listname;

	// the specified width and height of the shown inventorylist in itemslots
	const v2i m_geom;
	// the first item's index in inventory
	const s32 m_start_item_i;

	// specifies how large the slot rects are
	const v2i m_slot_size;
	// specifies how large the space between slots is (space between is spacing-size)
	const v2f m_slot_spacing;

	// the GUIFormSpecMenu can have an item selected and co.
	GUIFormSpecMenu *m_fs_menu;

	Options m_options;

	// the font
	render::TTFont *m_font;
	
    std::unique_ptr<UIRects> m_slots_rects;

	// the index of the hovered item; -1 if no item is hovered
	s32 m_hovered_i;

	// we do not want to write a warning on every draw
	bool m_already_warned;
};
