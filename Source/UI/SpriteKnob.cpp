#include "SpriteKnob.h"

namespace
{
constexpr int numFrames = 64;
constexpr float rotaryStart = juce::MathConstants<float>::pi * 1.2f;
constexpr float rotaryEnd = juce::MathConstants<float>::pi * 2.8f;

juce::Image imageFromMemory(const char* data, int size)
{
    return juce::ImageCache::getFromMemory(data, size);
}

juce::Image removeBlackMatte(juce::Image image)
{
    if (! image.isValid())
        return image;

    const auto corner = image.getPixelAt(0, 0);
    if (corner.getAlpha() < 250
        || corner.getRed() > 45
        || corner.getGreen() > 45
        || corner.getBlue() > 45)
    {
        return image;
    }

    auto keyed = image.createCopy();
    juce::Image::BitmapData pixels(keyed, juce::Image::BitmapData::readWrite);

    for (int y = 0; y < keyed.getHeight(); ++y)
    {
        for (int x = 0; x < keyed.getWidth(); ++x)
        {
            const auto c = pixels.getPixelColour(x, y);
            if (c.getRed() <= 45 && c.getGreen() <= 45 && c.getBlue() <= 45)
                pixels.setPixelColour(x, y, c.withAlpha(0.0f));
        }
    }

    return keyed;
}
} // namespace

