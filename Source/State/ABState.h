#pragma once

#include <JuceHeader.h>

namespace VoxlineState
{
enum class AbSlot
{
    a,
    b
};

class AbStateManager
{
public:
    explicit AbStateManager(juce::AudioProcessorValueTreeState&);

    void initialiseFromCurrentSound();
    void captureActiveSlot();
    void select(AbSlot);
    AbSlot activeSlot() const noexcept;
    juce::ValueTree toValueTree() const;
    bool canRestore(const juce::ValueTree&) const;
    void restore(const juce::ValueTree&);

private:
    juce::ValueTree snapshot(const juce::Identifier&) const;
    bool isValidSnapshot(const juce::ValueTree&) const;
    void apply(const juce::ValueTree&);

    juce::AudioProcessorValueTreeState& parameters;
    juce::ValueTree slotA;
    juce::ValueTree slotB;
    AbSlot active {AbSlot::a};
};
} // namespace VoxlineState
