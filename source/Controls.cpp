#include "Minimap.h"

#include "utils/INISettingCollection.h"

namespace DEM
{
	bool Minimap::InputHandler::CanProcess(RE::InputEvent* a_event)
	{
		// The handler stays registered for the whole session. Unregistering here and re-registering
		// from Minimap::ProcessMessage could leave the minimap without input for the rest of the session,
		// as HUD messages do not always reach the minimap.
		if (RE::UI__IsInMenuMode() || !miniMap->IsVisible())
		{
			if (isControllingMinimap)
			{
				StopControllingMinimap();
				miniMap->FoldControls();
			}

			return false;
		}

		// Only take the events the minimap uses. Taking any other event (even without consuming it)
		// changes how MenuControls reports it, and the game then forwards it to menus: e.g. the
		// gamepad button that just opened the Tween Menu reached it and closed it right away.
		return IsMinimapEvent(a_event);
	}

	bool Minimap::InputHandler::IsMinimapEvent(RE::InputEvent* a_event) const
	{
		switch (a_event->GetEventType())
		{
		case RE::INPUT_EVENT_TYPE::kButton:
			{
				RE::ButtonEvent* buttonEvent = a_event->AsButtonEvent();
				RE::INPUT_DEVICE device = buttonEvent->GetDevice();

				if (device == RE::INPUT_DEVICE::kGamepad)
				{
					if (IsGamepadToggleButton(buttonEvent))
					{
						return true;
					}
				}
				else if (device == RE::INPUT_DEVICE::kKeyboard || device == RE::INPUT_DEVICE::kMouse)
				{
					std::string_view userEventName = controlMap->GetUserEventName(buttonEvent->GetIDCode(), device, RE::ControlMap::InputContextID::kMap);
					if (IsToggleKey(buttonEvent, userEventName))
					{
						return true;
					}
				}

				if (isControllingMinimap)
				{
					std::string_view mapUserEventName = controlMap->GetUserEventName(buttonEvent->GetIDCode(), device, RE::ControlMap::InputContextID::kMap);
					return mapUserEventName == userEvents->zoomIn || mapUserEventName == userEvents->zoomOut;
				}

				return false;
			}
		case RE::INPUT_EVENT_TYPE::kThumbstick:
			{
				return isControllingMinimap &&
					   controlMap->GetUserEventName(a_event->AsIDEvent()->GetIDCode(), RE::INPUT_DEVICE::kGamepad, RE::ControlMap::InputContextID::kMap) == userEvents->look;
			}
		case RE::INPUT_EVENT_TYPE::kMouseMove:
			{
				return isControllingMinimap;
			}
		default:
			return false;
		}
	}

	bool Minimap::InputHandler::IsGamepadToggleButton(RE::ButtonEvent* a_buttonEvent) const
	{
		if (HasCustomGamepadToggle())
		{
			return miniMap->IsGamepadToggle(a_buttonEvent);
		}

		std::string_view gameplayUserEventName = controlMap->GetUserEventName(a_buttonEvent->GetIDCode(), RE::INPUT_DEVICE::kGamepad, RE::ControlMap::InputContextID::kGameplay);
		return gameplayUserEventName == userEvents->wait;
	}

	bool Minimap::InputHandler::ProcessThumbstick(RE::ThumbstickEvent* a_event)
	{
		if (isControllingMinimap)
		{
			std::string_view userEventName = controlMap->GetUserEventName(a_event->GetIDCode(), RE::INPUT_DEVICE::kGamepad, RE::ControlMap::InputContextID::kMap);

			if (userEventName == userEvents->look)
			{
				float xOffset = 2 * a_event->xValue * std::abs(a_event->xValue) * localMapGamepadPanSpeed;
				float yOffset = 2 * a_event->yValue * std::abs(a_event->yValue) * localMapGamepadPanSpeed;

				miniMap->ModTranslation(xOffset, yOffset);

				return true;
			}
		}

		// Let the event reach the rest of handlers
		return false;
	}

	bool Minimap::InputHandler::ProcessMouseMove(RE::MouseMoveEvent* a_event)
	{
		if (isControllingMinimap)
		{
			float xOffset = -a_event->mouseInputX * localMapMousePanSpeed;
			float yOffset = a_event->mouseInputY * localMapMousePanSpeed;

			miniMap->ModTranslation(xOffset, yOffset);

			return true;
		}

		return false;
	}
	
