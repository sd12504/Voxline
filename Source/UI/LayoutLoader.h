#pragma once

#include <JuceHeader.h>

class LayoutLoader
{
public:
    LayoutLoader() = default;

    /** Load from an external JSON file (hot-reload during development) */
    bool loadFromFile(const juce::File& file);

    /** Load from embedded BinaryData (for release builds) */
    bool loadFromMemory(const void* data, int dataSize);

    /** Load from a juce::String containing JSON text */
    bool loadFromString(const juce::String& jsonText);

    /** Get a component's bounds by its key name. Returns {} if not found. */
    juce::Rectangle<int> getBounds(const juce::String& key) const;

    /** Get the editor (plugin window) size. Defaults to 1080x720 if not specified. */
    int getEditorWidth() const;
    int getEditorHeight() const;

    /** Check if a key exists in the layout */
    bool hasKey(const juce::String& key) const;

    /** Get all component keys (for debugging) */
    juce::StringArray getAllKeys() const;

    /** Reload from the same source (useful for hot-reload) */
    bool reload();

private:
    juce::var root;
    juce::File sourceFile;

    juce::Rectangle<int> parseBounds(const juce::var& obj) const;
};
