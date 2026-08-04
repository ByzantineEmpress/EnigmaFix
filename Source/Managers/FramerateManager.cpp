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
#include "FramerateManager.h"
#include "../Settings/PlayerSettings.h"

auto& PlayerSettingsFrm = EnigmaFix::PlayerSettings::Get();

// Singleton Instance
EnigmaFix::FramerateManager EnigmaFix::FramerateManager::frm_Instance;

namespace EnigmaFix
{
    void FramerateManager::Update()
    {
        QueryPerformanceFrequency(&frequency); // Get the frequency of the performance counter
        while (loop) {
            QueryPerformanceCounter(&currentTime); // Grabs the current time.
            double deltaTimeSeconds = static_cast<double>(currentTime.QuadPart - lastTime) / frequency.QuadPart;
            msTime = deltaTimeSeconds * 1000.0;

            // Avoid division by zero by checking if deltaTimeSeconds is not too small.
            if (deltaTimeSeconds > 0.000001) {
                fps = 1.0 / deltaTimeSeconds; // Corrected framerate calculation
                timeOffset = originalFrTarget / float(deltaTimeSeconds);
            }
            lastTime = currentTime.QuadPart;
        }
    }

    int FramerateManager::Limit()
    {
        // Dynamically update targetFrameTime so UI changes take effect immediately
        if (PlayerSettingsFrm.SYNC.MaxFPS > 0) {
            targetFrameTime = 1.0 / PlayerSettingsFrm.SYNC.MaxFPS;
        } else {
            targetFrameTime = 0.0; // Uncapped
        }

        if (frequency.QuadPart == 0) {
            QueryPerformanceFrequency(&frequency);
        }

        static LARGE_INTEGER lastLimitTime = {0};
        if (lastLimitTime.QuadPart == 0) {
            QueryPerformanceCounter(&lastLimitTime);
        }

        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);

        double elapsedTime = static_cast<double>(currentTime.QuadPart - lastLimitTime.QuadPart) / frequency.QuadPart;
        double remainingTime = targetFrameTime - elapsedTime;

        int totalSleepTime = 0; // Store total sleep time in milliseconds

        if (remainingTime > 0)
        {
            int sleepTime = static_cast<int>(remainingTime * 1000.0); // Convert to milliseconds
            // We want to avoid sleeping the full duration to leave room for the spin-wait precision.
            if (sleepTime > 1)
            {
                Sleep(sleepTime - 1);
                totalSleepTime = sleepTime - 1;
            }

            // Fine-tune timing with a spin-wait
            LARGE_INTEGER spinTime;
            do {
                QueryPerformanceCounter(&spinTime);
            } while (static_cast<double>(spinTime.QuadPart - lastLimitTime.QuadPart) / frequency.QuadPart < targetFrameTime);
            
            currentTime = spinTime;
        }

        // Update lastLimitTime for the next frame
        lastLimitTime = currentTime;

        return totalSleepTime;
    }

    void FramerateManager::Init()
    {
        PlayerSettingsFrm.SYNC.TargetFrameTime = 1.0 / PlayerSettingsFrm.SYNC.MaxFPS;
    }
}
