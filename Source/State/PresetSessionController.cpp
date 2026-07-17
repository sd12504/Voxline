#include "PresetSessionController.h"

#include "../Parameters/ParameterRegistry.h"

VoxlineState::PresetSessionController::PresetSessionController(
    UserPresetLibrary& presetLibrary,
    juce::AudioProcessorValueTreeState& state,
    MonitorState& monitorState)
    : library(presetLibrary),
      parameters(state),
      monitor(monitorState),
      baseline(currentSound())
{
}

juce::ValueTree
VoxlineState::PresetSessionController::currentSound() const
{
    return Voxline::copyRegisteredSoundState(parameters.copyState());
}

VoxlineState::PresetPresentation
VoxlineState::PresetSessionController::presentation() const
{
    return {currentName, isEdited(), library.listNames()};
}

bool VoxlineState::PresetSessionController::isEdited() const
{
    return dirty.load(std::memory_order_acquire)
           && ! currentSound().isEquivalentTo(baseline);
}

void VoxlineState::PresetSessionController::onParameterChanged() noexcept
{
    dirty.store(true, std::memory_order_release);
}

juce::Result VoxlineState::PresetSessionController::saveAs(
    juce::String name)
{
    const auto sound = currentSound();
    if (const auto result = library.saveAs(name, sound); result.failed())
        return result;

    currentName = name.trim();
    baseline = sound;
    dirty.store(false, std::memory_order_release);
    return juce::Result::ok();
}

juce::Result VoxlineState::PresetSessionController::renameCurrent(
    juce::String name)
{
    if (currentName == "Untitled")
        return juce::Result::fail("Save the preset before renaming it");

    if (const auto result = library.rename(currentName, name);
        result.failed())
        return result;

    currentName = name.trim();
    return juce::Result::ok();
}

juce::Result VoxlineState::PresetSessionController::remove(
    juce::StringRef requestedName)
{
    const auto name = juce::String(requestedName).trim();
    if (const auto result = library.remove(name); result.failed())
        return result;

    if (currentName.equalsIgnoreCase(name))
    {
        currentName = "Untitled";
        baseline = currentSound();
        dirty.store(false, std::memory_order_release);
        monitor.clear();
    }
    return juce::Result::ok();
}

juce::Result VoxlineState::PresetSessionController::resolveUnsaved(
    UnsavedAction action)
{
    if (! isEdited())
        return juce::Result::ok();
    if (action == UnsavedAction::cancel)
        return juce::Result::fail("Cancelled");
    if (action == UnsavedAction::discard)
        return juce::Result::ok();
    if (currentName == "Untitled")
        return juce::Result::fail("Save As is required");

    const auto sound = currentSound();
    if (const auto result = library.replace(currentName, sound);
        result.failed())
        return result;
    baseline = sound;
    dirty.store(false, std::memory_order_release);
    return juce::Result::ok();
}

void VoxlineState::PresetSessionController::applySound(
    const juce::ValueTree& sound)
{
    for (const auto& child : sound)
    {
        const auto id = child.getProperty("id").toString();
        const auto* spec = Voxline::findParameterSpec(id);
        auto* parameter = parameters.getParameter(id);
        if (spec != nullptr && spec->saveInPreset && parameter != nullptr)
        {
            const auto value =
                static_cast<float>(child.getProperty("value"));
            parameter->setValueNotifyingHost(
                parameter->convertTo0to1(value));
        }
    }
}

juce::Result VoxlineState::PresetSessionController::select(
    juce::StringRef requestedName, UnsavedAction action)
{
    if (const auto result = resolveUnsaved(action); result.failed())
        return result;

    juce::ValueTree sound;
    if (const auto result = library.load(requestedName, sound);
        result.failed())
        return result;

    applySound(sound);
    currentName = juce::String(requestedName).trim();
    baseline = currentSound();
    dirty.store(false, std::memory_order_release);
    monitor.clear();
    return juce::Result::ok();
}

juce::Result VoxlineState::PresetSessionController::selectRelative(
    int delta, UnsavedAction action)
{
    const auto names = library.listNames();
    if (names.isEmpty())
        return juce::Result::fail("No user presets");

    auto currentIndex = -1;
    for (auto index = 0; index < names.size(); ++index)
        if (names[index].equalsIgnoreCase(currentName))
        {
            currentIndex = index;
            break;
        }

    if (currentIndex < 0)
        return select(delta < 0 ? names[names.size() - 1] : names[0],
                      action);

    const auto count = names.size();
    const auto target = ((currentIndex + delta) % count + count) % count;
    return select(names[target], action);
}

void VoxlineState::PresetSessionController::onAbChanged() noexcept
{
    monitor.clear();
    onParameterChanged();
}

void VoxlineState::PresetSessionController::onClose() noexcept
{
    monitor.clear();
}
