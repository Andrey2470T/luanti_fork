#include "EventReceiver.h"

bool MyEventReceiver::OnEvent(const SEvent &event)
{
	if (event.EventType == irr::EET_LOG_TEXT_EVENT) {
		static const LogLevel irr_loglev_conv[] = {
			LL_VERBOSE, // ELL_DEBUG
			LL_INFO,    // ELL_INFORMATION
			LL_WARNING, // ELL_WARNING
			LL_ERROR,   // ELL_ERROR
			LL_NONE,    // ELL_NONE
		};
		assert(event.LogEvent.Level < ARRLEN(irr_loglev_conv));
		g_logger.log(irr_loglev_conv[event.LogEvent.Level],
				std::string("Irrlicht: ") + event.LogEvent.Text);
		return true;
	}

	if (event.EventType == EET_APPLICATION_EVENT &&
			event.ApplicationEvent.EventType == EAET_DPI_CHANGED) {
		// This is a fake setting so that we can use (de)registerChangedCallback
		// not only to listen for gui/hud_scaling changes, but also for DPI changes.
		g_settings->setU16("dpi_change_notifier",
				g_settings->getU16("dpi_change_notifier") + 1);
		return true;
	}

	// This is separate from other keyboard handling so that it also works in menus.
	if (event.EventType == EET_KEY_INPUT_EVENT) {
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

	if (event.EventType == EET_MOUSE_INPUT_EVENT && !event.MouseInput.Simulated)
		last_pointer_type = PointerType::Mouse;
	else if (event.EventType == EET_TOUCH_INPUT_EVENT)
		last_pointer_type = PointerType::Touch;

	// Let the menu handle events, if one is active.
	if (isMenuActive()) {
		if (g_touchcontrols)
			g_touchcontrols->setVisible(false);
		return g_menumgr.preprocessEvent(event);
	}

	// Remember whether each key is down or up
	if (event.EventType == irr::EET_KEY_INPUT_EVENT) {
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

	} else if (g_touchcontrols && event.EventType == irr::EET_TOUCH_INPUT_EVENT) {
		// In case of touchcontrols, we have to handle different events
		g_touchcontrols->translateEvent(event);
		return true;
	} else if (event.EventType == irr::EET_JOYSTICK_INPUT_EVENT) {
		// joystick may be nullptr if game is launched with '--random-input' parameter
		return joystick && joystick->handleEvent(event.JoystickEvent);
	} else if (event.EventType == irr::EET_MOUSE_INPUT_EVENT) {
		// Handle mouse events
		switch (event.MouseInput.Event) {
		case EMIE_LMOUSE_PRESSED_DOWN:
			keyIsDown.set(LMBKey);
			keyWasDown.set(LMBKey);
			keyWasPressed.set(LMBKey);
			break;
		case EMIE_MMOUSE_PRESSED_DOWN:
			keyIsDown.set(MMBKey);
			keyWasDown.set(MMBKey);
			keyWasPressed.set(MMBKey);
			break;
		case EMIE_RMOUSE_PRESSED_DOWN:
			keyIsDown.set(RMBKey);
			keyWasDown.set(RMBKey);
			keyWasPressed.set(RMBKey);
			break;
		case EMIE_LMOUSE_LEFT_UP:
			keyIsDown.unset(LMBKey);
			keyWasReleased.set(LMBKey);
			break;
		case EMIE_MMOUSE_LEFT_UP:
			keyIsDown.unset(MMBKey);
			keyWasReleased.set(MMBKey);
			break;
		case EMIE_RMOUSE_LEFT_UP:
			keyIsDown.unset(RMBKey);
			keyWasReleased.set(RMBKey);
			break;
		case EMIE_MOUSE_WHEEL:
			mouse_wheel += event.MouseInput.Wheel;
			break;
		default:
			break;
		}
	}

	// tell Irrlicht to continue processing this event
	return false;
}
