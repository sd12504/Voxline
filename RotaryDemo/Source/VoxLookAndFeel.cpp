#include "VoxLookAndFeel.h"

//==============================================================================
VoxLookAndFeel::VoxLookAndFeel()
{
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
    juce::ignoreUnused(rotaryStartAngle, rotaryEndAngle);

    const auto bounds = juce::Rectangle<float>(static_cast<float>(x),
                                               static_cast<float>(y),
                                               static_cast<float>(width),
                                               static_cast<float>(height));
    const auto centre = bounds.getCentre();

    // Boutique small knob — design ref: 64x64 → face Ø42, arc radius 29
    const float compSize = juce::jmin(bounds.getWidth(), bounds.getHeight());
    const float scale = compSize / 64.0f;

    const float knobR   = 21.0f * scale;
    const float arcStroke = juce::jmin(4.5f, compSize * 0.07f);
    const float arcR    = juce::jmin(29.0f * scale,
                                      compSize * 0.5f - arcStroke * 0.5f - 2.0f);
    const float pointerLen  = knobR * 0.62f;
    const float pointerThick = 2.0f * scale;
    const float rimThick    = juce::jmin(1.5f, scale * 1.5f);

    const float startAngle = juce::degreesToRadians(135.0f);
    const float endAngle   = juce::degreesToRadians(405.0f);
    const float angle = startAngle + sliderPosProportional * (endAngle - startAngle);

    const bool active = slider.isMouseOverOrDragging();

    // ====================================================================
    // 1. Background arc  —  #D8D1C8 @ 0.65 alpha
    // ====================================================================
    {
        juce::Path p;
        p.addCentredArc(centre.x, centre.y, arcR, arcR, 0.0f,
                        startAngle, endAngle, true);
        g.setColour(juce::Colour::fromRGB(216, 209, 200).withAlpha(0.65f));
        g.strokePath(p, juce::PathStrokeType(arcStroke,
                                              juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
    }

    // ====================================================================
    // 2. Value arc  —  accent purple
    // ====================================================================
    if (sliderPosProportional > 0.0f)
    {
        // subtle glow when active
        if (active)
        {
            juce::Path glow;
            glow.addCentredArc(centre.x, centre.y, arcR + 1.5f, arcR + 1.5f, 0.0f,
                               startAngle, angle, true);
            g.setColour(juce::Colour::fromRGB(165, 108, 255).withAlpha(0.12f));
            g.strokePath(glow, juce::PathStrokeType(arcStroke + 4.0f,
                                                     juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
        }

        juce::Path p;
        p.addCentredArc(centre.x, centre.y, arcR, arcR, 0.0f,
                        startAngle, angle, true);
        g.setColour(juce::Colour::fromRGB(165, 108, 255));
        g.strokePath(p, juce::PathStrokeType(arcStroke,
                                              juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
    }

    // ====================================================================
    // 3. Drop shadow  —  black @ 0.16, offset y 2px, multi-layer
    // ====================================================================
    {
        g.setColour(juce::Colours::black.withAlpha(0.16f));
        g.fillEllipse(centre.x - knobR + 1.0f, centre.y - knobR + 3.0f,
                      knobR * 2.0f, knobR * 2.0f);
        g.setColour(juce::Colours::black.withAlpha(0.08f));
        g.fillEllipse(centre.x - knobR * 0.6f, centre.y - knobR * 0.6f + 3.0f,
                      knobR * 1.2f, knobR * 1.2f);
    }

    // ====================================================================
    // 4. Knob face  —  warm cream gradient #FBF5EA → #DDD5CA
    // ====================================================================
    {
        juce::ColourGradient faceGrad(
            juce::Colour::fromRGB(251, 245, 234),
            centre.x - knobR, centre.y - knobR,
            juce::Colour::fromRGB(221, 213, 202),
            centre.x + knobR, centre.y + knobR,
            false);
        g.setGradientFill(faceGrad);
        g.fillEllipse(centre.x - knobR, centre.y - knobR,
                      knobR * 2.0f, knobR * 2.0f);
    }

    // ====================================================================
    // 5. Outer rim  —  1px #CFC7BC
    // ====================================================================
    {
        g.setColour(juce::Colour::fromRGB(207, 199, 188));
        g.drawEllipse(centre.x - knobR, centre.y - knobR,
                      knobR * 2.0f, knobR * 2.0f,
                      rimThick);
    }

    // ====================================================================
    // 6. Inner highlight  —  white @ 0.35, 1px
    // ====================================================================
    {
        const float inset = 3.0f * scale;
        g.setColour(juce::Colours::white.withAlpha(0.35f));
        g.drawEllipse(centre.x - knobR + inset, centre.y - knobR + inset,
                      knobR * 2.0f - inset * 2.0f,
                      knobR * 2.0f - inset * 2.0f,
                      1.0f);
    }

    // ====================================================================
    // 7. Pointer  —  #3A3036, from centre to value angle
    // ====================================================================
    {
        const float px = centre.x + std::cos(angle - juce::MathConstants<float>::halfPi) * pointerLen;
        const float py = centre.y + std::sin(angle - juce::MathConstants<float>::halfPi) * pointerLen;
        g.setColour(juce::Colour::fromRGB(58, 48, 54));
        g.drawLine(centre.x, centre.y, px, py, pointerThick);
    }
}
