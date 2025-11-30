#pragma once

#include <BasicIncludes.h>
#include <Utils/Line3D.h>
#include "util/pointedthing.h"

class Client;
class RenderSystem;
class LocalPlayer;
class RenderCAO;
struct ItemStack;
struct ItemDefinition;
class NodeMetadata;
class InputHandler;
class ClientPacketHandler;

class PlayerInteraction
{
	Client *client;
	RenderSystem *rndsys;
	LocalPlayer *player;
    InputHandler *input;
    ClientPacketHandler *pkt_handler;

	f32 time_from_last_punch = 10.0f;
    f32 nodig_delay_timer;
    f32 dig_time;
    f32 dig_time_complete;
    bool btn_down_for_dig;
    bool dig_instantly;
    bool digging_blocked;
    u16 dig_index;
    f32 object_hit_delay_timer;

    f32 repeat_place_timer;
    f32 repeat_dig_time;

    bool digging;
    bool punching;
    
    RenderCAO *selected_object = nullptr;

    const float object_hit_delay = 0.2f;

	bool touch_use_crosshair;
    f32 repeat_place_time;

	PointedThing updatePointedThing(
		const line3f &shootline, bool liquids_pointable,
		const std::optional<Pointabilities> &pointabilities,
		bool look_for_object, const v3s16 &camera_offset);

    bool nodePlacement(const ItemDefinition &selected_def, const ItemStack &selected_item,
            const v3s16 &nodepos, const v3s16 &neighborpos, const PointedThing &pointed,
            const NodeMetadata *meta);

    static void settingChangedCallback(const std::string &setting_name, void *data);
    void readSettings();

    friend class LocalPlayer;
public:
    bool pointing_at_object = true;
    PointedThing pointed_old;

	PlayerInteraction(Client *_client);
	
	bool isTouchCrosshairDisabled();

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
