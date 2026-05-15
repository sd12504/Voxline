#pragma once

#include <JuceHeader.h>

namespace VoxlineAssets
{
    // Icons
    juce::Image loadLogoMark(bool dark, int targetSize = 34);
    juce::Image loadSettingsIcon(bool dark, int targetSize = 24);
    juce::Image loadBypassIcon(bool dark, int targetSize = 24);
    juce::Image loadListenIcon(bool dark, int targetSize = 24);

    // Overlays
    juce::Image loadNoiseTexture(bool dark, int targetW = 512, int targetH = 512);
    juce::Image loadRadialGlow(bool dark, int targetW = 512, int targetH = 512);
}
