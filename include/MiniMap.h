#pragma once

#include "IUI/GFxDisplayObject.h"

#include "RE/H/HUDObject.h"

#include "Settings.h"

#include "LMU/API.h"

namespace RE
{
	// Whether input currently goes to a menu instead of gameplay. Uses the control map's context stack
	// instead of a raw game function address, which could not be verified for every runtime.
	inline bool UI__IsInMenuMode()
	{
		auto controlMap = ControlMap::GetSingleton();
		if (!controlMap)
		{
			return false;
		}

		const auto& contextPriorityStack = controlMap->GetRuntimeData().contextPriorityStack;
		return !contextPriorityStack.empty() && contextPriorityStack.back() != UserEvents::INPUT_CONTEXT_ID::kGameplay;
	}
}

namespace DEM
{
	struct ExtraMarker
	{
		struct Type
		{
			enum
			{
				kEnemy,
				kHostile,
				kGuard,
				kDead,
				kTotal
			};
		};

		struct CreateData
		{
			enum
			{
				kName,
				kIconType,
				kStride
			};
		};

		struct RefreshData
		{
			enum
			{
				kX,
				kY,
				kStride
			};
		};
	};

	class Minimap : public RE::HUDObject
	{
	public:
		using Shape = LMU::PixelShaderProperty::Shape;
		using Style = LMU::PixelShaderProperty::Style;

		class InputHandler : public RE::MenuEventHandler
		{
		public:
			InputHandler(Minimap* a_miniMap)
			: miniMap{ a_miniMap }
			{}

			~InputHandler() final{};  // 00

			// override (RE::MenuEventHandler)
			bool CanProcess(RE::InputEvent* a_event) final;				 // 01
			bool ProcessThumbstick(RE::ThumbstickEvent* a_event) final;	 // 03
			bool ProcessMouseMove(RE::MouseMoveEvent* a_event) final;	 // 04
			bool ProcessButton(RE::ButtonEvent* a_event) final;			 // 05

			bool ProcessKeyboardOrMouseButton(RE::ButtonEvent* a_butonEvent);
			bool IsToggleKey(RE::ButtonEvent* a_buttonEvent, std::string_view a_userEventName) const;
			bool IsGamepadToggleButton(RE::ButtonEvent* a_buttonEvent) const;
			bool IsMinimapEvent(RE::InputEvent* a_event) const;
			bool ProcessGamepadButton(RE::ButtonEvent* a_buttonEvent);

			bool IsControllingMinimap() const { return isControllingMinimap; }

			void StartControllingMinimap();
			void StopControllingMinimap();

		private:
			Minimap* miniMap;

			bool isControllingMinimap = false;
			bool areFightingControlsDisabled = false;

			RE::MenuControls* menuControls = RE::MenuControls::GetSingleton();
			RE::ControlMap* controlMap = RE::ControlMap::GetSingleton();
			RE::BSInputDeviceManager* inputDeviceManager = RE::BSInputDeviceManager::GetSingleton();
			RE::UserEvents* userEvents = RE::UserEvents::GetSingleton();

			const float& localMapMousePanSpeed = RE::INISettingCollection::GetSingleton()->GetSetting("fMapLocalMousePanSpeed:MapMenu")->data.f;
			const float& localMapGamepadPanSpeed = RE::INISettingCollection::GetSingleton()->GetSetting("fMapLocalGamepadPanSpeed:MapMenu")->data.f;
			const float& localMapMouseZoomSpeed = RE::INISettingCollection::GetSingleton()->GetSetting("fMapLocalMouseZoomSpeed:MapMenu")->data.f;
			const float& localMapGamepadZoomSpeed = RE::INISettingCollection::GetSingleton()->GetSetting("fMapLocalGamepadZoomSpeed:MapMenu")->data.f;
		};

		static constexpr inline std::string_view path = "_level0.HUDMovieBaseInstance.Minimap";

		// override (RE::HUDObject)
		void Update() final {}											// 01
		bool ProcessMessage(RE::UIMessage* a_message) final;			// 02
		void RegisterHUDComponent(RE::FxDelegateArgs& a_params) final;	// 03

		static void InitSingleton(const IUI::GFxDisplayObject& a_gfxMinimap)
		{
			if (!singleton)
			{
				static Minimap singletonInstance{ a_gfxMinimap };
				singleton = &singletonInstance;
			}
		}

		static Minimap* GetSingleton() { return singleton; }

		void SetLocalMapExtents(const RE::FxDelegateArgs& a_delegateArgs);

		void Advance();
		void PreRender();
		void RefreshPlatform();

		// Controls
		bool IsVisible() const
		{
			RE::GFxValue::DisplayInfo displayInfo;
			displayObj.GetDisplayInfo(&displayInfo);

			return displayInfo.GetVisible();
		}

		bool IsInitialized() const
		{
			return localMap != nullptr;
		}

