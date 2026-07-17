#include "Theme.h"

const VoxlineTheme VoxlineTheme::dark =
{
    // == Editor & Window ==
    .editorBg        = juce::Colour(0xff0E0F0F),
    .mainCardBg      = juce::Colour(0xff141515),
    .panelBg         = juce::Colour(0xff191A1A),
    .panelBorder     = juce::Colour(0xff303131),

    // == Text ==
    .textPrimary     = juce::Colour(0xffEEE8DF),
    .textSecondary   = juce::Colour(0xffA49D94),
    .textMuted       = juce::Colour(0xff69655F),

    // == Accents ==
    .accentRose      = juce::Colour(0xffF06A3D),
    .accentPeach     = juce::Colour(0xffF06A3D),
    .accentAmber     = juce::Colour(0xffD99158),
    .accentPurple    = juce::Colour(0xffF06A3D),
    .accentLavender  = juce::Colour(0xffF06A3D),

    // == Meters ==
    .meterHigh       = juce::Colour(0xffD96A3D),
    .meterMid        = juce::Colour(0xffB79B45),
    .meterLow        = juce::Colour(0xff66864B),

    // == Knob ==
    .knobBodyTop     = juce::Colour(0xff343536),
    .knobBodyBottom  = juce::Colour(0xff171819),
    .knobBorder      = juce::Colour(0xff4A4A49),
    .knobShadow      = juce::Colour(0x30000000),
    .inactiveArc     = juce::Colour(0xff4A4643),
    .pointer         = juce::Colour(0xffF06A3D),
    .knobValueText   = juce::Colour(0xffEEE8DF),
    .knobLabelText   = juce::Colour(0xffA49D94),

    // == Shadows ==
    .shadowLight     = juce::Colour(0x30000000),
    .shadowMedium    = juce::Colour(0x40FFFFFF),
};
