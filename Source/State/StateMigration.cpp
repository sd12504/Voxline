#include "StateMigration.h"

#include "../Parameters/ParameterIDs.h"
#include "../Parameters/ParameterRegistry.h"

#include <array>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <limits>

namespace
{
constexpr auto stateRootName = "VOXLINEState";

struct DuplicateEqParameter
{
    const char* activeId;
    const char* duplicateId;
    float duplicateDefault;
};

constexpr std::array duplicateEqParameters {
    DuplicateEqParameter {VoxlineParameterIDs::body,
                          VoxlineParameterIDs::lowGain, 1.5f},
    DuplicateEqParameter {VoxlineParameterIDs::clarity,
                          VoxlineParameterIDs::presGain, 2.0f},
    DuplicateEqParameter {VoxlineParameterIDs::air,
                          VoxlineParameterIDs::airGain, 1.5f}};

constexpr std::array retiredIds {
    VoxlineParameterIDs::autoGain,
    VoxlineParameterIDs::cleanMode,
    VoxlineParameterIDs::listen,
    VoxlineParameterIDs::mudAmount,
    VoxlineParameterIDs::lowGain,
    VoxlineParameterIDs::presGain,
    VoxlineParameterIDs::airGain,
    VoxlineParameterIDs::compThreshold,
    VoxlineParameterIDs::spaceType,
    VoxlineParameterIDs::spaceTime};

juce::ValueTree parameterState(const juce::ValueTree& state)
{
    const auto nested = state.getChildWithName("PARAMETERS");
    return nested.isValid() ? nested : state;
}

std::optional<double> parseFiniteNumber(const juce::var& value) noexcept
{
    if (value.isInt() || value.isInt64() || value.isDouble()
        || value.isBool())
    {
        const auto numeric = static_cast<double>(value);
        return std::isfinite(numeric) ? std::optional<double>(numeric)
                                      : std::nullopt;
    }

    if (! value.isString())
        return std::nullopt;

    const auto text = value.toString();
    const auto* begin = text.toRawUTF8();
    char* end = nullptr;
    errno = 0;
    const auto numeric = std::strtod(begin, &end);

    if (begin == end || end == nullptr || *end != '\0'
        || errno == ERANGE || ! std::isfinite(numeric))
        return std::nullopt;

    return numeric;
}

std::optional<float> parseFiniteFloat(const juce::var& value) noexcept
{
    const auto numeric = parseFiniteNumber(value);
    if (! numeric)
        return std::nullopt;

    const auto narrowed = static_cast<float>(*numeric);
    return std::isfinite(narrowed) ? std::optional<float>(narrowed)
                                   : std::nullopt;
}

std::optional<int> readSchemaVersion(const juce::ValueTree& state)
{
    if (! state.hasProperty("schemaVersion"))
        return 1;

    const auto value = state.getProperty("schemaVersion");
    if (value.isBool())
        return std::nullopt;

    const auto numeric = parseFiniteNumber(value);
    if (! numeric || std::floor(*numeric) != *numeric
        || *numeric < static_cast<double>(std::numeric_limits<int>::min())
        || *numeric > static_cast<double>(std::numeric_limits<int>::max()))
        return std::nullopt;

    return static_cast<int>(*numeric);
}

juce::ValueTree findParameter(const juce::ValueTree& state,
                              const char* id)
{
    for (const auto& child : parameterState(state))
        if (child.getProperty("id").toString() == id)
            return child;

    return {};
}

std::optional<float> readParameterValue(const juce::ValueTree& state,
                                        const char* id)
{
    const auto child = findParameter(state, id);
    if (child.isValid())
    {
        const auto value = child.getProperty("value");
        return parseFiniteFloat(value);
    }

    const auto propertyName = juce::Identifier(id);
    const auto parameters = parameterState(state);
    if (parameters.hasProperty(propertyName))
        return parseFiniteFloat(parameters.getProperty(propertyName));
    if (parameters != state && state.hasProperty(propertyName))
        return parseFiniteFloat(state.getProperty(propertyName));

    return std::nullopt;
}

void setParameterValue(juce::ValueTree& state, const char* id,
                       float value)
{
    auto child = findParameter(state, id);
    if (! child.isValid())
    {
        child = juce::ValueTree("PARAM");
        child.setProperty("id", id, nullptr);
        auto parameters = parameterState(state);
        parameters.appendChild(child, nullptr);
    }

    child.setProperty("value", value, nullptr);
}

void migrateDuplicateEq(juce::ValueTree& state, bool sourceUsesPercent)
{
    const auto activeDefault = sourceUsesPercent ? 50.0f : 0.0f;

    for (const auto& mapping : duplicateEqParameters)
    {
        const auto active = readParameterValue(state, mapping.activeId);
        const auto duplicate =
            readParameterValue(state, mapping.duplicateId);

        if (! active && ! duplicate)
            continue;

        const auto activeWasChanged =
            active && ! juce::approximatelyEqual(*active, activeDefault);
        const auto duplicateWasChanged =
            duplicate
            && ! juce::approximatelyEqual(*duplicate,
                                          mapping.duplicateDefault);

        auto result = 0.0f;
        if (activeWasChanged)
            result = sourceUsesPercent ? (*active - 50.0f) * 0.24f
                                       : *active;
        else if (duplicateWasChanged)
            result = *duplicate;

        setParameterValue(
            state, mapping.activeId,
            juce::jlimit(-12.0f, 12.0f, result));
    }
}

void migrateLegacySpace(juce::ValueTree& state)
{
    const auto legacyMode =
        readParameterValue(state, VoxlineParameterIDs::spaceType)
            .value_or(0.0f);
    const auto legacyModeIndex =
        juce::jlimit(0, 2, juce::roundToInt(legacyMode));

    constexpr std::array mappedModes {0.0f, 3.0f, 4.0f};
    setParameterValue(state, VoxlineParameterIDs::spaceMode,
                      mappedModes[static_cast<size_t>(legacyModeIndex)]);

    if (legacyModeIndex == 1)
    {
        const auto legacyTime =
            readParameterValue(state, VoxlineParameterIDs::spaceTime)
                .value_or(1200.0f);
        setParameterValue(state, VoxlineParameterIDs::spaceSlapTime,
                          juce::jlimit(40.0f, 250.0f, legacyTime));
    }

    state.setProperty("legacySpaceAutomationBridge", true, nullptr);
}

void removeRetiredState(juce::ValueTree& state)
{
    auto parameters = parameterState(state);
    for (const auto* id : retiredIds)
    {
        state.removeProperty(juce::Identifier(id), nullptr);
        parameters.removeProperty(juce::Identifier(id), nullptr);

        for (auto index = parameters.getNumChildren() - 1;
             index >= 0; --index)
            if (parameters.getChild(index)
                    .getProperty("id")
                    .toString() == id)
                parameters.removeChild(index, nullptr);
    }
}

bool containsMalformedKnownParameter(const juce::ValueTree& state)
{
    const auto parameters = parameterState(state);
    for (const auto& child : parameters)
    {
        const auto id = child.getProperty("id").toString();
        if (Voxline::findParameterSpec(id) != nullptr
            && ! parseFiniteFloat(child.getProperty("value")))
            return true;
    }

    for (const auto& spec : Voxline::parameterRegistry())
    {
        if (parameters.hasProperty(spec.id)
            && ! parseFiniteFloat(parameters.getProperty(spec.id)))
            return true;
        if (parameters != state && state.hasProperty(spec.id)
            && ! parseFiniteFloat(state.getProperty(spec.id)))
            return true;
    }

    return false;
}
} // namespace

