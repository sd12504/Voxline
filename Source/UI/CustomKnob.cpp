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

    // ================================================================
    //  HERO KNOB (unchanged)
    // ================================================================
    if (isHero)
    {
        // Hero glow (dark theme only)
        if (isDark)
        {
            juce::ColourGradient glow(accent.withAlpha(0.18f), centre.x, centre.y,
                                       accent.withAlpha(0.0f), centre.x, centre.y + radius * 1.6f, true);
            g.setGradientFill(glow);
            g.fillEllipse(knobBounds.expanded(radius * 0.45f));
        }

        // Knob drop shadow
        g.setColour(t.knobShadow);
        g.fillEllipse(knobBounds.translated(0.0f, 6.0f));

        // Knob body gradient
        juce::ColourGradient bodyGradient(t.knobBodyTop, centre.x, knobBounds.getY(),
                                          t.knobBodyBottom, centre.x, knobBounds.getBottom(), false);
        g.setGradientFill(bodyGradient);
        g.fillEllipse(knobBounds);

        // Knob border
        g.setColour(t.knobBorder);
        g.drawEllipse(knobBounds, 2.4f);

        // Inactive arc
        const auto arcR = radius - arcInset;
        juce::Path inactiveArc;
        inactiveArc.addCentredArc(centre.x, centre.y, arcR, arcR, 0.0f, rotaryStart, rotaryEnd, true);
        g.setColour(t.inactiveArc);
        g.strokePath(inactiveArc, juce::PathStrokeType(7.0f,
                                                        juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));

        // Active arc
        juce::Path activeArc;
        activeArc.addCentredArc(centre.x, centre.y, arcR, arcR, 0.0f, rotaryStart, angle, true);
        g.setColour(accent);
        g.strokePath(activeArc, juce::PathStrokeType(7.0f,
                                                      juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));

        // Pointer
        juce::Path pointer;
        const float pLen = radius * 0.58f;
        const float pThick = 3.5f;
        pointer.addRoundedRectangle(-pThick * 0.5f, -pLen,
                                     pThick, pLen, pThick * 0.5f);
        pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
        g.setColour(t.pointer);
        g.fillPath(pointer);

        // Value text
        if (showInternalValue)
        {
            const float vY = knobBounds.getBottom() + 8.0f;
            g.setColour(t.knobValueText);
            g.setFont(juce::FontOptions(28.0f, juce::Font::bold));
            g.drawText(getTextFromValue(getValue()),
                       juce::Rectangle<float>(centre.x - 50.0f, vY, 100.0f, 34.0f),
                       juce::Justification::centred, false);
        }

        // Label text
        if (showInternalLabel && label.isNotEmpty())
        {
            const bool hasValue = showInternalValue;
            const float lY = hasValue
                ? knobBounds.getBottom() + 8.0f + 34.0f + 4.0f
                : bounds.getBottom() - 18.0f - 2.0f;
            g.setColour(t.knobLabelText);
            g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
            g.drawText(label.toUpperCase(),
                       juce::Rectangle<float>(centre.x - 50.0f, lY, 100.0f, 18.0f),
                       juce::Justification::centred, false);
        }
    }
    // ================================================================
    //  SMALL KNOB (boutique redesign)
    // ================================================================
    else
    {
        // --- Compute proportions ---
        // Design reference: 64x64 component → knob face Ø42, arc radius 29
        const float compSize = juce::jmin(bounds.getWidth(), bounds.getHeight());
        const float scale = compSize / 64.0f;

        const float knobR   = 21.0f * scale;            // face radius
        const float arcStroke = juce::jmin(4.5f, compSize * 0.07f);
        const float arcR    = juce::jmin(29.0f * scale,
                                          compSize * 0.5f - arcStroke * 0.5f - 2.0f);
        const float pointerLen  = knobR * 0.62f;        // ~13px at 1x
        const float pointerThick = 2.0f * scale;
        const float rimThick    = juce::jmin(1.5f, scale * 1.5f);

        const bool active = isMouseOverOrDragging();

        // 1. Background arc  —  #D8D1C8 @ 0.65 alpha
        {
            juce::Path p;
            p.addCentredArc(centre.x, centre.y, arcR, arcR, 0.0f,
                            rotaryStart, rotaryEnd, true);
            g.setColour(juce::Colour::fromRGB(216, 209, 200).withAlpha(0.65f));
            g.strokePath(p, juce::PathStrokeType(arcStroke,
                                                  juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
        }

        // 2. Value arc  —  accent color, full start→angle
        if (normalized > 0.0f)
        {
            // very subtle glow when active
            if (active)
            {
                juce::Path glow;
                glow.addCentredArc(centre.x, centre.y, arcR + 1.5f, arcR + 1.5f, 0.0f,
                                   rotaryStart, angle, true);
                g.setColour(accent.withAlpha(0.12f));
                g.strokePath(glow, juce::PathStrokeType(arcStroke + 4.0f,
                                                         juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));
            }

            juce::Path p;
            p.addCentredArc(centre.x, centre.y, arcR, arcR, 0.0f,
                            rotaryStart, angle, true);
            g.setColour(accent);
            g.strokePath(p, juce::PathStrokeType(arcStroke,
                                                  juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
        }

        // 3. Drop shadow  —  black @ 0.16, offset y 2px, blur-simulated with 2 stacked ellipses
        {
            g.setColour(juce::Colours::black.withAlpha(0.16f));
            // outer blur layer
            g.fillEllipse(centre.x - knobR + 1.0f, centre.y - knobR + 3.0f,
                          knobR * 2.0f, knobR * 2.0f);
            // tighter core shadow
            g.setColour(juce::Colours::black.withAlpha(0.08f));
            g.fillEllipse(centre.x - knobR * 0.6f, centre.y - knobR * 0.6f + 3.0f,
                          knobR * 1.2f, knobR * 1.2f);
        }

        // 4. Knob face  —  warm cream gradient #FBF5EA → #DDD5CA
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

        // 5. Outer rim  —  1px #CFC7BC
        {
            g.setColour(juce::Colour::fromRGB(207, 199, 188));
            g.drawEllipse(centre.x - knobR, centre.y - knobR,
                          knobR * 2.0f, knobR * 2.0f,
                          rimThick);
        }

        // 6. Inner highlight  —  white @ 0.35 alpha, 1px
        {
            const float inset = 3.0f * scale;
            g.setColour(juce::Colours::white.withAlpha(0.35f));
            g.drawEllipse(centre.x - knobR + inset, centre.y - knobR + inset,
                          knobR * 2.0f - inset * 2.0f,
                          knobR * 2.0f - inset * 2.0f,
                          1.0f);
        }

        // 7. Pointer  —  #3A3036, from centre to value angle
        {
            const float px = centre.x + std::cos(angle - juce::MathConstants<float>::halfPi) * pointerLen;
            const float py = centre.y + std::sin(angle - juce::MathConstants<float>::halfPi) * pointerLen;
            g.setColour(juce::Colour::fromRGB(58, 48, 54));
            g.drawLine(centre.x, centre.y, px, py, pointerThick);
        }

        // --- Text (small knob usually has it drawn externally, but support if enabled) ---
        const auto cx = centre.x;

        if (showInternalValue)
        {
            const float vY = knobBounds.getBottom() + 4.0f;
            g.setColour(t.knobValueText);
            g.setFont(juce::FontOptions(12.5f, juce::Font::bold));
            g.drawText(getTextFromValue(getValue()),
                       juce::Rectangle<float>(cx - 50.0f, vY, 100.0f, 18.0f),
                       juce::Justification::centred, false);
        }

        if (showInternalLabel && label.isNotEmpty())
        {
            const bool hasValue = showInternalValue;
            const float lY = hasValue
                ? knobBounds.getBottom() + 4.0f + 18.0f + 4.0f
                : bounds.getBottom() - 14.0f - 2.0f;
            g.setColour(t.knobLabelText);
            g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
            g.drawText(label.toUpperCase(),
                       juce::Rectangle<float>(cx - 50.0f, lY, 100.0f, 14.0f),
                       juce::Justification::centred, false);
        }
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