	bool Minimap::InputHandler::ProcessButton(RE::ButtonEvent* a_event)
	{
		if (RE::ButtonEvent* buttonEvent = a_event->AsButtonEvent())
		{
			// Do not filter by the active device, users with both a keyboard and a gamepad connected
			// (e.g. with input auto-switching mods) could not toggle the minimap from the keyboard.
			switch (buttonEvent->GetDevice())
			{
			case RE::INPUT_DEVICE::kKeyboard:
			case RE::INPUT_DEVICE::kMouse:
				return ProcessKeyboardOrMouseButton(buttonEvent);
			case RE::INPUT_DEVICE::kGamepad:
				return ProcessGamepadButton(buttonEvent);
			}
		}

		return false;
	}

	bool Minimap::InputHandler::ProcessKeyboardOrMouseButton(RE::ButtonEvent* a_buttonEvent)
	{
		float buttonMag = a_buttonEvent->Value();

		std::string_view userEventName = controlMap->GetUserEventName(a_buttonEvent->GetIDCode(), a_buttonEvent->GetDevice(), RE::ControlMap::InputContextID::kMap);

		bool isProcessed = false;

		if (IsToggleKey(a_buttonEvent, userEventName))
		{
			isProcessed = true;

			bool isPressed = buttonMag ? true : false;
			bool isReleased = !isPressed;
			float heldDownSecs = a_buttonEvent->HeldDuration();

			if (!miniMap->IsShown())
			{
				if (isReleased || (isPressed && heldDownSecs >= 2 * settings::controls::holdDownToControlSecs))
				{
					miniMap->Show();
				}
			}
			else
			{
				if (isReleased && heldDownSecs < settings::controls::holdDownToControlSecs)
				{
					miniMap->Hide();
				}
			}

			if (miniMap->IsShown())
			{
				if (isPressed && heldDownSecs >= settings::controls::holdDownToControlSecs)
				{
					if (!isControllingMinimap)
					{
						StartControllingMinimap();
						miniMap->UnfoldControls();
					}
				}
				else
				{
					if (isControllingMinimap)
					{
						StopControllingMinimap();
						miniMap->FoldControls();
					}

					miniMap->HideControlsAfter(settings::controls::delayToHideControlsSecs);
				}
			}
		}

		if (isControllingMinimap)
		{
			if (userEventName == userEvents->zoomIn)
			{
				miniMap->ModZoom(localMapMouseZoomSpeed);
				isProcessed = true;
			}
			else if (userEventName == userEvents->zoomOut)
			{
				miniMap->ModZoom(-localMapMouseZoomSpeed);
				isProcessed = true;
			}
		}

		// Only consume the events used by the minimap, so the rest of handlers still receive them
		return isProcessed;
	}

	bool Minimap::InputHandler::IsToggleKey(RE::ButtonEvent* a_buttonEvent, std::string_view a_userEventName) const
	{
		if (settings::controls::toggleKey)
		{
			return a_buttonEvent->GetDevice() == RE::INPUT_DEVICE::kKeyboard &&
				   a_buttonEvent->GetIDCode() == settings::controls::toggleKey;
		}

		return a_userEventName == userEvents->localMap;
	}

	bool Minimap::InputHandler::ProcessGamepadButton(RE::ButtonEvent* a_buttonEvent)
	{
		float buttonMag = a_buttonEvent->Value();

		bool isToggle = IsGamepadToggleButton(a_buttonEvent);

		bool isProcessed = false;

		if (isToggle)
		{
			isProcessed = true;

			bool isPressed = buttonMag ? true : false;
			bool isReleased = !isPressed;
			float heldDownSecs = a_buttonEvent->HeldDuration();

			if (!miniMap->IsShown())
			{
				if (isReleased || (isPressed && heldDownSecs >= 2 * settings::controls::holdDownToControlSecs))
				{
					miniMap->Show();
				}
			}
			else
			{
				if (isReleased && heldDownSecs < settings::controls::holdDownToControlSecs)
				{
					miniMap->Hide();
				}
			}

			if (miniMap->IsShown())
			{
				if (isPressed && heldDownSecs >= settings::controls::holdDownToControlSecs)
				{
					if (!isControllingMinimap)
					{
						StartControllingMinimap();
						miniMap->UnfoldControls();
					}
				}
				else
				{
					if (isControllingMinimap)
					{
						StopControllingMinimap();
						miniMap->FoldControls();
					}

					miniMap->HideControlsAfter(settings::controls::delayToHideControlsSecs);
				}
			}
		}

		if (isControllingMinimap)
		{
			std::string_view mapUserEventName = controlMap->GetUserEventName(a_buttonEvent->GetIDCode(), RE::INPUT_DEVICE::kGamepad, RE::ControlMap::InputContextID::kMap);

			if (mapUserEventName == userEvents->zoomIn)
			{
				miniMap->ModZoom(buttonMag * localMapGamepadZoomSpeed);
				isProcessed = true;
			}
			else if (mapUserEventName == userEvents->zoomOut)
			{
				miniMap->ModZoom(-buttonMag * localMapGamepadZoomSpeed);
				isProcessed = true;
			}
		}

		return isProcessed;
	}

