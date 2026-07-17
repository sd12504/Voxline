#pragma once

#include <JuceHeader.h>

namespace VoxlineState
{
class UserPresetLibrary
{
public:
    explicit UserPresetLibrary(juce::File rootDirectory);

    juce::Result initialise();
    juce::StringArray listNames() const;
    juce::Result saveAs(juce::String name,
                        const juce::ValueTree& soundState);
    juce::Result load(juce::StringRef name,
                      juce::ValueTree& destination) const;
    juce::Result rename(juce::StringRef from, juce::String to);
    juce::Result remove(juce::StringRef name);

private:
    static juce::Result normaliseName(juce::String&);
    juce::File fileFor(const juce::String&) const;
    juce::Result read(const juce::File&, juce::ValueTree&) const;

    juce::File root;
};
} // namespace VoxlineState
