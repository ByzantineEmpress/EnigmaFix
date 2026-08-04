/**
EnigmaFix Copyright (c) 2026 Bryce Q.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**/

// Internal Functionality
#include "ConfigManager.h"
#include "../Settings/PlayerSettings.h"
#include "../Utilities/DisplayHelper.h"
// System Libraries
#include <iostream>
#include <fstream>
#include <regex>
#include <filesystem>
// Third Party Libraries
#include <inipp.h>
#include <spdlog/spdlog.h>
// Variables
auto& PlayerSettingsConf = EnigmaFix::PlayerSettings::Get();
// Namespaces
using namespace std;
// Singleton Instance
EnigmaFix::ConfigManager EnigmaFix::ConfigManager::cm_Instance; // Seemingly need this declared in PlayerSettings.cpp so a bunch of linker errors don't happen.

namespace EnigmaFix {
    inipp::Ini<char> config;
    std::ifstream is("Config.ini");

    void ConfigManager::Init() {
        ifstream configName("Config.ini");
        config.parse(configName);
        config.generate(cout);
        config.default_section(config.sections["Settings"]);
        config.interpolate();
    }


    void ConfigManager::SaveConfig() { 
        if (AlreadyReadConfig) {
            spdlog::info("Saving Config...");

            std::ifstream is("Config.ini");
            std::string content((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
            is.close();

            auto replace = [&](const std::string& key, const std::string& value) {
                std::regex re("(^|\\n)([ \\t]*)" + key + "[ \\t]*=[ \\t]*([^\\n\\r]*)", std::regex_constants::icase);
                content = std::regex_replace(content, re, "$1$2" + key + " = " + value);
            };

            auto btos = [](bool b) { return b ? "true" : "false"; };

            // Resolution Settings
            replace("UseCustomResolution", btos(PlayerSettingsConf.RES.UseCustomRes));
            replace("HorizontalResolution", std::to_string(PlayerSettingsConf.RES.Resolution.x));
            replace("VerticalResolution", std::to_string(PlayerSettingsConf.RES.Resolution.y));
            replace("UseResolutionScale", btos(PlayerSettingsConf.RES.UseCustomResScale));
            replace("ResolutionScalePercentage", std::to_string(PlayerSettingsConf.RES.CustomResScale));

            // FOV Settings
            replace("UseCustomFOV", btos(PlayerSettingsConf.FOV.UseCustomFOV));
            replace("FieldOfView", std::to_string(PlayerSettingsConf.FOV.FieldOfView));
            replace("UseAdaptiveFOVScaling", btos(PlayerSettingsConf.FOV.AdaptiveFOVScaling));

            // Sync and Framerate Settings
            replace("MaxFPS", std::to_string(PlayerSettingsConf.SYNC.MaxFPS));
            replace("VSync", btos(PlayerSettingsConf.SYNC.VSync));
            replace("SyncInterval", std::to_string(PlayerSettingsConf.SYNC.SyncInterval));

            // Rendering Settings
            replace("CameraDistortion", btos(PlayerSettingsConf.RS.CameraDistortion));
            replace("EdgeRendering", btos(PlayerSettingsConf.RS.EdgeRendering));
            replace("ColorCorrection", btos(PlayerSettingsConf.RS.ColorCorrection));
            replace("DepthOfField", btos(PlayerSettingsConf.RS.DepthOfField));
            replace("Fog", btos(PlayerSettingsConf.RS.Fog));
            replace("Foliage", btos(PlayerSettingsConf.RS.FoliageRendering));
            replace("Bloom", btos(PlayerSettingsConf.RS.Bloom));
            replace("IBL", btos(PlayerSettingsConf.RS.IBL));
            replace("LensFlare", btos(PlayerSettingsConf.RS.LensFlare));
            replace("MotionBlur", btos(PlayerSettingsConf.RS.MotionBlur));
            replace("MotionBlurPreset", std::to_string(PlayerSettingsConf.RS.MotionBlurPreset));
            replace("RLRLighting", btos(PlayerSettingsConf.RS.RLRLighting));
            replace("Shadows", btos(PlayerSettingsConf.RS.Shadows));
            replace("ShadowResolution", std::to_string(PlayerSettingsConf.RS.ShadowRes));
            replace("SSAO", btos(PlayerSettingsConf.RS.SSAO));
            replace("SSR", btos(PlayerSettingsConf.RS.SSR));
            replace("TAA", btos(PlayerSettingsConf.RS.TAA));
            replace("Tonemapping", btos(PlayerSettingsConf.RS.Tonemapping));
            replace("Vignette", btos(PlayerSettingsConf.RS.Vignette));

            // Input Settings
            replace("KBMPrompts", btos(PlayerSettingsConf.IS.KBMPrompts));
            replace("DisableSteamInput", btos(PlayerSettingsConf.IS.DisableSteamInput));
            const char* InputOptions[]{ "Auto", "Xbox", "PlayStation", "Switch" };
            int inputIdx = PlayerSettingsConf.IS.InputDeviceType;
            if (inputIdx >= 0 && inputIdx < 4) {
                replace("InputType", InputOptions[inputIdx]);
            }

            // Misc Settings
            replace("SkipOpeningVideos", btos(PlayerSettingsConf.MS.SkipOpeningVideos));
            replace("CameraTweaks", btos(PlayerSettingsConf.MS.CameraTweaks));
            replace("EnableConsoleLog", btos(PlayerSettingsConf.MS.EnableConsoleLog));

            // Launcher Settings
            replace("IgnoreUpdates", btos(PlayerSettingsConf.LS.IgnoreUpdates));

            std::ofstream os("Config.ini");
            os << content;
        }
    }

    void ConfigManager::ReadConfig() {
        spdlog::info("Reading Config...");
        cout.flush();
        cout.clear();
        cin.clear();
        Init();

        // Resolution Settings
        inipp::extract(config.sections["Resolution"]["useCustomResolution"], PlayerSettingsConf.RES.UseCustomRes);
        inipp::extract(config.sections["Resolution"]["HorizontalResolution"], PlayerSettingsConf.RES.Resolution.x);
        inipp::extract(config.sections["Resolution"]["VerticalResolution"], PlayerSettingsConf.RES.Resolution.y);
        inipp::extract(config.sections["Resolution"]["UseResolutionScale"], PlayerSettingsConf.RES.UseCustomResScale);
        inipp::extract(config.sections["Resolution"]["ResolutionScalePercentage"], PlayerSettingsConf.RES.CustomResScale);

        // FOV Settings
        inipp::extract(config.sections["FieldOfView"]["UseCustomFOV"], PlayerSettingsConf.FOV.UseCustomFOV);
        inipp::extract(config.sections["FieldOfView"]["FieldOfView"], PlayerSettingsConf.FOV.FieldOfView);
        inipp::extract(config.sections["FieldOfView"]["UseAdaptiveFOVScaling"], PlayerSettingsConf.FOV.AdaptiveFOVScaling);
        // Sync and Framerate Settings
        inipp::extract(config.sections["Framerate"]["MaxFPS"], PlayerSettingsConf.SYNC.MaxFPS);
        inipp::extract(config.sections["Framerate"]["VSync"], PlayerSettingsConf.SYNC.VSync);
        inipp::extract(config.sections["Framerate"]["SyncInterval"], PlayerSettingsConf.SYNC.SyncInterval);
        // Rendering Settings
        inipp::extract(config.sections["Rendering"]["CameraDistortion"], PlayerSettingsConf.RS.CameraDistortion);
        inipp::extract(config.sections["Rendering"]["EdgeRendering"], PlayerSettingsConf.RS.EdgeRendering);
        inipp::extract(config.sections["Rendering"]["ColorCorrection"], PlayerSettingsConf.RS.ColorCorrection);
        inipp::extract(config.sections["Rendering"]["DepthOfField"], PlayerSettingsConf.RS.DepthOfField);
        inipp::extract(config.sections["Rendering"]["Fog"], PlayerSettingsConf.RS.Fog);
        inipp::extract(config.sections["Rendering"]["Foliage"], PlayerSettingsConf.RS.FoliageRendering);
        inipp::extract(config.sections["Rendering"]["Bloom"], PlayerSettingsConf.RS.Bloom);
        inipp::extract(config.sections["Rendering"]["IBL"], PlayerSettingsConf.RS.IBL);
        inipp::extract(config.sections["Rendering"]["LensFlare"], PlayerSettingsConf.RS.LensFlare);
        inipp::extract(config.sections["Rendering"]["MotionBlur"], PlayerSettingsConf.RS.MotionBlur);
        inipp::extract(config.sections["Rendering"]["MotionBlurPreset"], PlayerSettingsConf.RS.MotionBlurPreset);
        inipp::extract(config.sections["Rendering"]["RLRLighting"], PlayerSettingsConf.RS.RLRLighting);
        inipp::extract(config.sections["Rendering"]["Shadows"], PlayerSettingsConf.RS.Shadows);
        inipp::extract(config.sections["Rendering"]["ShadowResolution"], PlayerSettingsConf.RS.ShadowRes);
        inipp::extract(config.sections["Rendering"]["SSAO"], PlayerSettingsConf.RS.SSAO);
        string ssaoQuality;
        string ssrQuality;
        inipp::extract(config.sections["Rendering"]["SSAOQuality"], ssaoQuality);
        inipp::extract(config.sections["Rendering"]["SSR"], PlayerSettingsConf.RS.SSR);
        inipp::extract(config.sections["Rendering"]["SSRQuality"], ssrQuality);
        inipp::extract(config.sections["Rendering"]["TAA"], PlayerSettingsConf.RS.TAA);
        inipp::extract(config.sections["Rendering"]["Tonemapping"], PlayerSettingsConf.RS.Tonemapping);
        inipp::extract(config.sections["Rendering"]["Vignette"], PlayerSettingsConf.RS.Vignette);
        // Input Settings
        inipp::extract(config.sections["Input"]["KBMPrompts"], PlayerSettingsConf.IS.KBMPrompts);
        inipp::extract(config.sections["Input"]["DisableSteamInput"], PlayerSettingsConf.IS.DisableSteamInput);
        string inputDeviceType;
        inipp::extract(config.sections["Input"]["InputType"], inputDeviceType);
        // Misc Settings
        inipp::extract(config.sections["Misc"]["SkipOpeningVideos"], PlayerSettingsConf.MS.SkipOpeningVideos);
        inipp::extract(config.sections["Misc"]["CameraTweaks"], PlayerSettingsConf.MS.CameraTweaks);
        inipp::extract(config.sections["Misc"]["EnableConsoleLog"], PlayerSettingsConf.MS.EnableConsoleLog);
        string cpuSchedulerMode;
        inipp::extract(config.sections["Misc"]["CPUSchedulerMode"], cpuSchedulerMode);
        // Launcher Settings
        inipp::extract(config.sections["Launcher"]["IgnoreUpdates"], PlayerSettingsConf.LS.IgnoreUpdates);

        // Check if the Horizontal or Vertical Res is 0. If so, default        // If resolution is 0x0, use the current desktop resolution.
        if (PlayerSettingsConf.RES.Resolution.x == 0 || PlayerSettingsConf.RES.Resolution.y == 0) {
            Util::DesktopResolution CurrentResolution = Util::GetCurrentDisplayResolution();
            PlayerSettingsConf.RES.Resolution.x   = CurrentResolution.x;
            PlayerSettingsConf.RES.Resolution.y   = CurrentResolution.y;
            spdlog::info("ConfigManager: Custom Resolution is set to 0x0. Defaulting to {}x{}.", PlayerSettingsConf.RES.Resolution.x, PlayerSettingsConf.RES.Resolution.y);
        }

        // Pre-initialize InternalResolution to the user's custom resolution to prevent a fatal size mismatch
        // during early engine rendering before the dynamic main-render-target detection kicks in.
        if (PlayerSettingsConf.RES.UseCustomRes) {
            PlayerSettingsConf.INS.InternalResolution = PlayerSettingsConf.RES.Resolution;
        }
        AlreadyReadConfig = true; // After the INI file has successfully been read for the first time, allow writing.
    }
}