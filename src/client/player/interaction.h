#pragma once

#include <BasicIncludes.h>

class LocalPlayer;


class PlayerInteraction
{
	LocalPlayer *player;

	f32 time_from_last_punch = 10.0f;
    f32 nodig_delay_timer;
    f32 object_hit_delay_timer;

	bool touch_use_crosshair;

	PointedThing updatePointedThing(
		const line3f &shootline, bool liquids_pointable,
		const std::optional<Pointabilities> &pointabilities,
		bool look_for_object, const v3s16 &camera_offset);

	bool isTouchCrosshairDisabled();
public:
	PlayerInteraction() = default;

    /*!
     * Returns the object or node the player is pointing at.
     * Also updates the selected thing in the Hud.
     *
     * @param[in]  shootline         the shootline, starting from
     * the camera position. This also gives the maximal distance
     * of the search.
     * @param[in]  liquids_pointable if false, liquids are ignored
     * @param[in]  pointabilities    item specific pointable overriding
     * @param[in]  look_for_object   if false, objects are ignored
     * @param[in]  camera_offset     offset of the camera
     * @param[out] selected_object   the selected object or
     * NULL if not found
     */
	void handlePointingAtNothing(const ItemStack &playerItem);
	void handlePointingAtNode(const PointedThing &pointed,
		const ItemStack &selected_item, const ItemStack &hand_item, f32 dtime);

	void handlePointingAtObject(const PointedThing &pointed, const ItemStack &playeritem,
		const v3f &player_position, bool show_debug);

	void handleDigging(const PointedThing &pointed, const v3s16 &nodepos,
		const ItemStack &selected_item, const ItemStack &hand_item, f32 dtime);

	void updateTimers(f32 dtime);

	void step(f32 dtime);
};
