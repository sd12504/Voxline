#pragma once

#include <JuceHeader.h>

#include <span>

namespace Voxline
{
enum class ParameterRole
{
    sound,
    utility,
    retired
};

struct ParameterSpec
{
    const char* id;
    ParameterRole role;
    bool saveInPreset;
    bool saveInAb;
};

std::span<const ParameterSpec> parameterRegistry() noexcept;
const ParameterSpec* findParameterSpec(juce::StringRef id) noexcept;
juce::ValueTree copyRegisteredSoundState(const juce::ValueTree& source);
} // namespace Voxline
