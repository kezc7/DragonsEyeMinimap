#include "Settings.h"

#include "utils/INISettingCollection.h"

namespace settings
{
	using namespace utils;

	void Init(const std::string& a_iniFileName)
	{
		INISettingCollection* iniSettingCollection = INISettingCollection::GetSingleton();

		{
			using namespace debug;
			iniSettingCollection->AddSettings(
				MakeSetting("uLogLevel:Debug", static_cast<std::uint32_t>(logLevel)));
		}

		{
			using namespace display;
			iniSettingCollection->AddSettings(
				MakeSetting("fPositionX:Display", positionX),
				MakeSetting("fPositionY:Display", positionY),
				MakeSetting("fScale:Display", scale),
				MakeSetting("uShape:Display", shape),
				MakeSetting("bShowOnGameStart:Display", showOnGameStart),
				MakeSetting("sControlHideTip:Display", controlHideTip),
				MakeSetting("sControlMoveTip:Display", controlMoveTip),
				MakeSetting("sControlZoomTip:Display", controlZoomTip)
			);
		}

		{
			using namespace controls;
			iniSettingCollection->AddSettings(
				MakeSetting("uToggleKey:Controls", toggleKey),
				MakeSetting("uGamepadToggleKey:Controls", gamepadToggleKey),
				MakeSetting("uGamepadToggleModifier:Controls", gamepadToggleModifier),
				MakeSetting("bFollowPlayerCameraRotation:Controls", followPlayerCameraRotation),
				MakeSetting("fHoldDownToControlSecs:Controls", holdDownToControlSecs),
				MakeSetting("fDelayToHideControlsSecs:Controls", delayToHideControlsSecs)
			);
		}

		if (!iniSettingCollection->ReadFromFile(a_iniFileName))
		{
			logger::warn("Could not read {}, falling back to default options", a_iniFileName);
		}

		{
			using namespace debug;
			std::uint32_t iniLogLevel = iniSettingCollection->GetSetting<std::uint32_t>("uLogLevel:Debug");
			logLevel = static_cast<logger::level>(std::min<std::uint32_t>(iniLogLevel, logger::level::off));
		}

		{
			using namespace display;
			positionX = iniSettingCollection->GetSetting<float>("fPositionX:Display");
			positionY = iniSettingCollection->GetSetting<float>("fPositionY:Display");
			scale = iniSettingCollection->GetSetting<float>("fScale:Display");
			shape = iniSettingCollection->GetSetting<std::uint32_t>("uShape:Display");
			showOnGameStart = iniSettingCollection->GetSetting<bool>("bShowOnGameStart:Display");
			controlHideTip = iniSettingCollection->GetSetting<const char*>("sControlHideTip:Display");
			controlMoveTip = iniSettingCollection->GetSetting<const char*>("sControlMoveTip:Display");
			controlZoomTip = iniSettingCollection->GetSetting<const char*>("sControlZoomTip:Display");
		}

		{
			using namespace controls;
			toggleKey = iniSettingCollection->GetSetting<std::uint32_t>("uToggleKey:Controls");
			gamepadToggleKey = iniSettingCollection->GetSetting<std::uint32_t>("uGamepadToggleKey:Controls");
			gamepadToggleModifier = iniSettingCollection->GetSetting<std::uint32_t>("uGamepadToggleModifier:Controls");
			followPlayerCameraRotation = iniSettingCollection->GetSetting<bool>("bFollowPlayerCameraRotation:Controls");
			holdDownToControlSecs = iniSettingCollection->GetSetting<float>("fHoldDownToControlSecs:Controls");
			delayToHideControlsSecs = iniSettingCollection->GetSetting<float>("fDelayToHideControlsSecs:Controls");
		}
	}
}