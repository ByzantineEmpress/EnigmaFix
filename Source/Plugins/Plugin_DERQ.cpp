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
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**/

// Internal Functionality
#include "Plugin_DERQ.h"
#include "../Settings/PlayerSettings.h"
#include "../Utilities/MemoryHelper.h"
#include "../Managers/PatchManager.h"
#include "../Utilities/FOVHelper.h"

// Third Party Libraries
#include <codecvt>
#include <safetyhook.hpp>

#include "../Managers/FramerateManager.h"
#include "../Managers/LogManager.h"
#include "spdlog/spdlog.h"

auto& PlayerSettingsPDQ = EnigmaFix::PlayerSettings::Get();
auto& PatchManagerPDQ = EnigmaFix::PatchManager::Get();
auto& LogManagerPDQ = EnigmaFix::LogManager::Get();
auto& FramerateManagerPDQ = EnigmaFix::FramerateManager::Get();

EnigmaFix::Plugin_DERQ EnigmaFix::Plugin_DERQ::pq_Instance; // Seemingly need this declared in Plugin_DERQ.cpp so a bunch of linker errors don't happen.

namespace EnigmaFix
{

    struct ResolutionPtr {
        int* X;
        int* Y;

        // Constructor that takes offsets and calculates addresses dynamically
        ResolutionPtr(intptr_t xOffset, intptr_t yOffset) {
            uintptr_t baseModule = reinterpret_cast<uintptr_t>(PatchManagerPDQ.BaseModule);
            X = reinterpret_cast<int*>(baseModule + xOffset);
            Y = reinterpret_cast<int*>(baseModule + yOffset);
        }
    };

    ResolutionPtr resolutionList[] = {
        { 0xF58720, 0xF58724 },  // 640x360 (0)
        { 0xF58728, 0xF5872C },  // 720x405 (1)
        { 0xF58730, 0xF58734 },  // 800x450 (2)
        { 0xF58738, 0xF5873C },  // 1024x576 (3)
        { 0xF58740, 0xF58744 },  // 1152x648 (4)
        { 0xF58748, 0xF5874C },  // 1280x720 (5)
        { 0xF58750, 0xF58754 },  // 1360x765 (6)
        { 0xF58758, 0xF5875C },  // 1366x768 (7)
        { 0xF58760, 0xF58764 },  // 1600x900 (8)
        { 0xF58768, 0xF5876C },  // 1920x1080 (9)
        { 0xF58770, 0xF58774 },  // 2560x1440 (10)
        { 0xF58778, 0xF5877C },  // 3840x2160 (11)
    };

    void NOPPattern(HMODULE baseModule, const std::string& pattern, size_t nopsCount, const std::string& patternName)
    {
        if (auto patternAddr = Memory::PatternScan(baseModule, pattern.c_str())) {
            spdlog::info("{} found at: {}", patternName, reinterpret_cast<void*>(patternAddr));

            // NOP out the specified number of bytes (replace with "90")
            for (size_t i = 0; i < nopsCount; ++i) {
                Memory::Write(reinterpret_cast<uintptr_t>(patternAddr + i), static_cast<uint8_t>(0x90));  // Write NOP byte
            }
            spdlog::info("Patched {} bytes of {} with NOPs.", nopsCount, patternName);
        }
    }

    using ResCheckFunctionType = void(*)(char*, int, int);
    ResCheckFunctionType OriginalResCheckFunction = nullptr;
    safetyhook::InlineHook ResolutionPatchHook{};

    void ResCheckFunctionHook(char* param_1, int param_2, int param_3)
    {
        // Removed hardcoded memory patches that caused access violations on different game builds (like GOG).
        return ResolutionPatchHook.call<void>(param_1, param_2, param_3);
    }

