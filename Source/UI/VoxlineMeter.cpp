#include "VoxlineMeter.h"

VoxlineLevelMeter::VoxlineLevelMeter()
{
    setRepaintsOnMouseActivity(false);
}

void VoxlineLevelMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    const auto corner = cornerSize;

    const auto bg = findColour(backgroundColour);
    const auto fg = findColour(foregroundColour);

    // Well
    g.setColour(bg);
    g.fillRoundedRectangle(bounds, corner);

    // Fill
    const auto fillHeight = bounds.getHeight() * juce::jlimit(0.0f, 1.0f, smoothed);
    const auto fillRect = bounds.withTop(bounds.getBottom() - fillHeight);

    if (fillHeight > 0.0f)
    {
        g.setColour(fg);
        if (fillHeight >= bounds.getHeight() - corner)
        {
            g.fillRoundedRectangle(bounds, corner);
        }
        else
        {
            g.fillRoundedRectangle(fillRect.expanded(0.0f, corner), corner);
            g.setColour(bg);
            g.fillRect(fillRect.getX(), fillRect.getY(), fillRect.getWidth(), corner + 1.0f);
        }
    }

    // Peak hold line
    if (peakHold > 0.001f)
    {
        const auto peakY = bounds.getBottom() - bounds.getHeight() * peakHold;
        g.setColour(fg.brighter(0.25f));
        g.fillRect(bounds.getX(), peakY - 1.0f, bounds.getWidth(), 2.0f);
    }

    // Border
    g.setColour(juce::Colour(0x18FFFFFF));
    g.drawRoundedRectangle(bounds.reduced(0.5f), corner, 1.0f);
}

void VoxlineLevelMeter::setLevel(float v)
{
    level = juce::jlimit(0.0f, 1.0f, v);

    if (level > peakHold)
        peakHold = level;
    else
        peakHold += (level - peakHold) * 0.15f;

    smoothed += (level - smoothed) * 0.3f;
    repaint();
}
