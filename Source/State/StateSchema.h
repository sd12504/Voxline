#pragma once

#include <JuceHeader.h>
#include "StateMigration.h"

#include <optional>

namespace VoxlineState
{
void serialise(const juce::ValueTree& state, juce::MemoryBlock& destination);
std::optional<juce::ValueTree> deserialise(const void* data,
                                           int sizeInBytes,
                                           juce::Identifier expectedRoot);
}