	bool Minimap::UpdateGamepadToggle(RE::ButtonEvent* a_buttonEvent)
	{
		std::uint32_t idCode = a_buttonEvent->GetIDCode();

		if (settings::controls::gamepadToggleModifier && idCode == settings::controls::gamepadToggleModifier)
		{
			isGamepadToggleModifierHeld = a_buttonEvent->IsPressed();
		}
		else if (idCode == settings::controls::gamepadToggleKey)
		{
			// Decide on the initial press only, so releasing the modifier first still ends the combo
			if (a_buttonEvent->IsDown())
			{
				isGamepadToggleActive = !settings::controls::gamepadToggleModifier || isGamepadToggleModifierHeld;
			}

			return isGamepadToggleActive;
		}

		return false;
	}

	void Minimap::InputHandler::StartControllingMinimap()
	{
		isControllingMinimap = true;

		miniMap->ShowControls();

		controlMap->ToggleControls(RE::ControlMap::UEFlag::kWheelZoom, false);
		controlMap->ToggleControls(RE::ControlMap::UEFlag::kLooking, false);

		// Remember it, the active device can change while controlling the minimap (e.g. input auto-switching mods)
		areFightingControlsDisabled = inputDeviceManager->IsGamepadEnabled();
		if (areFightingControlsDisabled)
		{
			controlMap->ToggleControls(RE::ControlMap::UEFlag::kFighting, false);
		}
	}

	void Minimap::InputHandler::StopControllingMinimap()
	{
		isControllingMinimap = false;

		miniMap->HideControlsAfter(settings::controls::delayToHideControlsSecs);

		controlMap->ToggleControls(RE::ControlMap::UEFlag::kWheelZoom, true);
		controlMap->ToggleControls(RE::ControlMap::UEFlag::kLooking, true);

		if (areFightingControlsDisabled)
		{
			controlMap->ToggleControls(RE::ControlMap::UEFlag::kFighting, true);
			areFightingControlsDisabled = false;
		}
	}

	void Minimap::Show()
	{

		settings::display::showOnGameStart = true;

		auto iniSettingCollection = utils::INISettingCollection::GetSingleton();
		if (auto showOnGameStart = iniSettingCollection->GetSetting("bShowOnGameStart:Display"))
		{
			showOnGameStart->data.b = settings::display::showOnGameStart;
			iniSettingCollection->WriteSetting(showOnGameStart);
		}

		localMap_->inForeground = localMap_->enabled = true;
		localMap_->root.Invoke("Show", std::array<RE::GFxValue, 1>{ true });
		ShowControls();
	}

	void Minimap::Hide()
	{

		settings::display::showOnGameStart = false;

		auto iniSettingCollection = utils::INISettingCollection::GetSingleton();
		if (auto showOnGameStart = iniSettingCollection->GetSetting("bShowOnGameStart:Display"))
		{
			showOnGameStart->data.b = settings::display::showOnGameStart;
			iniSettingCollection->WriteSetting(showOnGameStart);
		}

		localMap_->inForeground = localMap_->enabled = false;
		localMap_->root.Invoke("Show", std::array<RE::GFxValue, 1>{ false });
	}

	void Minimap::ShowControls()
	{
		localMap_->root.Invoke("ShowControls");
	}

	void Minimap::HideControlsAfter(float a_delaySecs)
	{
		localMap_->root.Invoke("HideControls", std::array<RE::GFxValue, 1>{ a_delaySecs });
	}

	void Minimap::FoldControls()
	{
		localMap_->root.Invoke("FoldControls", std::array<RE::GFxValue, 1>{ settings::display::controlHideTip });
	}

	void Minimap::UnfoldControls()
	{
		localMap_->root.Invoke("UnfoldControls", std::array<RE::GFxValue, 2>{ settings::display::controlMoveTip, settings::display::controlZoomTip });
	}
}