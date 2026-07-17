#include "ImageButton.h"

//==============================================================================
VoxlineImageButton::VoxlineImageButton(const juce::String& name)
    : juce::Button(name)
{
    setClickingTogglesState(true);
}

//==============================================================================
void VoxlineImageButton::setThemeImages(const juce::Image& normalDark_,
                                        const juce::Image& normalLight_,
                                        const juce::Image& activeDark_,
                                        const juce::Image& activeLight_)
{
    nDark    = normalDark_;
    nLight   = normalLight_;
    aDark    = activeDark_;
    aLight   = activeLight_;
    isThemed = true;
    updateCurrentImages();
}

void VoxlineImageButton::setThemeIndex(int index)
{
    themeIndex = index;
    if (isThemed)
        updateCurrentImages();
}

void VoxlineImageButton::resizeToImage()
{
    const auto& img = normalImage.isValid() ? normalImage : activeImage;
    if (img.isValid())
        setSize(img.getWidth(), img.getHeight());
}

void VoxlineImageButton::setHitTestInsets(int left, int top, int right, int bottom)
{
    hitTestInsets = juce::BorderSize<int>(top, left, bottom, right);
}

//==============================================================================
void VoxlineImageButton::paintButton(juce::Graphics& g,
                                     bool shouldDrawButtonAsHighlighted,
                                     bool shouldDrawButtonAsDown)
{
    const auto name = getName();
    const auto isEqBand = name == "HPF" || name == "LOW" || name == "MUD"
                       || name == "PRES" || name == "AIR" || name == "LPF";
    const auto active = getToggleState();
    const auto bounds = getLocalBounds().toFloat().reduced(0.5f);
    const auto accent = juce::Colour(0xffF06A3D);
    const auto compact = getHeight() <= 30;
    const auto radius = compact ? 5.0f : 6.0f;

    auto fill = active ? juce::Colour(0xff241713) : juce::Colour(0xff171818);
    auto border = active ? accent : juce::Colour(0xff55534F);
    if (shouldDrawButtonAsHighlighted)
    {
        fill = active ? juce::Colour(0xff2C1A14) : juce::Colour(0xff202121);
        border = active ? accent.brighter(0.08f) : juce::Colour(0xff77736D);
    }
    if (shouldDrawButtonAsDown)
        fill = active ? juce::Colour(0xff331B14) : juce::Colour(0xff101111);

    g.setColour(juce::Colours::black.withAlpha(0.38f));
    g.fillRoundedRectangle(bounds.translated(0.0f, 1.5f), radius);
    g.setColour(fill);
    g.fillRoundedRectangle(bounds, radius);
    g.setColour(border);
    g.drawRoundedRectangle(bounds, radius, active ? 1.15f : 0.85f);

    auto text = name;
    if (name == "Auto Gain" || name == "EQ On")
        text = active ? "ON" : "OFF";

    g.setColour(active ? juce::Colour(0xffF47A50)
                       : (shouldDrawButtonAsHighlighted ? juce::Colour(0xffEEE8DF)
                                                        : juce::Colour(0xffC6BFB5)));
    g.setFont(juce::Font(juce::FontOptions(isEqBand ? 9.5f : 10.5f, juce::Font::bold))
                  .withExtraKerningFactor(0.08f));
    g.drawText(text.toUpperCase(), getLocalBounds().reduced(5),
               juce::Justification::centred, false);
}

bool VoxlineImageButton::hitTest(int x, int y)
{
    auto hitBounds = getLocalBounds();
    hitTestInsets.subtractFrom(hitBounds);
    return hitBounds.contains(x, y);
}

//==============================================================================
void VoxlineImageButton::updateCurrentImages()
{
    if (isThemed)
    {
        normalImage = (themeIndex == 0) ? nLight : nDark;
        activeImage = (themeIndex == 0) ? aLight : aDark;
    }
    repaint();
}
