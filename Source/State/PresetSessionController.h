#pragma once

#include <JuceHeader.h>

#include "MonitorState.h"
#include "UserPresetLibrary.h"

#include <atomic>

namespace VoxlineState
{
enum class UnsavedAction
{
    save,
    discard,
    cancel
};

struct PresetPresentation
{
    juce::String currentName {"Untitled"};
    bool edited {};
    juce::StringArray names;
};

class PresetSessionController
{
public:
    PresetSessionController(UserPresetLibrary&,
                            juce::AudioProcessorValueTreeState&,
                            MonitorState&);

    PresetPresentation presentation() const;
    juce::Result saveAs(juce::String);
    juce::Result renameCurrent(juce::String);
    juce::Result remove(juce::StringRef);
    juce::Result select(juce::StringRef, UnsavedAction);
    juce::Result selectRelative(int delta, UnsavedAction);
    bool isEdited() const;
    void onParameterChanged() noexcept;
    void onAbChanged() noexcept;
    void onClose() noexcept;

private:
    juce::ValueTree currentSound() const;
    void applySound(const juce::ValueTree&);
    juce::Result resolveUnsaved(UnsavedAction);

    UserPresetLibrary& library;
    juce::AudioProcessorValueTreeState& parameters;
    MonitorState& monitor;
    juce::String currentName {"Untitled"};
    juce::ValueTree baseline;
    std::atomic<bool> dirty {};
};
} // namespace VoxlineState
