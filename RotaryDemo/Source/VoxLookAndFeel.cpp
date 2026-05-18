#include "VoxLookAndFeel.h"

//==============================================================================
VoxLookAndFeel::VoxLookAndFeel()
{
    setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(166, 108, 255));
    setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour::fromRGB(210, 202, 194));
}

//==============================================================================
void VoxLookAndFeel::drawRotarySlider(
    juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPosProportional,
    float rotaryStartAngle,
    float rotaryEndAngle,
    juce::Slider& slider)
{
    // unused parameters – we use fixed 135°–405° range
    juce::ignoreUnused(rotaryStartAngle, rotaryEndAngle, slider);

    auto bounds = juce::Rectangle<float>(static_cast<float>(x),
                                         static_cast<float>(y),
                                         static_cast<float>(width),
                                         static_cast<float>(height)).reduced(4.0f);
    auto centre = bounds.getCentre();
    const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const float arcRadius = radius - 4.0f;
    const float knobRadius = radius * 0.72f;
    const float startAngle = juce::degreesToRadians(135.0f);
    const float endAngle   = juce::degreesToRadians(405.0f);
    const float angle = startAngle + sliderPosProportional * (endAngle - startAngle);

    // 1. Background arc
    juce::Path bgArc;
    bgArc.addCentredArc(
        centre.x, centre.y,
        arcRadius, arcRadius,
        0.0f,
        startAngle, endAngle,
        true
    );
    g.setColour(juce::Colour::fromRGB(210, 202, 194));
    g.strokePath(bgArc, juce::PathStrokeType(
        5.0f,
        juce::PathStrokeType::curved,
        juce::PathStrokeType::rounded
    ));

    // 2. Value arc
    juce::Path valueArc;
    valueArc.addCentredArc(
        centre.x, centre.y,
        arcRadius, arcRadius,
        0.0f,
        startAngle, angle,
        true
    );
    g.setColour(juce::Colour::fromRGB(166, 108, 255));
    g.strokePath(valueArc, juce::PathStrokeType(
        5.0f,
        juce::PathStrokeType::curved,
        juce::PathStrokeType::rounded
    ));

    // 3. Drop shadow
    g.setColour(juce::Colours::black.withAlpha(0.18f));
    g.fillEllipse(
        centre.x - knobRadius + 2.0f,
        centre.y - knobRadius + 3.0f,
        knobRadius * 2.0f,
        knobRadius * 2.0f
    );

    // 4. Knob face (gradient)
    juce::ColourGradient faceGradient(
        juce::Colour::fromRGB(250, 244, 235),  // top-left
        centre.x - knobRadius,
        centre.y - knobRadius,
        juce::Colour::fromRGB(221, 214, 205),  // bottom-right
        centre.x + knobRadius,
        centre.y + knobRadius,
        false
    );
    g.setGradientFill(faceGradient);
    g.fillEllipse(
        centre.x - knobRadius,
        centre.y - knobRadius,
        knobRadius * 2.0f,
        knobRadius * 2.0f
    );

    // 5. Outer rim
    g.setColour(juce::Colour::fromRGB(205, 198, 190));
    g.drawEllipse(
        centre.x - knobRadius,
        centre.y - knobRadius,
        knobRadius * 2.0f,
        knobRadius * 2.0f,
        1.5f
    );

    // 6. Inner highlight
    g.setColour(juce::Colours::white.withAlpha(0.35f));
    g.drawEllipse(
        centre.x - knobRadius + 4.0f,
        centre.y - knobRadius + 4.0f,
        knobRadius * 2.0f - 8.0f,
        knobRadius * 2.0f - 8.0f,
        1.0f
    );

    // 7. Pointer
    const float pointerLength = knobRadius * 0.62f;
    const float pointerThickness = 2.2f;
    juce::Point<float> pointerEnd(
        centre.x + std::cos(angle - juce::MathConstants<float>::halfPi) * pointerLength,
        centre.y + std::sin(angle - juce::MathConstants<float>::halfPi) * pointerLength
    );
    g.setColour(juce::Colour::fromRGB(58, 48, 54));
    g.drawLine(
        centre.x,
        centre.y,
        pointerEnd.x,
        pointerEnd.y,
        pointerThickness
    );
}
