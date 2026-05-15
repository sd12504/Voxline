#include "CustomKnob.h"

namespace
{
constexpr float rotaryStart = juce::MathConstants<float>::pi * 1.2f;
constexpr float rotaryEnd = juce::MathConstants<float>::pi * 2.8f;
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

void VoxlineCustomKnob::paint(juce::Graphics& g)
{
    const auto& t = (theme != nullptr) ? *theme : VoxlineTheme::light;

    const auto bounds = getLocalBounds().toFloat();
    const auto knobBounds = getKnobBounds();
    const auto radius = knobBounds.getWidth() * 0.5f;
    const auto centre = knobBounds.getCentre();
    const auto normalized = getNormalizedValue();
    const auto angle = rotaryStart + (rotaryEnd - rotaryStart) * normalized;

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
    juce::Path inactiveArc;
    inactiveArc.addCentredArc(centre.x, centre.y, radius - 7.0f, radius - 7.0f, 0.0f, rotaryStart, rotaryEnd, true);
    g.setColour(t.inactiveArc);
    g.strokePath(inactiveArc, juce::PathStrokeType(isHero ? 8.0f : 6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Active arc
    juce::Path activeArc;
    activeArc.addCentredArc(centre.x, centre.y, radius - 7.0f, radius - 7.0f, 0.0f, rotaryStart, angle, true);
    g.setColour(accent);
    g.strokePath(activeArc, juce::PathStrokeType(isHero ? 8.0f : 6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Pointer
    juce::Path pointer;
    const auto pointerLength = radius * (isHero ? 0.62f : 0.56f);
    const auto pointerThickness = isHero ? 4.0f : 3.0f;
    pointer.addRoundedRectangle(-pointerThickness * 0.5f, -pointerLength, pointerThickness, pointerLength, pointerThickness * 0.5f);
    pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
    g.setColour(t.pointer);
    g.fillPath(pointer);

    // Value text
    g.setColour(t.knobValueText);
    g.setFont(juce::FontOptions(isHero ? 28.0f : 12.5f, juce::Font::bold));
    g.drawText(getTextFromValue(getValue()),
               bounds.withTop(knobBounds.getBottom() + (isHero ? 8.0f : 6.0f)).withHeight(isHero ? 34.0f : 18.0f),
               juce::Justification::centred,
               false);

    // Label text
    g.setColour(t.knobLabelText);
    g.setFont(juce::FontOptions(isHero ? 13.0f : 11.5f, juce::Font::bold));
    g.drawText(label.toUpperCase(),
               bounds.withBottom(bounds.getBottom() - (isHero ? 2.0f : 0.0f)).withHeight(isHero ? 20.0f : 16.0f),
               juce::Justification::centred,
               false);
}

juce::Rectangle<float> VoxlineCustomKnob::getKnobBounds() const
{
    auto bounds = getLocalBounds().toFloat();
    const auto footerHeight = isHero ? 58.0f : 34.0f;
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