    void Plugin_DERQ::ResolutionPatches(HMODULE baseModule)
    {
        // TODO: Find a good place to put this check inside of the game code, just before the resolution change occurs.
        if (auto resolutionCheckFunc = Memory::PatternScan(baseModule, "48 89 ?? ?? ?? 48 89 ?? ?? ?? 57 48 83 EC ?? 48 8B ?? ?? ?? ?? ?? 66 0F")) {
            spdlog::info("Resolution: Found Resolution Check Signature at: {}", reinterpret_cast<void*>(resolutionCheckFunc));

            // Store original function pointer
            OriginalResCheckFunction = reinterpret_cast<ResCheckFunctionType>(resolutionCheckFunc);
            // Disabled hooking this function because the body relies on hardcoded offsets that crash.
            // ResolutionPatchHook = safetyhook::create_inline(OriginalResCheckFunction, ResCheckFunctionHook);
        }

        // Signature for currently selected screen mode index:
        // "09 00 00 00 00 00 00 00 00 00 00 00 FF 01 00 00 4C 00 ?? 00 01 00 02 00 04 ?? 05 ?? ?? ?? ?? 00 08 00 0E 00 0D ?? ?? ?? ?? 00 12 00 13 00 15 ?? ?? ?? ?? 00 19 00 1B 00 1C ?? 1F 00 21 00 23 00 25 ?? ?? ?? ?? 00 29 00 2A 00 2C ?? 2F 00 31 00 32 00 34 ?? 35 ?? ?? ?? ?? 00 3B 00 3C ?? 3D ?? ?? ?? ?? 00 42 ?? 43 00 ?? ?? 48 00 ?? ?? ?? 00 4A ?? 4B 00 ?? ?? 4E 00 ?? ?? 50 00 51 ?? 52 00 53 ?<content omitted. Read with read_file_chars(path="C:/Users/Kyra/.gemini/antigravity/scratch/EnigmaFix/Source/Plugins/Plugin_DERQ.cpp", start_char=8650, chars_to_read=6407)>
        // Signature for currently selected resolution index:
        // "09 00 00 00 00 00 00 00 00 00 00 00 FF 01 00 00 4C 00 ?? 00 01 00 02 00 04 ?? 05 ?? ?? ?? ?? 00 08 00 0E 00 0D ?? ?? ?? ?? 00 12 00 13 00 15 ?? ?? ?? ?? 00 19 00 1B 00 1C ?? 1F 00 21 00 23 00 25 ?? ?? ?? ?? 00 29 00 2A 00 2C ?? 2F 00 31 00 32 00 34 ?? 35 ?? ?? ?? ?? 00 3B 00 3C ?? 3D ?? ?? ?? ?? 00 42 ?? 43 00 ?? ?? 48 00 ?? ?? ?? 00 4A ?? 4B 00 ?? ?? 4E 00 ?? ?? 50 00 51 ?? 52 00 53 ?<content omitted. Read with read_file_chars(path="C:/Users/Kyra/.gemini/antigravity/scratch/EnigmaFix/Source/Plugins/Plugin_DERQ.cpp", start_char=15520, chars_to_read=6419)>

        // Write the custom resolution directly to the game's "4K Native" option variables.
        // This is safe, crash-free, and doesn't require code-hooks because it only writes
        // to global data variables on startup.
        if (PlayerSettingsPDQ.RES.UseCustomRes) {
            auto* hWinSize4KPtr = reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(PatchManagerPDQ.BaseModule) + 0xF58780);
            auto* vWinSize4KPtr = reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(PatchManagerPDQ.BaseModule) + 0xF58784);