juce::ValueTree VoxlineState::migrateV1ToV3(juce::ValueTree state)
{
    migrateDuplicateEq(state, true);
    migrateLegacySpace(state);
    removeRetiredState(state);
    state.setProperty("schemaVersion", currentSchemaVersion, nullptr);
    return state;
}

juce::ValueTree VoxlineState::migrateV2ToV3(juce::ValueTree state)
{
    migrateDuplicateEq(state, false);
    migrateLegacySpace(state);
    removeRetiredState(state);
    state.setProperty("schemaVersion", currentSchemaVersion, nullptr);
    return state;
}

std::optional<juce::ValueTree> VoxlineState::migrateToCurrent(
    const juce::ValueTree& source)
{
    if (! source.isValid()
        || ! source.hasType(juce::Identifier(stateRootName))
        || containsMalformedKnownParameter(source))
        return std::nullopt;

    const auto schemaVersion = readSchemaVersion(source);
    if (! schemaVersion || *schemaVersion < 1
        || *schemaVersion > currentSchemaVersion)
        return std::nullopt;

    if (*schemaVersion == 1)
        return migrateV1ToV3(source.createCopy());

    if (*schemaVersion == 2)
        return migrateV2ToV3(source.createCopy());

    auto result = source.createCopy();
    removeRetiredState(result);
    if (! result.hasProperty("legacySpaceAutomationBridge"))
        result.setProperty("legacySpaceAutomationBridge", false, nullptr);
    result.setProperty("schemaVersion", currentSchemaVersion, nullptr);
    return result;
}
