#include "eventreceiver.h"
#include "joystickcontroller.h"
#include "settings.h"
#include <assert.h>
#include "log_internal.h"
#include "gui/mainmenumanager.h"
#include "gui/touchcontrols.h"

bool MyEventReceiver::OnEvent(const Event &event)
{
    if (event.Type == ET_LOG_TEXT_EVENT) {
        assert(event.Log.Level < LL_MAX);
        g_logger.log(event.Log.Level,
                std::string("Irrlicht: ") + event.Log.Text);
		return true;
    }

    if (event.Type == ET_APPLICATION_EVENT &&
        event.Application.Type == AET_DPI_CHANGED) {
		// This is a fake setting so that we can use (de)registerChangedCallback
		// not only to listen for gui/hud_scaling changes, but also for DPI changes.
		g_settings->setU16("dpi_change_notifier",
				g_settings->getU16("dpi_change_notifier") + 1);
		return true;
	}

	// This is separate from other keyboard handling so that it also works in menus.
    if (event.Type == ET_KEY_INPUT_EVENT) {
		const KeyPress keyCode(event.KeyInput);
		if (keyCode == getKeySetting("keymap_fullscreen")) {
			if (event.KeyInput.PressedDown && !fullscreen_is_down) {
				IrrlichtDevice *device = RenderingEngine::get_raw_device();

				bool new_fullscreen = !device->isFullscreen();
				// Only update the setting if toggling succeeds - it always fails
				// if Minetest was built without SDL.
				if (device->setFullscreen(new_fullscreen)) {
					g_settings->setBool("fullscreen", new_fullscreen);
				}
			}
			fullscreen_is_down = event.KeyInput.PressedDown;
			return true;
		}
	}

    if (event.Type == ET_MOUSE_INPUT_EVENT && !event.MouseInput.Simulated)
		last_pointer_type = PointerType::Mouse;
    else if (event.Type == ET_TOUCH_INPUT_EVENT)
		last_pointer_type = PointerType::Touch;

	// Let the menu handle events, if one is active.
	if (isMenuActive()) {
		if (g_touchcontrols)
			g_touchcontrols->setVisible(false);
		return g_menumgr.preprocessEvent(event);
	}

	// Remember whether each key is down or up
    if (event.Type == ET_KEY_INPUT_EVENT) {
		const KeyPress keyCode(event.KeyInput);
		if (keysListenedFor[keyCode]) {
			if (event.KeyInput.PressedDown) {
				if (!IsKeyDown(keyCode))
					keyWasPressed.set(keyCode);

				keyIsDown.set(keyCode);
				keyWasDown.set(keyCode);
			} else {
				if (IsKeyDown(keyCode))
					keyWasReleased.set(keyCode);

				keyIsDown.unset(keyCode);
			}

			return true;
		}

    } else if (g_touchcontrols && event.Type == ET_TOUCH_INPUT_EVENT) {
		// In case of touchcontrols, we have to handle different events
		g_touchcontrols->translateEvent(event);
		return true;
    } else if (event.Type == ET_JOYSTICK_INPUT_EVENT) {
		// joystick may be nullptr if game is launched with '--random-input' parameter
        return joystick && joystick->handleEvent(event.Joystick);
    } else if (event.Type == ET_MOUSE_INPUT_EVENT) {
		// Handle mouse events
        switch (event.MouseInput.Type) {
        case MIE_LMOUSE_PRESSED_DOWN:
			keyIsDown.set(LMBKey);
			keyWasDown.set(LMBKey);
			keyWasPressed.set(LMBKey);
			break;
        case MIE_MMOUSE_PRESSED_DOWN:
			keyIsDown.set(MMBKey);
			keyWasDown.set(MMBKey);
			keyWasPressed.set(MMBKey);
			break;
        case MIE_RMOUSE_PRESSED_DOWN:
			keyIsDown.set(RMBKey);
			keyWasDown.set(RMBKey);
			keyWasPressed.set(RMBKey);
			break;
        case MIE_LMOUSE_LEFT_UP:
			keyIsDown.unset(LMBKey);
			keyWasReleased.set(LMBKey);
			break;
        case MIE_MMOUSE_LEFT_UP:
			keyIsDown.unset(MMBKey);
			keyWasReleased.set(MMBKey);
			break;
        case MIE_RMOUSE_LEFT_UP:
			keyIsDown.unset(RMBKey);
			keyWasReleased.set(RMBKey);
			break;
        case MIE_MOUSE_WHEEL:
            mouse_wheel += event.MouseInput.WheelDelta;
			break;
		default:
			break;
		}
	}

	// tell Irrlicht to continue processing this event
	return false;
}