            Memory::Write(reinterpret_cast<uintptr_t>(hWinSize4KPtr), PlayerSettingsPDQ.RES.Resolution.x);
            Memory::Write(reinterpret_cast<uintptr_t>(vWinSize4KPtr), PlayerSettingsPDQ.RES.Resolution.y);
            spdlog::info("Resolution: Patched 4K Option Internal Resolution variables to: {}x{}", *hWinSize4KPtr, *vWinSize4KPtr);
        }
    }

    void Plugin_DERQ::AspectRatioPatches(HMODULE baseModule)
    {
        // TODO: Fix these
        // Disable aspect ratio values from being overwritten
        //NOPPattern(baseModule, "89 41 ?? 0F 10 ?? ?? 0F 11 ?? ?? 0F 10 ?? ?? 0F 11 ?? ?? 8B 82", 3, "Aspect Ratio Change Blocker Opcode 1"); // (89 41 50)
        //NOPPattern(baseModule, "F3 0F ?? ?? ?? E8 ?? ?? ?? ?? 85 C0 75", 5, "Aspect Ratio Change Blocker Opcode 2"); // (F3 0F 11 4F 50)
        // NOP out the specified number of bytes (replace with "00")
        for (size_t i = 0; i < 5; ++i) {
            Memory::Write(reinterpret_cast<uintptr_t>(cpuSchedulerPatchIntel + i), static_cast<uint8_t>(cpuSchedulerModeByte));
        }
    }

    void Plugin_DERQ::FOVPatches(HMODULE baseModule)
    {
        // Only hook if custom FOV is enabled in config
        if (!PlayerSettingsPDQ.FOV.UseCustomFOV) {
            return;
        }

        // 1. Hook the simple getter at 0x1406E145F (file 0x6E085F) to always return the default gameplay FOV.
        // This ensures the battle arena (and any other geometry logic using it) does not shrink or stretch!
        // Pattern: F3 0F 10 81 C4 04 00 00 C3
        auto fovGetterFunc = Memory::PatternScan(baseModule, "F3 0F 10 81 C4 04 00 00 C3");
        if (fovGetterFunc) {
            spdlog::info("FOV: Found simple getter signature at: {}", reinterpret_cast<void*>(fovGetterFunc));

            static safetyhook::MidHook fovGetterHook;
            fovGetterHook = safetyhook::create_mid(fovGetterFunc + 8, [](safetyhook::Context& ctx) {
                ctx.xmm0.f32[0] = 43.60281754f; // Override returned value to default gameplay FOV
            });
            spdlog::info("FOV: Successfully initialized battle arena protection hook.");
        } else {
            spdlog::error("FOV: Could not find simple getter signature!");
        }

        // Shared callback function for both FOV write hooks:
        auto fovWriteCallback = [](safetyhook::Context& ctx) {
            auto* base = reinterpret_cast<uint8_t*>(PatchManagerPDQ.BaseModule);
            auto* cameraManagerPtr = *reinterpret_cast<uintptr_t**>(base + 0x1022050);
            if (cameraManagerPtr) {
                auto mainCamera = cameraManagerPtr[8]; // 0x40 offset
                if (ctx.rcx == mainCamera) {
                    float originalFOV = ctx.xmm0.f32[0];

                    // Only override if the written value is close to the default gameplay FOV (43.0f to 46.0f).
                    // This retains director-intended FOV values during in-game cutscenes / cinematics!
                    if (originalFOV >= 43.0f && originalFOV <= 46.0f) {
                        float targetFOV = static_cast<float>(PlayerSettingsPDQ.FOV.FieldOfView);

                        // Adaptive FOV scaling (Vert+) for narrow aspect ratios (like 16:10 or 4:3).
                        if (PlayerSettingsPDQ.FOV.AdaptiveFOVScaling) {
                            float currentAspect = PlayerSettingsPDQ.RES.InternalAspectRatio;
                            targetFOV = FOV::AdjustFOVHorPlusToVertPlus(currentAspect, targetFOV);
                        }

                        ctx.xmm0.f32[0] = targetFOV;
                    }
                }
            }
        };

        // 2. Hook exploration FOV write site at 0x1406E1F73 (file 0x6E1373)
        // Pattern: F3 0F 11 81 C4 04 00 00 48 83 C4 28
        auto fovWriteFunc1 = Memory::PatternScan(baseModule, "F3 0F 11 81 C4 04 00 00 48 83 C4 28");
        if (fovWriteFunc1) {
            spdlog::info("FOV: Found exploration write signature at: {}", reinterpret_cast<void*>(fovWriteFunc1));
            static safetyhook::MidHook fovWriteHook1;
            fovWriteHook1 = safetyhook::create_mid(fovWriteFunc1, fovWriteCallback);
            spdlog::info("FOV: Successfully initialized exploration FOV write hook.");
        } else {
            spdlog::error("FOV: Could not find exploration write signature!");
        }

        // 3. Hook battle / cutscene FOV write site at 0x1406E1A6F (file 0x6E0E6F)
        // Pattern: F3 0F 11 81 C4 04 00 00 C3 CC CC CC CC CC CC CC CC CC CC
        auto fovWriteFunc2 = Memory::PatternScan(baseModule, "F3 0F 11 81 C4 04 00 00 C3 CC CC CC CC CC CC CC CC CC CC");
        if (fovWriteFunc2) {
            spdlog::info("FOV: Found battle/cutscene write signature at: {}", reinterpret_cast<void*>(fovWriteFunc2));
            static safetyhook::MidHook fovWriteHook2;
            fovWriteHook2 = safetyhook::create_mid(fovWriteFunc2, fovWriteCallback);
            spdlog::info("FOV: Successfully initialized battle/cutscene FOV write hook.");
        } else {
            spdlog::error("FOV: Could not find battle/cutscene write signature!");
        }
    }

    void Plugin_DERQ::UIPatches(HMODULE baseModule)
    {
        // Skip Opening Videos
        // The game stores intro movie filenames as C-strings in .rdata.
        // Scanning for the first filename ("game_op.usm") and zeroing the following
        // contiguous block of filenames causes the video player to find no valid
        // path and skip each video silently.
        if (PlayerSettingsPDQ.MS.SkipOpeningVideos) {
            // Anchor: short relative path "game_op.usm" immediately before the full paths
            if (auto movieStrings = Memory::PatternScan(baseModule,
                "67 61 64 65 5F 6F 70 2E 75 73 6D 00 2E 2E 2F 2E 2E 2F 72 65 73 6F 75 72 63 65")) {
                spdlog::info("Skip Intro: Found movie string block at: {}", reinterpret_cast<void*>(movieStrings));
                // Zero out movie filenames only (161 bytes):
                //   game_op.usm (12) + logo_if.usm (48) + logo_ch.usm (48) + logo_silicon.usm (53)
                // NOTE: SYSTEM/WARN/* strings must NOT be zeroed — they are UI screen identifiers,
                // not file paths. Zeroing them causes a black screen hang.
                static const char zeros[161] = {};
                Memory::PatchBytes(reinterpret_cast<uintptr_t>(movieStrings), zeros, 161);
                spdlog::info("Skip Intro: Movie filenames zeroed — videos will be skipped.");
            } else {
                spdlog::error("Skip Intro: Could not find movie string block!");
            }

            // Skip Warning Pages completely by forcing the warning screen config getters to return 0.
            // This is clean, safe, and avoids the black screen state machine hang because the game
            // transitions to the title screen when it thinks there are 0 warnings left.
            auto* scanBytes = reinterpret_cast<uint8_t*>(baseModule);
            auto dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(scanBytes + dosHeader->e_lfanew);
            auto ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(scanBytes + dosHeader->e_lfanew);
            auto sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;

            static const uint8_t sig[] = { 0x48, 0x89, 0x5C, 0x24, 0x08, 0x55, 0x48, 0x8D, 0x6C, 0x24, 0xA9, 0x48, 0x81, 0xEC, 0xA0, 0x00, 0x00, 0x00 };
            int patchCount = 0;
            for (size_t i = 0; i < sizeOfImage - sizeof(sig); ++i) {
                if (memcmp(&scanBytes[i], sig, sizeof(sig)) == 0) {
                    Memory::PatchBytes(reinterpret_cast<uintptr_t>(&scanBytes[i]), "\x31\xC0\xC3", 3);
                    patchCount++;
                }
            }
            spdlog::info("Skip Warnings: Patched {} warning screen getters.", patchCount);
        }
    }

    void Plugin_DERQ::FrameratePatches(HMODULE baseModule)
    {
        static safetyhook::MidHook framerateMidHook;

        if (auto framerateCapFunc = Memory::PatternScan(baseModule, "8B 80 ?? ?? ?? ?? 89 44 ?? ?? 83 7C 24 44 ?? 74 ?? 83 7C 24 44")) {
            spdlog::info("Found Framerate Limiter Signature at: {}", reinterpret_cast<void*>(framerateCapFunc));

            // Hook right after the game reads its internal frame cap setting into eax.
            // We sleep the thread for our custom cap, and then overwrite eax with 999
            // so that the game's default switch statement falls through and bypasses the internal cap.
            framerateMidHook = safetyhook::create_mid(framerateCapFunc + 6, [](safetyhook::Context& ctx) {
                if (PlayerSettingsPDQ.SYNC.MaxFPS > 0) {
                    FramerateManagerPDQ.Limit();
                }

                // Set rax=0 so the engine takes the "no-sleep" branch (xorps xmm0,xmm0),
                // handing all frame pacing control to FramerateManager::Limit() above.
                // Using 999 bypassed the engine's sleep entirely AND decoupled its internal
                // frame budget accounting, causing game logic to run at full speed.
                // rax=0 is the engine's own uncapped mode — safe to use as our base.
                ctx.rax = 0;
            });
        }
    }

    void Plugin_DERQ::SchedulerPatches(HMODULE baseModule)
    {
        int cpuSchedulerMode = 0; // 0 : Original, 1: AMD, 2: Intel
        uint8_t cpuSchedulerModeByte;
        switch (cpuSchedulerMode) {
            case 0: break; // Original, do nothing
            case 1: { // AMD CPU Scheduling
                cpuSchedulerModeByte = 0x00;
                break;
            }
            case 2: { // Intel CPU Scheduling
                cpuSchedulerModeByte = 0x01;
                break;
            }
            default: break;  // Original, do nothing
        }
        if (cpuSchedulerMode != 0 && cpuSchedulerModeByte)
        {
            // TODO: Figure if there's any performance improvement or rammifications from this. If there's an improvement, keep it. If some notice it's better on their setup while others have issues with it, just make it a config option.
            // Use Intel CPU scheduling instead of AMD on AMD CPU (I'm morbidly curious since apparently Cyberpunk 2077 had some issue at launch revolving around it).
            if (auto cpuSchedulerPatchAMD = Memory::PatternScan(baseModule, "C7 44 24 24 ?? ?? ?? ?? EB ?? C7 44 24 24 ?? ?? ?? ?? 8B 44 ?? ?? 89 44 ?? ?? 83 7C 24 20")) {
                spdlog::info("{} found at: {}", "CPU Scheduler for AMD", reinterpret_cast<void*>(cpuSchedulerPatchAMD));
                // NOP out the specified number of bytes (replace with "01")
                for (size_t i = 0; i < 5; ++i) {
                    Memory::Write(reinterpret_cast<uintptr_t>(cpuSchedulerPatchAMD + i), static_cast<uint8_t>(cpuSchedulerModeByte));
                }
            }
            // Use AMD CPU scheduling instead of Intel on Intel CPU.
            if (auto cpuSchedulerPatchIntel = Memory::PatternScan(baseModule, "C7 44 24 24 ?? ?? ?? ?? 8B 44 ?? ?? 89 44 ?? ?? 83 7C 24 20")) {
                spdlog::info("{} found at: {}", "CPU Scheduler for Intel", reinterpret_cast<void*>(cpuSchedulerPatchIntel));
                // NOP out the specified number of bytes (replace with "00")
                for (size_t i = 0; i < 5; ++i) {
                    Memory::Write(reinterpret_cast<uintptr_t>(cpuSchedulerPatchIntel + i), static_cast<uint8_t>(cpuSchedulerModeByte));
                }
            }
        }
    }

    void Plugin_DERQ::GraphicsSettingsPatches(HMODULE baseModule)
    {
        // NOTE: Because you can technically adjust these settings while in-game, maybe I should do the signature scans first, and then find a way to have those reference the PlayerSettings variables.

        // Toggle off post-processing (From what I see, this is only a thing in the CE Table, a debugging thing if you will)
        if (auto postProcessToggleFunc = Memory::PatternScan(baseModule, "83 F8 ?? 75 ?? 41 8B ?? 49 8D")) { // Application.exe+6D8639: cmp eax,01 (83 F8 01)
            spdlog::info("Post Processing: Found Post Processing Toggle Signature at: {}", reinterpret_cast<void*>(postProcessToggleFunc));
        }

        if (PlayerSettingsPDQ.RS.Tonemapping) {
            if (auto tonemappingToggleFunc = Memory::PatternScan(baseModule, "42 88 ?? ?? ?? 48 8D ?? ?? ?? 48 8D ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0 74 ?? 41 0F ?? ?? ?? ?? ?? ?? 49 69 CF ?? ?? ?? ?? 42 88 ?? ?? ?? 41 0F")) { // "Application.exe"+29C420: mov [rcx+r13+70],al (42 88 44 29 70)
                spdlog::info("Post Processing: Found Color Correction Signature at: {}", reinterpret_cast<void*>(tonemappingToggleFunc));
                static SafetyHookMid tonemappingToggleHook;
            }
        }

        if (PlayerSettingsPDQ.RS.ColorCorrection) {
            if (auto colorCorrectionToggleFunc = Memory::PatternScan(baseModule, "42 88 ?? ?? ?? 48 8D ?? ?? ?? 48 8D ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0 74 ?? 66 41")) { // "Application.exe"+29C858: mov [rcx+r13+04],al (42 88 44 29 04)
                spdlog::info("Post Processing: Found Color Correction Signature at: {}", reinterpret_cast<void*>(colorCorrectionToggleFunc));
            }
        }

        if (PlayerSettingsPDQ.RS.LensFlare) {
            if (auto lensFlareToggleFunc = Memory::PatternScan(baseModule, "42 88 ?? ?? ?? ?? ?? ?? 48 8D ?? ?? ?? 48 8D ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0 74 ?? 49 69 C7 ?? ?? ?? ?? 0F 28 ?? 0F 28 ?? F3 41 ?? ?? ?? ?? ?<content omitted. Read with read_file_chars(path="C:/Users/Kyra/.gemini/antigravity/scratch/EnigmaFix/Source/Plugins/Plugin_DERQ.cpp", start_char=39444, chars_to_read=256)>
                spdlog::info("Post Processing: Found Lens Flare Signature at: {}", reinterpret_cast<void*>(lensFlareToggleFunc));
            }
        }

        if (PlayerSettingsPDQ.RS.Fog) {
            if (auto fogToggleFunc = Memory::PatternScan(baseModule, "88 44 ?? ?? 48 8D ?? ?? ?? 48 8D ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0 74 ?? 49 6B C7 ?? 0F 28 ?? 0F 28 ?? F3 41 ?? ?? ?? ?? ?<content omitted. Read with read_file_chars(path="C:/Users/Kyra/.gemini/antigravity/scratch/EnigmaFix/Source/Plugins/Plugin_DERQ.cpp", start_char=39444, chars_to_read=256)>
                spdlog::info("Post Processing: Found Fog Signature at: {}", reinterpret_cast<void*>(fogToggleFunc));
            }
        }

        if (PlayerSettingsPDQ.RS.RLRLighting) {
            // Disable IBL Lighting:
            // E8 ?? ?? ?? ?? 49 8B ?? E8 ?? ?? ?? ?? 48 8D ?? ?? 4C 89 (Application.exe+2981AD - E8 8E AE FF FF - call Application.exe+293040) This needs to have a switch statement that checks things before running it.

            // In the post processing settings
            if (auto rlrLightingToggleFunc = Memory::PatternScan(baseModule, "42 88 ?? ?? ?? ?? ?? ?? 48 8D ?? ?? ?? 48 8D ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0 74 ?? 49 69 C7 ?? ?? ?? ?? 0F 28 ?? 0F 28 ?? F3 41 ?? ?? ?? ?? ?<content omitted. Read with read_file_chars(path="C:/Users/Kyra/.gemini/antigravity/scratch/EnigmaFix/Source/Plugins/Plugin_DERQ.cpp", start_char=41320, chars_to_read=246)>
                spdlog::info("Post Processing: Found RLR Lighting Signature at: {}", reinterpret_cast<void*>(rlrLightingToggleFunc));
            }
        }

        if (PlayerSettingsPDQ.RS.CameraDistortion) {
            if (auto cameraDistortionToggleFunc = Memory::PatternScan(baseModule, "42 88 ?? ?? ?? ?? ?? ?? 48 8D ?? ?? ?? 48 8D ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0 74 ?? 49 69 C7 ?? ?? ?? ?? 0F 28 ?? 0F 28 ?? F3 41 ?? ?? ?? ?? ?<content omitted. Read with read_file_chars(path="C:/Users/Kyra/.gemini/antigravity/scratch/EnigmaFix/Source/Plugins/Plugin_DERQ.cpp", start_char=39444, chars_to_read=256)>
                spdlog::info("Post Processing: Found Camera Distortion Signature at: {}", reinterpret_cast<void*>(cameraDistortionToggleFunc));
            }
        }

        if (PlayerSettingsPDQ.RS.Bloom) {
            if (auto bloomToggleFunc = Memory::PatternScan(baseModule, "42 88 ?? ?? ?? ?? ?? ?? 48 8D ?? ?? ?? 48 8D ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0 74 ?? 66 41 ?? ?? ?? ?? ?<content omitted. Read with read_file_chars(path="C:/Users/Kyra/.gemini/antigravity/scratch/EnigmaFix/Source/Plugins/Plugin_DERQ.cpp", start_char=39444, chars_to_read=256)>
                spdlog::info("Post Processing: Found Bloom Signature at: {}", reinterpret_cast<void*>(bloomToggleFunc));
            }
        }

        if (PlayerSettingsPDQ.RS.MotionBlur) {
            if (auto motionBlurToggleFunc = Memory::PatternScan(baseModule, "42 88 ?? ?? ?? 48 8D ?? ?? ?? 48 8D ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0 74 ?? 41 8B")) { // "Application.exe"+29CFF3: mov [rcx+r13+58],al (42 88 44 29 58)
                spdlog::info("Post Processing: Found Motion Blur Signature at: {}", reinterpret_cast<void*>(motionBlurToggleFunc));
            }
            //switch (PlayerSettingsPDQ.RS.MotionBlurPreset) {
                //case 0:  // Disabled, will probably keep this around for motion vectors
                //case 1:  // Short
                //case 2:  // Medium
                //case 3:  // Long
                //default: // Shutter Ratio (Application.exe+7735D8, default: 0.67542696) and Max Blur Length (Application.exe+25EE50, default: 0.1000000015)
            //}
        }

        if (PlayerSettingsPDQ.RS.DepthOfField) {
            if (auto dofToggleFunc = Memory::PatternScan(baseModule, "42 88 ?? ?? ?? ?? ?? ?? 48 8D ?? ?? ?? 48 8D ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0 74 ?? 49 69 C7 ?? ?? ?? ?? 0F 28 ?? 0F 28 ?? F3 41 ?? ?? ?? ?<content omitted. Read with read_file_chars(path="C:/Users/Kyra/.gemini/antigravity/scratch/EnigmaFix/Source/Plugins/Plugin_DERQ.cpp", start_char=39444, chars_to_read=256)>
                spdlog::info("Post Processing: Found Depth of Field Signature at: {}", reinterpret_cast<void*>(dofToggleFunc));
            }
        }

        if (PlayerSettingsPDQ.RS.SSAO) {
            if (auto ssaoToggleFunc = Memory::PatternScan(baseModule, "42 88 ?? ?? ?? ?? ?? ?? 48 8D ?? ?? ?? 48 8D ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0 74 ?? 66 41 ?? ?? ?? ?? ?<content omitted. Read with read_file_chars(path="C:/Users/Kyra/.gemini/antigravity/scratch/EnigmaFix/Source/Plugins/Plugin_DERQ.cpp", start_char=39444, chars_to_read=256)>
                spdlog::info("Post Processing: Found SSAO Signature at: {}", reinterpret_cast<void*>(ssaoToggleFunc));
            }
        }

        if (PlayerSettingsPDQ.RS.TAA) {
            if (auto taaToggleFunc = Memory::PatternScan(baseModule, "42 88 ?? ?? ?? 48 8D ?? ?? ?? 48 8D ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0 74 ?? 49 69 C7")) { // "Application.exe"+29CF8D: mov [rcx+r13+69],al (42 88 44 29 69)
                spdlog::info("Post Processing: Found TAA Signature at: {}", reinterpret_cast<void*>(taaToggleFunc));
            }
            if (auto aaToggleFunc = Memory::PatternScan(baseModule, "42 88 ?? ?? ?? 48 8D ?? ?? ?? 48 8D ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0 74 ?? 41 0F ?? ?? ?? ?? ?? ?? 49 69 CF ?? ?? ?? ?? 42 88 ?? ?? ?? 48 8D")) { // "Application.exe"+29CF64: mov [rcx+r13+68],al (42 88 44 29 68)
                spdlog::info("Post Processing: Found Post AA Signature at: {}", reinterpret_cast<void*>(aaToggleFunc));
            }
        }

        if (!PlayerSettingsPDQ.RS.Vignette) { // This one works a little bit differently, as you need to move a new float variable to the xmm1 register BEFORE the opcode is done, that way we can override the float variable used for the vignette intensity.
            if (auto vignetteFunc = Memory::PatternScan(baseModule, "F3 42 ?? ?? ?? ?? ?? ?? ?? ?? ?<content omitted. Read with read_file_chars(path="C:/Users/Kyra/.gemini/antigravity/scratch/EnigmaFix/Source/Plugins/Plugin_DERQ.cpp", start_char=39444, chars_to_read=256)>
                spdlog::info("Post Processing: Found Vignette Signature at: {}", reinterpret_cast<void*>(vignetteFunc));
                static SafetyHookMid vignetteFuncMidHook{};
                vignetteFuncMidHook = safetyhook::create_mid(vignetteFunc,
                    [](SafetyHookContext& ctx) {
                        ctx.xmm1.f32[0] = 0.0f; // The default is "0.200000003"
                    });
            }
        }
    }

    void Plugin_DERQ::CameraPatches(HMODULE baseModule)
    {

    }

    void Plugin_DERQ::PhotoModePatches(HMODULE baseModule)
    {

    }

    std::string ConvertUTF16toUTF8(const wchar_t* wstr) {
        std::string utf8_str;
        const wchar_t* ptr = wstr;

        while (*ptr) {
            // Assuming UTF-16 with most characters fitting into 3 bytes in UTF-8
            if (*ptr < 0x80) {
                utf8_str.push_back(static_cast<char>(*ptr));  // ASCII range
            } else if (*ptr < 0x800) {
                utf8_str.push_back(static_cast<char>(0xC0 | (*ptr >> 6)));
                utf8_str.push_back(static_cast<char>(0x80 | (*ptr & 0x3F)));
            } else {
                utf8_str.push_back(static_cast<char>(0xE0 | (*ptr >> 12)));
                utf8_str.push_back(static_cast<char>(0x80 | ((*ptr >> 6) & 0x3F)));
                utf8_str.push_back(static_cast<char>(0x80 | (*ptr & 0x3F)));
            }
            ++ptr;
        }
        return utf8_str;
    }

    void HookedLoggingFunction(int code, const char* format, ...) {
        if (!format) return;  // Avoid null format strings

        // Allocate a buffer for the formatted message
        char buffer[1024];

        // Process variable arguments correctly
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        // Check for Japanese characters (heuristic: if `format` contains wide chars)
        std::string message = buffer;
        if (strstr(format, "%ls")) {  // If the format string expects wide strings
            const wchar_t* wideStr = reinterpret_cast<const wchar_t*>(buffer);
            message = ConvertUTF16toUTF8(wideStr);
        }

        // Switches the log type based on the output.
        switch (code) {
            case 0:  spdlog::info("[Game Log] {}", message);  break;
            case 1:  spdlog::warn("[Game Log] {}", message);  break;
            case 2:  spdlog::error("[Game Log] {}", message); break;
            default: spdlog::info("[Game Log] [Unknown Code {}] {}", code, message); break;
        }
    }

    void Plugin_DERQ::LoggingPatches(HMODULE baseModule)
    {


        if (auto loggingFunc = Memory::PatternScan(baseModule, "4C 89 44 24 ?? 4C 89 4C 24 ?? C3 CC CC CC CC CC 48 8b 01")) {
            spdlog::info("Logging: Found Engine Logging Function Signature at: {}", reinterpret_cast<void*>(loggingFunc));
            using LoggingFunctionType = void(*)(int, const char*, ...);
            LoggingFunctionType OriginalLoggingFunction = nullptr;

            // Store original function pointer
            OriginalLoggingFunction = reinterpret_cast<LoggingFunctionType>(loggingFunc);
            static auto hook = safetyhook::create_inline(OriginalLoggingFunction, HookedLoggingFunction);
            // TODO: Print the logs to the console.
        }
    }

    // ALT+F4 Window Signatures:

    // Battle Movement Decoupling Hooks
    // These hooks scale enemy/entity movement by the dynamic FPS scale (60.0f / currentFPS)
    // to keep gameplay speed consistent regardless of framerate. This prevents enemies from
    // moving too fast at high framerates while keeping game logic decoupled.

    void Plugin_DERQ::BattleMovementPatches(HMODULE baseModule) {
        // Battle Movement Decoupling Hooks
        // These hooks scale enemy/entity movement by the dynamic FPS scale (60.0f / currentFPS)
        // to keep gameplay speed consistent regardless of framerate. This prevents enemies from
        // moving too fast at high framerates while keeping game logic decoupled.

        static safetyhook::MidHook battleMovementHook1;
        if (auto battleSig1 = Memory::PatternScan(baseModule, "F3 0F 10 4C 24 50")) {
            spdlog::info("Battle Movement: Found Fix 1 Signature at: {}", reinterpret_cast<void*>(battleSig1));

            // Hook at offset +0x2A from the signature start
            static constexpr int HOOK_OFFSET_1 = 0x2A;
            battleMovementHook1 = safetyhook::create_mid(battleSig1 + HOOK_OFFSET_1, [](safetyhook::Context& ctx) {
                float dynamicFPS = 60.0f / FramerateManagerPDQ.fps;
                ctx.xmm1.f32[0] *= dynamicFPS;
                ctx.xmm2.f32[0] *= dynamicFPS;
            });
        } else {
            spdlog::error("Battle Movement: Could not find Fix 1 Signature!");
        }

        static safetyhook::MidHook battleMovementHook2;
        if (auto battleSig2 = Memory::PatternScan(baseModule, "F3 0F 10 4D 10 F3 0F 10 55 14 3C 01 75 12")) {
            spdlog::info("Battle Movement: Found Fix 2 Signature at: {}", reinterpret_cast<void*>(battleSig2));

            // Hook at offset +0x30 from the signature start
            static constexpr int HOOK_OFFSET_2 = 0x30;
            battleMovementHook2 = safetyhook::create_mid(battleSig2 + HOOK_OFFSET_2, [](safetyhook::Context& ctx) {
                float dynamicFPS = 60.0f / FramerateManagerPDQ.fps;
                ctx.xmm1.f32[0] *= dynamicFPS;
                ctx.xmm2.f32[0] *= dynamicFPS;
            });
        } else {
            spdlog::error("Battle Movement: Could not find Fix 2 Signature!");
        }
    }}

    void Plugin_DERQ::AnimationPatches(HMODULE baseModule) {
        // Animation Interpolation Speed Decoupling
        static safetyhook::MidHook animInterpHook;
        if (auto animSig = Memory::PatternScan(baseModule, "F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? EB ?? F3 0F ?? ?? ?? ?? EB ?? F3 0F ?? ?? ?? ?? ?<content omitted. Read with read_file_chars(path="C:/Users/Kyra/.gemini/antigravity/scratch/EnigmaFix/Source/Plugins/Plugin_DERQ.cpp", start_char=17538, chars_to_read=224)>)) {
            spdlog::info("Animation: Found Animation Interpolation Signature at: {}", reinterpret_cast<void*>(animSig));

            // Hook at offset +0x08 from the signature start
            static constexpr int HOOK_OFFSET_ANIM = 0x08;
            animInterpHook = safetyhook::create_mid(animSig + HOOK_OFFSET_ANIM, [](safetyhook::Context& ctx) {
                float dynamicFPS = 60.0f / FramerateManagerPDQ.fps;
                ctx.xmm0.f32[0] = dynamicFPS;
            });
        } else {
            spdlog::error("Animation: Could not find Animation Interpolation Signature!");
        }
    }}

    void Plugin_DERQ::TwoDEffectsPatches(HMODULE baseModule) {
        // 2D Effect Speed Decoupling
        static safetyhook::MidHook effectSpeedHook;
        if (auto effectSig = Memory::PatternScan(baseModule, "F3 0F ?? ?? ?? ?? ?? ?? 0F 57 ?? F3 0F ?? ?? ?? 0F 2F ?? 0F 83")) {
            spdlog::info("2D Effects: Found Effect Signature at: {}", reinterpret_cast<void*>(effectSig));

            // Hook at offset +0x08 from the signature start
            static constexpr int HOOK_OFFSET_EFFECT = 0x08;
            effectSpeedHook = safetyhook::create_mid(effectSig + HOOK_OFFSET_EFFECT, [](safetyhook::Context& ctx) {
                float dynamicFPS = 60.0f / FramerateManagerPDQ.fps;
                ctx.xmm4.f32[0] -= 1.0f + dynamicFPS;
            });
        } else {
            spdlog::error("2D Effects: Could not find Effect Signature!");
        }
    }}

    void Plugin_DERQ::Live2DPatches(HMODULE baseModule) {
        // Live2D Animation Speed Decoupling
        static safetyhook::MidHook live2DHook;
        if (auto live2DSig = Memory::PatternScan(baseModule, "F3 44 ?? ?? ?? ?? ?? ?? ?? 45 0F ?? ?? ?? ?? ?<content omitted. Read with read_file_chars(path="C:/Users/Kyra/.gemini/antigravity/scratch/EnigmaFix/Source/Plugins/Plugin_DERQ.cpp", start_char=17538, chars_to_read=224)>)) {
            spdlog::info("Live2D: Found Live2D Signature at: {}", reinterpret_cast<void*>(live2DSig));

            // Hook at offset +0x09 from the signature start
            static constexpr int HOOK_OFFSET_LIVE2D = 0x09;
            live2DHook = safetyhook::create_mid(live2DSig + HOOK_OFFSET_LIVE2D, [](safetyhook::Context& ctx) {
                float dynamicFPS = 60.0f / FramerateManagerPDQ.fps;
                ctx.xmm10.f32[0] *= dynamicFPS;
            });
        } else {
            spdlog::error("Live2D: Could not find Live2D Signature!");
        }
    }}

    // ALT+F4 Window Signatures:
    // FF 15 ?? ?? ?? ?? 83 F8 ?? 0F 85
    // 83 F8 ?? 0F 85 ?? ?? ?? ?? 45 89 ?? 33 C0
    // FF 15 ?? ?? ?? ?? 83 F8 ?? 0F 85
    // 83 F8 ?? 0F 85 ?? ?? ?? ?? 45 89 ?? 33 C0
} // EnigmaFix