#include "CustomKnob.h"

namespace
{
constexpr float rotaryStart = juce::MathConstants<float>::pi * 1.2f;
constexpr float rotaryEnd   = juce::MathConstants<float>::pi * 2.8f;
constexpr float arcInset    = 10.0f;
}

VoxlineCustomKnob::VoxlineCustomKnob(juce::String labelText, juce::Colour accentColour, bool isHeroKnob)
    : label(std::move(labelText)), accent(accentColour), isHero(isHeroKnob)
{
    setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    setRotaryParameters(rotaryStart, rotaryEnd, true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void VoxlineCustomKnob::setTheme(const VoxlineTheme& newTheme)
{
    theme = &newTheme;
    repaint();
}

void VoxlineCustomKnob::setShowInternalLabel(bool show)
{
    showInternalLabel = show;
    repaint();
}

void VoxlineCustomKnob::setShowInternalValue(bool show)
{
    showInternalValue = show;
    repaint();
}

void VoxlineCustomKnob::paint(juce::Graphics& g)
{
    const auto& t = (theme != nullptr) ? *theme : VoxlineTheme::light;

    const auto bounds = getLocalBounds().toFloat();
    const auto knobBounds = getKnobBounds();
    const auto radius = knobBounds.getWidth() * 0.5f;
    const auto centre = knobBounds.getCentre();
    const auto normalized = getNormalizedValue();
    const auto angle = rotaryStart + (rotaryEnd - rotaryStart) * normalized;

    const bool isDark = (t.knobBodyTop.getBrightness() < 0.3f);

    // Hero glow (dark theme only)
    if (isHero && isDark)
    {
        juce::ColourGradient glow(accent.withAlpha(0.18f), centre.x, centre.y,
                                   accent.withAlpha(0.0f), centre.x, centre.y + radius * 1.6f, true);
        g.setGradientFill(glow);
        g.fillEllipse(knobBounds.expanded(radius * 0.45f));
    }

    // Knob drop shadow
    g.setColour(t.knobShadow);
    g.fillEllipse(knobBounds.translated(0.0f, isHero ? 6.0f : 4.0f));

    // Knob body gradient
    juce::ColourGradient bodyGradient(t.knobBodyTop, centre.x, knobBounds.getY(),
                                      t.knobBodyBottom, centre.x, knobBounds.getBottom(), false);
    g.setGradientFill(bodyGradient);
    g.fillEllipse(knobBounds);

    // Knob border
    g.setColour(t.knobBorder);
    g.drawEllipse(knobBounds, isHero ? 2.4f : 1.6f);

    // Inactive arc
    const auto arcR = radius - arcInset;
    juce::Path inactiveArc;
    inactiveArc.addCentredArc(centre.x, centre.y, arcR, arcR, 0.0f, rotaryStart, rotaryEnd, true);
    g.setColour(t.inactiveArc);
    g.strokePath(inactiveArc, juce::PathStrokeType(isHero ? 7.0f : 5.0f,
                                                    juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

    // Active arc
    juce::Path activeArc;
    activeArc.addCentredArc(centre.x, centre.y, arcR, arcR, 0.0f, rotaryStart, angle, true);
    g.setColour(accent);
    g.strokePath(activeArc, juce::PathStrokeType(isHero ? 7.0f : 5.0f,
                                                  juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));

    // Pointer
    juce::Path pointer;
    const auto pointerLength = radius * (isHero ? 0.58f : 0.52f);
    const auto pointerThickness = isHero ? 3.5f : 2.5f;
    pointer.addRoundedRectangle(-pointerThickness * 0.5f, -pointerLength,
                                 pointerThickness, pointerLength, pointerThickness * 0.5f);
    pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
    g.setColour(t.pointer);
    g.fillPath(pointer);

    const auto cx = centre.x; // single centre-X for all text alignment

    // Value text
    if (showInternalValue)
    {
        const float vH = isHero ? 34.0f : 18.0f;
        const float vY = knobBounds.getBottom() + (isHero ? 8.0f : 4.0f);
        g.setColour(t.knobValueText);
        g.setFont(juce::FontOptions(isHero ? 28.0f : 12.5f, juce::Font::bold));
        g.drawText(getTextFromValue(getValue()),
                   juce::Rectangle<float>(cx - 50.0f, vY, 100.0f, vH),
                   juce::Justification::centred,
                   false);
    }

    // Label text
    if (showInternalLabel && label.isNotEmpty())
    {
        const float lH = isHero ? 18.0f : 14.0f;
        const bool hasValue = showInternalValue;

        const float lY = hasValue
            ? knobBounds.getBottom() + (isHero ? 8.0f + 34.0f + 4.0f : 4.0f + 18.0f + 4.0f)
            : bounds.getBottom() - lH - 2.0f;

        g.setColour(t.knobLabelText);
        g.setFont(juce::FontOptions(isHero ? 13.0f : 11.0f, juce::Font::bold));
        g.drawText(label.toUpperCase(),
                   juce::Rectangle<float>(cx - 50.0f, lY, 100.0f, lH),
                   juce::Justification::centred,
                   false);
    }
}

juce::Rectangle<float> VoxlineCustomKnob::getKnobBounds() const
{
    auto bounds = getLocalBounds().toFloat();
    const bool noInternalText = (!showInternalLabel && !showInternalValue);
    const auto footerHeight = noInternalText ? 12.0f : (isHero ? 52.0f : 30.0f);
    bounds.removeFromBottom(footerHeight);
    bounds.reduce(isHero ? 6.0f : 4.0f, isHero ? 2.0f : 1.0f);
    const auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    return juce::Rectangle<float>(size, size).withCentre(bounds.getCentre());
}

float VoxlineCustomKnob::getNormalizedValue() const
{
    const auto range = getRange();
    const auto denominator = range.getLength();
    if (denominator <= 0.0)
        return 0.0f;

    return juce::jlimit(0.0f, 1.0f, static_cast<float>((getValue() - range.getStart()) / denominator));
}
