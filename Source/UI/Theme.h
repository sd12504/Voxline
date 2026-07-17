#pragma once

#include <JuceHeader.h>

struct VoxlineTheme
{
    // === Editor & Window ===
    juce::Colour editorBg;
    juce::Colour mainCardBg;
    juce::Colour panelBg;
    juce::Colour panelBorder;

    // === Text ===
    juce::Colour textPrimary;
    juce::Colour textSecondary;
    juce::Colour textMuted;

    // === Accents (knob active arcs, active buttons) ===
    juce::Colour accentRose;
    juce::Colour accentPeach;
    juce::Colour accentAmber;
    juce::Colour accentPurple;
    juce::Colour accentLavender;

    // === Meters ===
    juce::Colour meterHigh;
    juce::Colour meterMid;
    juce::Colour meterLow;

    // === Knob-specific ===
    juce::Colour knobBodyTop;
    juce::Colour knobBodyBottom;
    juce::Colour knobBorder;
    juce::Colour knobShadow;
    juce::Colour inactiveArc;
    juce::Colour pointer;
    juce::Colour knobValueText;
    juce::Colour knobLabelText;

    // === Shadows / Glows ===
    juce::Colour shadowLight;
    juce::Colour shadowMedium;

    // === Fixed product theme ===
    static const VoxlineTheme dark;

    static const VoxlineTheme& get(int)
    {
        return dark;
    }
};
