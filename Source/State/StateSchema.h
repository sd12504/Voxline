#pragma once

#include <JuceHeader.h>
#include <optional>

namespace VoxlineState
{
inline constexpr int currentSchemaVersion = 2;

void serialise(const juce::ValueTree& state, juce::MemoryBlock& destination);
std::optional<juce::ValueTree> deserialise(const void* data,
                                           int sizeInBytes,
                                           juce::Identifier expectedRoot);
}
