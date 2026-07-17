#pragma once

#include <JuceHeader.h>

#include <optional>

namespace VoxlineState
{
inline constexpr int currentSchemaVersion = 3;

std::optional<juce::ValueTree> migrateToCurrent(
    const juce::ValueTree& source);
juce::ValueTree migrateV1ToV3(juce::ValueTree state);
juce::ValueTree migrateV2ToV3(juce::ValueTree state);
} // namespace VoxlineState
