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

//==============================================================================
void VoxlineImageButton::paintButton(juce::Graphics& g,
                                     bool /*shouldDrawButtonAsHighlighted*/,
                                     bool /*shouldDrawButtonAsDown*/)
{
    const auto& img = getToggleState() ? activeImage : normalImage;
    if (! img.isValid())
        return;

    if (scaleToFit)
    {
        // Scale proportionally to fit button bounds
        g.drawImageWithin(img, 0, 0, getWidth(), getHeight(),
                          juce::RectanglePlacement::centred);
    }
    else
    {
        // Center the image at its original pixel size
        const auto bx = (getWidth()  - img.getWidth())  / 2;
        const auto by = (getHeight() - img.getHeight()) / 2;
        g.drawImageAt(img, bx, by);
    }
}

//==============================================================================
void VoxlineImageButton::updateCurrentImages()
{
    if (isThemed)
    {
        normalImage = (themeIndex == 0) ? nDark : nLight;
        activeImage = (themeIndex == 0) ? aDark : aLight;
    }
    repaint();
}