VoxlineSpriteKnob::VoxlineSpriteKnob(juce::String labelText, SpriteSize size)
    : label(std::move(labelText)), spriteSize(size)
{
    setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    setRotaryParameters(rotaryStart, rotaryEnd, true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void VoxlineSpriteKnob::setTheme(const VoxlineTheme& newTheme)
{
    theme = &newTheme;
    repaint();
}

void VoxlineSpriteKnob::setSpriteSize(SpriteSize size)
{
    spriteSize = size;
    repaint();
}

void VoxlineSpriteKnob::setShowInternalLabel(bool show)
{
    showInternalLabel = show;
    repaint();
}

void VoxlineSpriteKnob::setShowInternalValue(bool show)
{
    showInternalValue = show;
    repaint();
}

void VoxlineSpriteKnob::paint(juce::Graphics& g)
{
    const auto& t = (theme != nullptr) ? *theme : VoxlineTheme::dark;
    const auto knobBounds = getKnobBounds();
    const auto sizeClass = resolveSpriteSize();
    const auto isLarge = sizeClass == SpriteSize::large;
    const auto normal = getNormalizedValue();
    const auto angle = rotaryStart + normal * (rotaryEnd - rotaryStart);
    const auto radius = knobBounds.getWidth() * 0.5f;
    const auto centre = knobBounds.getCentre();
    const auto accent = juce::Colour(0xffF06A3D);

    if (isLarge)
    {
        const auto bezel = knobBounds.reduced(radius * 0.09f);
        juce::Path shadowShape;
        shadowShape.addEllipse(bezel);
        juce::DropShadow(juce::Colours::black.withAlpha(0.70f),
                         juce::roundToInt(radius * 0.17f), {0, 7}).drawForPath(g, shadowShape);

        juce::ColourGradient bezelGradient(juce::Colour(0xff484847), centre.x, bezel.getY(),
                                           juce::Colour(0xff111212), centre.x, bezel.getBottom(), false);
        g.setGradientFill(bezelGradient);
        g.fillEllipse(bezel);
        g.setColour(juce::Colour(0xff85817A).withAlpha(0.38f));
        g.drawEllipse(bezel, 1.35f);

        const auto face = bezel.reduced(radius * 0.18f);
        g.setColour(juce::Colours::black.withAlpha(0.62f));
        g.fillEllipse(face.expanded(3.0f));
        juce::ColourGradient faceGradient(juce::Colour(0xff343535), centre.x - radius * 0.35f, face.getY(),
                                          juce::Colour(0xff1B1C1D), centre.x + radius * 0.25f, face.getBottom(), false);
        g.setGradientFill(faceGradient);
        g.fillEllipse(face);
        g.setColour(juce::Colour(0xff77736D).withAlpha(0.32f));
        g.drawEllipse(face, 1.1f);

        const auto pointerLength = radius * 0.62f;
        const auto pointerStart = radius * 0.34f;
        juce::Path pointer;
        pointer.startNewSubPath(centre.x + std::sin(angle) * pointerStart,
                                centre.y - std::cos(angle) * pointerStart);
        pointer.lineTo(centre.x + std::sin(angle) * pointerLength,
                       centre.y - std::cos(angle) * pointerLength);
        g.setColour(accent.withAlpha(isEnabled() ? 1.0f : 0.35f));
        g.strokePath(pointer, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }
    else
    {
        // The reference's small controls have a compact face and a single
        // open scale arc. Avoid full concentric rings, which made the previous
        // implementation look oversized and mechanical.
        const auto face = knobBounds.reduced(radius * 0.29f);
        juce::Path shadowShape;
        shadowShape.addEllipse(face);
        juce::DropShadow(juce::Colours::black.withAlpha(0.68f),
                         juce::roundToInt(radius * 0.13f), {0, 4}).drawForPath(g, shadowShape);

        juce::ColourGradient faceGradient(juce::Colour(0xff363737), centre.x - radius * 0.22f, face.getY(),
                                          juce::Colour(0xff1B1C1D), centre.x + radius * 0.16f, face.getBottom(), false);
        g.setGradientFill(faceGradient);
        g.fillEllipse(face);
        g.setColour(juce::Colour(0xff8A857D).withAlpha(0.32f));
        g.drawEllipse(face, 0.9f);

        const auto arcBounds = knobBounds.reduced(radius * 0.13f);
        const auto lineWidth = juce::jmax(1.1f, radius * 0.024f);
        juce::Path inactive;
        inactive.addCentredArc(centre.x, centre.y, arcBounds.getWidth() * 0.5f,
                               arcBounds.getHeight() * 0.5f, 0.0f,
                               rotaryStart, rotaryEnd, true);
        g.setColour(juce::Colour(0xffC3BAB0).withAlpha(0.54f));
        g.strokePath(inactive, juce::PathStrokeType(lineWidth,
                                                    juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

        const auto range = getRange();
        const auto isBipolar = range.getStart() < 0.0 && range.getEnd() > 0.0;
        const auto zeroNormal = isBipolar
            ? static_cast<float>((0.0 - range.getStart()) / range.getLength()) : 0.0f;
        const auto zeroAngle = rotaryStart + zeroNormal * (rotaryEnd - rotaryStart);
        if (std::abs(angle - zeroAngle) > 0.008f)
        {
            juce::Path active;
            active.addCentredArc(centre.x, centre.y, arcBounds.getWidth() * 0.5f,
                                 arcBounds.getHeight() * 0.5f, 0.0f,
                                 juce::jmin(zeroAngle, angle), juce::jmax(zeroAngle, angle), true);
            g.setColour(accent.withAlpha(isEnabled() ? 0.88f : 0.25f));
            g.strokePath(active, juce::PathStrokeType(lineWidth,
                                                      juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
        }

        const auto pointerLength = radius * 0.42f;
        const auto pointerStart = radius * 0.18f;
        juce::Path pointer;
        pointer.startNewSubPath(centre.x + std::sin(angle) * pointerStart,
                                centre.y - std::cos(angle) * pointerStart);
        pointer.lineTo(centre.x + std::sin(angle) * pointerLength,
                       centre.y - std::cos(angle) * pointerLength);
        g.setColour(accent.withAlpha(isEnabled() ? 1.0f : 0.35f));
        g.strokePath(pointer, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }

    if (showInternalValue)
    {
        const auto fontSize = knobBounds.getWidth() > 100.0f ? 28.0f : 12.5f;
        const auto valueHeight = knobBounds.getWidth() > 100.0f ? 34.0f : 18.0f;
        const auto valueY = knobBounds.getBottom() + (knobBounds.getWidth() > 100.0f ? 8.0f : 4.0f);
        g.setColour(t.knobValueText);
        g.setFont(juce::FontOptions(fontSize, juce::Font::bold));
        g.drawText(getTextFromValue(getValue()),
                   juce::Rectangle<float>(knobBounds.getCentreX() - 55.0f, valueY, 110.0f, valueHeight),
                   juce::Justification::centred, false);
    }

    if (showInternalLabel && label.isNotEmpty())
    {
        const auto labelHeight = knobBounds.getWidth() > 100.0f ? 18.0f : 14.0f;
        const auto labelY = showInternalValue
            ? knobBounds.getBottom() + (knobBounds.getWidth() > 100.0f ? 46.0f : 26.0f)
            : getLocalBounds().toFloat().getBottom() - labelHeight - 2.0f;
        g.setColour(t.knobLabelText);
        g.setFont(juce::FontOptions(knobBounds.getWidth() > 100.0f ? 13.0f : 11.0f, juce::Font::bold));
        g.drawText(label.toUpperCase(),
                   juce::Rectangle<float>(knobBounds.getCentreX() - 55.0f, labelY, 110.0f, labelHeight),
                   juce::Justification::centred, false);
    }
}

juce::Rectangle<float> VoxlineSpriteKnob::getKnobBounds() const
{
    auto bounds = getLocalBounds().toFloat();
    const auto noInternalText = ! showInternalLabel && ! showInternalValue;
    const auto resolved = resolveSpriteSize();
    const auto footerHeight = noInternalText ? 0.0f : (resolved == SpriteSize::large ? 52.0f : 30.0f);
    bounds.removeFromBottom(footerHeight);

    const auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    return juce::Rectangle<float>(size, size).withCentre(bounds.getCentre());
}

float VoxlineSpriteKnob::getNormalizedValue() const
{
    const auto range = getRange();
    const auto denominator = range.getLength();
    if (denominator <= 0.0)
        return 0.0f;

    return juce::jlimit(0.0f, 1.0f, static_cast<float>((getValue() - range.getStart()) / denominator));
}

VoxlineSpriteKnob::SpriteSize VoxlineSpriteKnob::resolveSpriteSize() const
{
    if (spriteSize != SpriteSize::autoSize)
        return spriteSize;

    const auto side = static_cast<float>(juce::jmin(getWidth(), getHeight()));
    if (side >= 110.0f)
        return SpriteSize::large;
    if (side >= 58.0f)
        return SpriteSize::medium;
    return SpriteSize::small;
}

VoxlineSpriteKnob::SpriteImages VoxlineSpriteKnob::getImages(SpriteSize size, bool dark) const
{
    switch (size)
    {
    case SpriteSize::large:
        return {
            removeBlackMatte(imageFromMemory(dark ? BinaryData::knob_face_large_dark2x_png : BinaryData::knob_face_large_light2x_png,
                                             dark ? BinaryData::knob_face_large_dark2x_pngSize : BinaryData::knob_face_large_light2x_pngSize)),
            imageFromMemory(dark ? BinaryData::knob_face_large_inside_dark2x_png : BinaryData::knob_face_large_inside_light2x_png,
                            dark ? BinaryData::knob_face_large_inside_dark2x_pngSize : BinaryData::knob_face_large_inside_light2x_pngSize)
        };
    case SpriteSize::medium:
        return {
            removeBlackMatte(imageFromMemory(dark ? BinaryData::knob_face_medium_dark2x_png : BinaryData::knob_face_medium_light2x_png,
                                             dark ? BinaryData::knob_face_medium_dark2x_pngSize : BinaryData::knob_face_medium_light2x_pngSize)),
            imageFromMemory(dark ? BinaryData::knob_face_medium_inside_dark2x_png : BinaryData::knob_face_medium_inside_light2x_png,
                            dark ? BinaryData::knob_face_medium_inside_dark2x_pngSize : BinaryData::knob_face_medium_inside_light2x_pngSize)
        };
    case SpriteSize::small:
    case SpriteSize::autoSize:
        return {
            imageFromMemory(dark ? BinaryData::knob_face_small_dark2x_png : BinaryData::knob_face_small_light2x_png,
                            dark ? BinaryData::knob_face_small_dark2x_pngSize : BinaryData::knob_face_small_light2x_pngSize),
            imageFromMemory(dark ? BinaryData::knob_face_small_inside_dark2x_png : BinaryData::knob_face_small_inside_light2x_png,
                            dark ? BinaryData::knob_face_small_inside_dark2x_pngSize : BinaryData::knob_face_small_inside_light2x_pngSize)
        };
    }

    return {};
}

void VoxlineSpriteKnob::drawImageFitted(juce::Graphics& g, const juce::Image& image, juce::Rectangle<float> bounds) const
{
    const auto imageAspect = static_cast<float>(image.getWidth()) / static_cast<float>(image.getHeight());
    auto target = bounds;

    if (imageAspect > 1.0f)
        target = target.withHeight(target.getWidth() / imageAspect).withCentre(bounds.getCentre());
    else
        target = target.withWidth(target.getHeight() * imageAspect).withCentre(bounds.getCentre());

    g.drawImage(image, target);
}
