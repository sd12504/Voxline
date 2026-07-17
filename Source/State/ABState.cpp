#include "ABState.h"

#include "../Parameters/ParameterRegistry.h"

#include <cerrno>
#include <cmath>
#include <cstdlib>

namespace
{
constexpr auto abStateType = "AB_STATE";
constexpr auto slotAType = "SLOT_A";
constexpr auto slotBType = "SLOT_B";

std::optional<float> finiteFloat(const juce::var& value)
{
    double numeric {};
    if (value.isInt() || value.isInt64() || value.isDouble()
        || value.isBool())
    {
        numeric = static_cast<double>(value);
    }
    else if (value.isString())
    {
        const auto text = value.toString();
        const auto* begin = text.toRawUTF8();
        char* end = nullptr;
        errno = 0;
        numeric = std::strtod(begin, &end);
        if (begin == end || end == nullptr || *end != '\0'
            || errno == ERANGE)
            return std::nullopt;
    }
    else
    {
        return std::nullopt;
    }

    const auto result = static_cast<float>(numeric);
    return std::isfinite(numeric) && std::isfinite(result)
               ? std::optional<float>(result)
               : std::nullopt;
}

juce::ValueTree findParameter(const juce::ValueTree& state,
                              const char* id)
{
    for (const auto& child : state)
        if (child.getProperty("id").toString() == id)
            return child;
    return {};
}
} // namespace

VoxlineState::AbStateManager::AbStateManager(
    juce::AudioProcessorValueTreeState& state)
    : parameters(state)
{
    initialiseFromCurrentSound();
}

juce::ValueTree VoxlineState::AbStateManager::snapshot(
    const juce::Identifier& type) const
{
    juce::ValueTree result(type);
    const auto sound =
        Voxline::copyRegisteredSoundState(parameters.copyState());
    for (const auto& child : sound)
        result.appendChild(child.createCopy(), nullptr);
    return result;
}

void VoxlineState::AbStateManager::initialiseFromCurrentSound()
{
    slotA = snapshot(slotAType);
    slotB = snapshot(slotBType);
    active = AbSlot::a;
}

void VoxlineState::AbStateManager::captureActiveSlot()
{
    if (active == AbSlot::a)
        slotA = snapshot(slotAType);
    else
        slotB = snapshot(slotBType);
}

void VoxlineState::AbStateManager::select(AbSlot slot)
{
    if (slot == active)
        return;

    captureActiveSlot();
    active = slot;
    apply(active == AbSlot::a ? slotA : slotB);
}

VoxlineState::AbSlot
VoxlineState::AbStateManager::activeSlot() const noexcept
{
    return active;
}

juce::ValueTree VoxlineState::AbStateManager::toValueTree() const
{
    juce::ValueTree result(abStateType);
    result.setProperty("active", active == AbSlot::a ? "A" : "B", nullptr);
    result.appendChild(slotA.createCopy(), nullptr);
    result.appendChild(slotB.createCopy(), nullptr);
    return result;
}

bool VoxlineState::AbStateManager::isValidSnapshot(
    const juce::ValueTree& state) const
{
    auto expectedCount = 0;
    for (const auto& spec : Voxline::parameterRegistry())
    {
        if (! spec.saveInAb)
            continue;

        ++expectedCount;
        const auto child = findParameter(state, spec.id);
        if (! child.isValid()
            || ! finiteFloat(child.getProperty("value")))
            return false;
    }

    if (state.getNumChildren() != expectedCount)
        return false;

    for (const auto& child : state)
    {
        const auto* spec = Voxline::findParameterSpec(
            child.getProperty("id").toString());
        if (spec == nullptr || ! spec->saveInAb)
            return false;
    }

    return true;
}

void VoxlineState::AbStateManager::apply(const juce::ValueTree& state)
{
    for (const auto& child : state)
    {
        const auto id = child.getProperty("id").toString();
        const auto* spec = Voxline::findParameterSpec(id);
        const auto value = finiteFloat(child.getProperty("value"));
        auto* parameter = parameters.getParameter(id);
        if (spec != nullptr && spec->saveInAb && value && parameter != nullptr)
            parameter->setValueNotifyingHost(
                parameter->convertTo0to1(*value));
    }
}

void VoxlineState::AbStateManager::restore(const juce::ValueTree& state)
{
    const auto restoredA = state.getChildWithName(slotAType);
    const auto restoredB = state.getChildWithName(slotBType);
    const auto activeName = state.getProperty("active").toString();

    if (! state.hasType(abStateType)
        || ! isValidSnapshot(restoredA)
        || ! isValidSnapshot(restoredB)
        || (activeName != "A" && activeName != "B"))
    {
        initialiseFromCurrentSound();
        return;
    }

    slotA = restoredA.createCopy();
    slotB = restoredB.createCopy();
    active = activeName == "B" ? AbSlot::b : AbSlot::a;
    apply(active == AbSlot::a ? slotA : slotB);
}
