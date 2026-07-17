#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/**
    A toggle button that draws a PNG image at its original pixel size.
    Supports two modes:
      - Single-theme:       one normal + one active image
      - Theme-aware (dark/light): four images, switched via setThemeIndex()

    No hover state. No scaling/stretching.
 */
class VoxlineImageButton final : public juce::Button
{
public:
    //==============================================================================
    explicit VoxlineImageButton(const juce::String& name);

    //==============================================================================
    /** Set single-theme images (no theme switching). */
    void setNormalImage(const juce::Image& img)      { normalImage = img;      updateCurrentImages(); }
    void setActiveImage(const juce::Image& img)       { activeImage  = img;     updateCurrentImages(); }

    /** Set theme-aware image pairs. Call setThemeIndex() afterwards. */
    void setThemeImages(const juce::Image& normalDark,
                        const juce::Image& normalLight,
                        const juce::Image& activeDark,
                        const juce::Image& activeLight);

    /** Switch active theme pair (0 = light, 1 = dark). */
    void setThemeIndex(int index);

    /** Resize the button to match image dimensions (call after setting images). */
    void resizeToImage();

    /** When true, scale the PNG proportionally to fit button bounds.
        When false (default), draw at original PNG size, centered. */
    void setScaleToFit(bool shouldScale)   { scaleToFit = shouldScale; }

    /** Restrict hit testing to the visible button body while still painting full PNG padding. */
    void setHitTestInsets(int left, int top, int right, int bottom);

    //==============================================================================
    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted,
                     bool shouldDrawButtonAsDown) override;
    bool hitTest(int x, int y) override;

private:
    //==============================================================================
    void updateCurrentImages();

    juce::Image normalImage;
    juce::Image activeImage;

    // Theme-paired variants
    juce::Image nDark, nLight;
    juce::Image aDark, aLight;
    int themeIndex = 0;
    bool isThemed = false;
    bool scaleToFit = false;
    juce::BorderSize<int> hitTestInsets;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoxlineImageButton)
};
