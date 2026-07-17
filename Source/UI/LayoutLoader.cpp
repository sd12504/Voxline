#include "LayoutLoader.h"

bool LayoutLoader::loadFromFile(const juce::File& file)
{
    sourceFile = file;
    return reload();
}

bool LayoutLoader::loadFromMemory(const void* data, int dataSize)
{
    sourceFile = juce::File{};
    auto result = juce::JSON::parse(juce::String::fromUTF8(static_cast<const char*>(data), dataSize), root);
    return result.wasOk();
}

bool LayoutLoader::loadFromString(const juce::String& jsonText)
{
    sourceFile = juce::File{};
    auto result = juce::JSON::parse(jsonText, root);
    return result.wasOk();
}

bool LayoutLoader::reload()
{
    if (sourceFile.existsAsFile())
    {
        auto fileContents = sourceFile.loadFileAsString();
        auto result = juce::JSON::parse(fileContents, root);
        return result.wasOk();
    }
    return false;
}

juce::Rectangle<int> LayoutLoader::getBounds(const juce::String& key) const
{
    auto* components = root["components"].getDynamicObject();
    if (components == nullptr)
        return {};

    auto* obj = components->getProperty(key).getDynamicObject();
    if (obj == nullptr)
        return {};

    return parseBounds(juce::var(obj));
}

juce::Rectangle<int> LayoutLoader::parseBounds(const juce::var& obj) const
{
    const auto* dyn = obj.getDynamicObject();
    if (dyn == nullptr)
        return {};

    auto getInt = [dyn](const juce::Identifier& name)
    {
        return dyn->hasProperty(name) ? static_cast<int>(dyn->getProperty(name)) : 0;
    };

    const int x = getInt("x");
    const int y = getInt("y");
    const int w = getInt("w");
    const int h = getInt("h");

    return { x, y, w, h };
}

int LayoutLoader::getEditorWidth() const
{
    auto* editor = root["editor"].getDynamicObject();
    if (editor == nullptr)
        return 1080;
    return editor->hasProperty("width") ? static_cast<int>(editor->getProperty("width")) : 1080;
}

int LayoutLoader::getEditorHeight() const
{
    auto* editor = root["editor"].getDynamicObject();
    if (editor == nullptr)
        return 720;
    return editor->hasProperty("height") ? static_cast<int>(editor->getProperty("height")) : 720;
}

bool LayoutLoader::hasKey(const juce::String& key) const
{
    auto* components = root["components"].getDynamicObject();
    if (components == nullptr)
        return false;
    return components->hasProperty(key);
}

juce::StringArray LayoutLoader::getAllKeys() const
{
    juce::StringArray keys;
    auto* components = root["components"].getDynamicObject();
    if (components != nullptr)
    {
        for (auto& name : components->getProperties())
            keys.add(name.name.toString());
    }
    return keys;
}
