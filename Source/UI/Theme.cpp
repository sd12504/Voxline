#include "Theme.h"

const VoxlineTheme VoxlineTheme::light =
{
    // == Editor & Window ==
    .editorBg        = juce::Colour(0xffd9ddd6),
    .mainCardBg      = juce::Colour(0xfff3f1eb),
    .panelBg         = juce::Colour(0xffece5de),
    .panelBorder     = juce::Colour(0xffddd2c4),

    // == Text ==
    .textPrimary     = juce::Colour(0xff26222B),
    .textSecondary   = juce::Colour(0xff746D7C),
    .textMuted       = juce::Colour(0xffA198A8),

    // == Accents ==
    .accentRose      = juce::Colour(0xffD86F96),
    .accentPeach     = juce::Colour(0xffE99A5C),
    .accentAmber     = juce::Colour(0xffD8A548),
    .accentPurple    = juce::Colour(0xff8D70E8),
    .accentLavender  = juce::Colour(0xffB8A6F3),

    // == Meters ==
    .meterHigh       = juce::Colour(0xffE99A5C),
    .meterMid        = juce::Colour(0xffD86F96),
    .meterLow        = juce::Colour(0xff8D70E8),

    // == Knob ==
    .knobBodyTop     = juce::Colour(0xfffbf8f2),
    .knobBodyBottom  = juce::Colour(0xffddd3c8),
    .knobBorder      = juce::Colour(0x1f000000),
    .knobShadow      = juce::Colour(0x16000000),
    .inactiveArc     = juce::Colour(0x22000000),
    .pointer         = juce::Colour(0xff403731),
    .knobValueText   = juce::Colour(0xff2f2925),
    .knobLabelText   = juce::Colour(0xff6c5f56),

    // == Shadows ==
    .shadowLight     = juce::Colour(0x16000000),
    .shadowMedium    = juce::Colour(0x22000000),
};

const VoxlineTheme VoxlineTheme::dark =
{
    // == Editor & Window ==
    .editorBg        = juce::Colour(0xff09080D),
    .mainCardBg      = juce::Colour(0xff111018),
    .panelBg         = juce::Colour(0xff181622),
    .panelBorder     = juce::Colour(0xff2A2635),

    // == Text ==
    .textPrimary     = juce::Colour(0xffF5F0EA),
    .textSecondary   = juce::Colour(0xff9D96A8),
    .textMuted       = juce::Colour(0xff6E6878),

    // == Accents ==
    .accentRose      = juce::Colour(0xffE98BAA),
    .accentPeach     = juce::Colour(0xffF2A766),
    .accentAmber     = juce::Colour(0xffE6B45C),
    .accentPurple    = juce::Colour(0xffA98CFF),
    .accentLavender  = juce::Colour(0xffC7B7FF),

    // == Meters ==
    .meterHigh       = juce::Colour(0xffF2A766),
    .meterMid        = juce::Colour(0xffE98BAA),
    .meterLow        = juce::Colour(0xffA98CFF),

    // == Knob ==
    .knobBodyTop     = juce::Colour(0xff2A2635),
    .knobBodyBottom  = juce::Colour(0xff181622),
    .knobBorder      = juce::Colour(0xff3A3645),
    .knobShadow      = juce::Colour(0x30000000),
    .inactiveArc     = juce::Colour(0x40FFFFFF),
    .pointer         = juce::Colour(0xffF5F0EA),
    .knobValueText   = juce::Colour(0xffF5F0EA),
    .knobLabelText   = juce::Colour(0xff9D96A8),

    // == Shadows ==
    .shadowLight     = juce::Colour(0x30000000),
    .shadowMedium    = juce::Colour(0x40FFFFFF),
};
