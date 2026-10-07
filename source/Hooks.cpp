#include "Hooks.h"

#include "Minimap.h"

void AcceptHUDMenu(RE::HUDMenu* a_hudMenu, RE::FxDelegateHandler::CallbackProcessor* a_gameDelegate)
{
	hooks::HUDMenu::Accept(a_hudMenu, a_gameDelegate);

	a_gameDelegate->Process("SetLocalMapExtents",
		[](const RE::FxDelegateArgs& a_delegateArgs) -> void
		{
			if (auto miniMap = DEM::Minimap::GetSingleton(); miniMap && miniMap->IsInitialized())
			{
				miniMap->SetLocalMapExtents(a_delegateArgs);
			}
		});
}

void AdvanceMovieHUDMenu(RE::HUDMenu* a_hudMenu, float a_interval, std::uint32_t a_currentTime)
{
	hooks::HUDMenu::AdvanceMovie(a_hudMenu, a_interval, a_currentTime);

	if (auto miniMap = DEM::Minimap::GetSingleton())
	{
		// Only render offscreen while there is something to render, so the HUD behaves like vanilla otherwise
		static const bool hudRendersOffscreenTargets = a_hudMenu->menuFlags.all(RE::UI_MENU_FLAGS::kRendersOffscreenTargets);

		if (miniMap->IsVisible() && miniMap->IsShown())
		{
			a_hudMenu->menuFlags.set(RE::UI_MENU_FLAGS::kRendersOffscreenTargets);
		}
		else if (!hudRendersOffscreenTargets)
		{
			a_hudMenu->menuFlags.reset(RE::UI_MENU_FLAGS::kRendersOffscreenTargets);
		}

		miniMap->Advance();
	}
}

void PreDisplayHUDMenu(RE::HUDMenu* a_hudMenu)
{
	if (auto miniMap = DEM::Minimap::GetSingleton(); miniMap && miniMap->IsVisible())
	{
		miniMap->PreRender();
	}

	hooks::HUDMenu::PreDisplay(a_hudMenu);
}

void RefreshPlatformHUDMenu(RE::HUDMenu* a_hudMenu)
{
	hooks::HUDMenu::RefreshPlatform(a_hudMenu);

	if (auto miniMap = DEM::Minimap::GetSingleton())
	{
		miniMap->RefreshPlatform();
	}
}

bool CanProcessMenuOpenHandler(RE::MenuOpenHandler* a_menuOpenHandler, RE::InputEvent* a_event)
{
	auto miniMap = DEM::Minimap::GetSingleton();

	if (miniMap && DEM::Minimap::HasCustomGamepadToggle())
	{
		if (a_event->GetDevice() == RE::INPUT_DEVICE::kGamepad)
		{
			if (RE::ButtonEvent* buttonEvent = a_event->AsButtonEvent())
			{
				// Track the combo even in menus, so the modifier state is never stale
				bool isToggle = miniMap->UpdateGamepadToggle(buttonEvent);

				// Keep the combo from also triggering the button's own control (e.g. Back -> Tween Menu)
				if (isToggle && miniMap->IsInitialized() && !RE::UI__IsInMenuMode())
				{
					return false;
				}
			}
		}

		return hooks::MenuOpenHandler::CanProcess(a_menuOpenHandler, a_event);
	}

	// Only defer the wait button during gameplay. Inside menus the same button is used for other actions
	// (e.g. switching buy/sell in the barter menu), and rewriting its release into a press broke them.
	if (a_event->GetDevice() == RE::INPUT_DEVICE::kGamepad && miniMap && miniMap->IsInitialized() && !RE::UI__IsInMenuMode())
	{
		if (RE::ButtonEvent* buttonEvent = a_event->AsButtonEvent())
		{
			auto userEvents = RE::UserEvents::GetSingleton();

			const RE::BSFixedString& buttonUserEvent = buttonEvent->QUserEvent();

			// Defer wait button
			if (buttonUserEvent == userEvents->wait)
			{
				if (buttonEvent->IsDown())
				{
					return false;
				}
				else if (buttonEvent->IsUp() && !miniMap->IsShown())
				{
					buttonEvent->GetRuntimeData().value = 1.0F;
					buttonEvent->GetRuntimeData().heldDownSecs = 0.0F;
				}
			}
		}
	}

	return hooks::MenuOpenHandler::CanProcess(a_menuOpenHandler, a_event);
}