		bool IsShown() const
		{
			return localMap && localMap_->enabled;
		}

		void Show();
		void Hide();

		void ShowControls();
		void HideControlsAfter(float a_delaySecs);
		void FoldControls();
		void UnfoldControls();

		void ModTranslation(float a_xOffset, float a_yOffset, float a_zOffset = 0.0F)
		{
			RE::NiPoint3 translationOffset = cameraContext->cameraRoot->local.rotate * RE::NiPoint3{ a_zOffset, a_yOffset, a_xOffset };
			cameraContext->defaultState->translation += translationOffset;
		}

		// Custom gamepad toggle (settings::controls::gamepadToggleKey + gamepadToggleModifier)
		static bool HasCustomGamepadToggle() { return settings::controls::gamepadToggleKey != 0; }

		// Fed with every gamepad button event from the MenuOpenHandler hook, which receives them before
		// the minimap input handler. Returns whether the event belongs to an active toggle combo.
		bool UpdateGamepadToggle(RE::ButtonEvent* a_buttonEvent);

		bool IsGamepadToggle(RE::ButtonEvent* a_buttonEvent) const
		{
			return isGamepadToggleActive && a_buttonEvent->GetIDCode() == settings::controls::gamepadToggleKey;
		}

		void ModZoom(float a_zoomMod)
		{
			cameraContext->zoomInput += a_zoomMod;
		}

		// Local Map Upgrade interface
		static inline void (*SetPixelShaderProperties)(LMU::PixelShaderProperty::Shape a_shape, LMU::PixelShaderProperty::Style a_style);
		static inline void (*GetPixelShaderProperties)(LMU::PixelShaderProperty::Shape& a_shape, LMU::PixelShaderProperty::Style& a_style);

	private:
		Minimap(const IUI::GFxDisplayObject& a_gfxMinimap) :
			RE::HUDObject{ a_gfxMinimap.GetMovieView() },
			displayObj{ a_gfxMinimap }
		{
			if (displayObj.HasMember("Minimap"))
			{
				displayObj.Invoke("Minimap", settings::display::positionX, settings::display::positionY);

				float width = displayObj.GetMember("_width").GetNumber();
				displayObj.SetMember("_width", width * settings::display::scale);

				float height = displayObj.GetMember("_height").GetNumber();
				displayObj.SetMember("_height", height * settings::display::scale);
			}
		}

		void InitLocalMap();

		void UpdateFogOfWar();
		void RenderOffScreen();
		void ClearTerrainRenderPasses(RE::NiPointer<RE::NiAVObject>& a_object);
		void CullTerrain(const RE::GridCellArray* a_gridCells, RE::LocalMapMenu::LocalMapCullingProcess::UnkData& a_unkData,
						 const RE::TESObjectCELL* a_cell);

		static inline Minimap* singleton = nullptr;

		// members
		IUI::GFxDisplayObject displayObj;

		Shape shape = static_cast<Shape>(settings::display::shape);

		RE::LocalMapMenu* localMap = nullptr;
		RE::LocalMapMenu::RUNTIME_DATA* localMap_ = nullptr;
		RE::LocalMapMenu::LocalMapCullingProcess* cullingProcess = nullptr;
		RE::LocalMapCamera* cameraContext = nullptr;

		float minCamFrustumHalfWidth = 0.0F;
		float minCamFrustumHalfHeight = 0.0F;

		RE::BSTSmartPointer<InputHandler> inputHandler = RE::make_smart<InputHandler>(this);
		bool isInputHandlerRegistered = false;

		bool isGamepadToggleModifierHeld = false;
		bool isGamepadToggleActive = false;

		bool isCameraUpdatePending = true;

		const char* const& clearedStr = RE::GameSettingCollection::GetSingleton()->GetSetting("sCleared")->data.s;
		const float& localMapHeight = RE::INISettingCollection::GetSingleton()->GetSetting("fMapLocalHeight:MapMenu")->data.f;

		const float& localMapMargin = *REL::Relocation<float*>{ RELOCATION_ID(234438, 189820) }.get();
		const bool& isFogOfWarEnabled = *REL::Relocation<bool*>{ REL::VariantID{ 501260, 359696, 0x1E70DFC } }.get();

		bool& useMapBrightnessAndContrastBoost = *REL::Relocation<bool*>{ RELOCATION_ID(528107, 415052) }.get();
		bool& nodeFadeEnabled = *REL::Relocation<bool*>{ RELOCATION_ID(513141, 390865) }.get();
		bool& nodeDrawFadeEnabled = *REL::Relocation<bool*>{ RELOCATION_ID(513142, 390866) }.get();
		std::uint32_t& dword_1431D0D8C = *REL::Relocation<std::uint32_t*>{ RELOCATION_ID(527629, 414558) }.get();
		bool& byte_1431D1D30 = *REL::Relocation<bool*>{ RELOCATION_ID(527793, 414746) }.get();
	};
}