#include "VoxlineMeter.h"

VoxlineLevelMeter::VoxlineLevelMeter()
{
    setRepaintsOnMouseActivity(false);
    setOpaque(false);         // panel background shows through empty area
}

void VoxlineLevelMeter::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto fg = findColour(foregroundColour);
    const auto bg = findColour(backgroundColour);

    // 1. Well background (subtle recessed fill)
    g.setColour(bg);
    g.fillRoundedRectangle(bounds, cornerSize);

    // 2. Scale marks
    g.setColour(bg.brighter(0.08f));
    for (int i = 1; i < 6; ++i)
    {
        const auto y = juce::roundToInt(bounds.getBottom() - bounds.getHeight() * (float)i / 6.0f);
        g.drawHorizontalLine(y, bounds.getX() + 4.0f, bounds.getRight() - 4.0f);
    }

    // 3. Meter fill — drawn from bottom up
    const float display = juce::jmax(smoothed, minimumLevel);
    const auto fillArea = bounds.reduced(3.0f);
    const auto fillH = fillArea.getHeight() * juce::jlimit(0.0f, 1.0f, display);
    if (fillH > 0.5f)
    {
        const auto fillR = fillArea.withTop(fillArea.getBottom() - fillH);
        g.setColour(fg);
        g.fillRoundedRectangle(fillR, cornerSize);
    }

    // 4. Peak hold line
    const float peak = juce::jmax(peakHold, minimumLevel);
    if (peak > 0.005f)
    {
        const auto peakY = fillArea.getBottom() - fillArea.getHeight() * peak;
        g.setColour(fg.brighter(0.35f));
        g.drawLine(fillArea.getX(), peakY, fillArea.getRight(), peakY, 1.5f);
    }

    // 5. Border on top of everything
    g.setColour(bg.darker(0.3f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 1.0f);
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

void VoxlineLevelMeter::setMinimumLevel(float v)
{
    minimumLevel = juce::jlimit(0.0f, 1.0f, v);
    repaint();
}
