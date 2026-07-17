#include "ParameterRegistry.h"

#include "ParameterIDs.h"

#include <array>

namespace
{
using Role = Voxline::ParameterRole;
using Spec = Voxline::ParameterSpec;

constexpr Spec sound(const char* id) noexcept
{
    return {id, Role::sound, true, true};
}

constexpr Spec utility(const char* id) noexcept
{
    return {id, Role::utility, false, false};
}

constexpr Spec retired(const char* id) noexcept
{
    return {id, Role::retired, false, false};
}

constexpr std::array registry {
    sound(VoxlineParameterIDs::inputGain),
    retired(VoxlineParameterIDs::autoGain),
    sound(VoxlineParameterIDs::polish),
    sound(VoxlineParameterIDs::body),
    sound(VoxlineParameterIDs::clarity),
    sound(VoxlineParameterIDs::air),
    sound(VoxlineParameterIDs::smooth),
    sound(VoxlineParameterIDs::comp),
    sound(VoxlineParameterIDs::drive),
    sound(VoxlineParameterIDs::outputGain),
    utility(VoxlineParameterIDs::bypass),
    retired(VoxlineParameterIDs::cleanMode),
    retired(VoxlineParameterIDs::listen),
    sound(VoxlineParameterIDs::spaceAmount),
    sound(VoxlineParameterIDs::spaceType),
    sound(VoxlineParameterIDs::spaceTime),
    sound(VoxlineParameterIDs::spacePreDelay),
    sound(VoxlineParameterIDs::spaceWidth),
    sound(VoxlineParameterIDs::spaceTone),
    sound(VoxlineParameterIDs::spaceDecay),
    sound(VoxlineParameterIDs::spaceDucking),
    sound(VoxlineParameterIDs::hpfFreq),
    sound(VoxlineParameterIDs::hpfSlope),
    retired(VoxlineParameterIDs::mudAmount),
    sound(VoxlineParameterIDs::eqEnabled),
    sound(VoxlineParameterIDs::lowFreq),
    retired(VoxlineParameterIDs::lowGain),
    sound(VoxlineParameterIDs::lowQ),
    sound(VoxlineParameterIDs::mudFreq),
    sound(VoxlineParameterIDs::mudGain),
    sound(VoxlineParameterIDs::mudQ),
    sound(VoxlineParameterIDs::presFreq),
    retired(VoxlineParameterIDs::presGain),
    sound(VoxlineParameterIDs::presQ),
    sound(VoxlineParameterIDs::airFreq),
    retired(VoxlineParameterIDs::airGain),
    sound(VoxlineParameterIDs::airQ),
    sound(VoxlineParameterIDs::lpfFreq),
    sound(VoxlineParameterIDs::lpfSlope),
    retired(VoxlineParameterIDs::compThreshold),
    sound(VoxlineParameterIDs::compRatio),
    sound(VoxlineParameterIDs::compAttack),
    sound(VoxlineParameterIDs::compRelease),
    sound(VoxlineParameterIDs::compMix),
    sound(VoxlineParameterIDs::deEssFreq),
    sound(VoxlineParameterIDs::deEssThreshold),
    sound(VoxlineParameterIDs::deEssRange),
    sound(VoxlineParameterIDs::deEssMode),
    sound(VoxlineParameterIDs::driveTone),
    sound(VoxlineParameterIDs::driveMix),
    sound(VoxlineParameterIDs::driveCharacter),
    sound(VoxlineParameterIDs::hpfEnabled),
    sound(VoxlineParameterIDs::lowEnabled),
    sound(VoxlineParameterIDs::mudEnabled),
    sound(VoxlineParameterIDs::presEnabled),
    sound(VoxlineParameterIDs::airEnabled),
    sound(VoxlineParameterIDs::lpfEnabled),
    sound(VoxlineParameterIDs::compSensitivity),
    sound(VoxlineParameterIDs::compMakeup),
    sound(VoxlineParameterIDs::compAutoMakeup),
    sound(VoxlineParameterIDs::driveOutputTrim),
    sound(VoxlineParameterIDs::driveLevelMatch),
    sound(VoxlineParameterIDs::spaceSize),
    sound(VoxlineParameterIDs::spaceFeedback),
    sound(VoxlineParameterIDs::spaceMonoSafety)
};
} // namespace

std::span<const Voxline::ParameterSpec> Voxline::parameterRegistry() noexcept
{
    return registry;
}

const Voxline::ParameterSpec* Voxline::findParameterSpec(juce::StringRef id) noexcept
{
    for (const auto& spec : registry)
        if (id == juce::StringRef(spec.id))
            return &spec;

    return nullptr;
}

juce::ValueTree Voxline::copyRegisteredSoundState(const juce::ValueTree& source)
{
    if (! source.isValid())
        return {};

    juce::ValueTree result(source.getType());
    for (const auto& child : source)
    {
        const auto* spec = findParameterSpec(child.getProperty("id").toString());
        if (spec != nullptr && spec->role == ParameterRole::sound)
            result.appendChild(child.createCopy(), nullptr);
    }

    return result;
}
