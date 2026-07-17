#include "StateSchema.h"

void VoxlineState::serialise(const juce::ValueTree& state,
                             juce::MemoryBlock& destination)
{
    auto versioned = state.createCopy();
    versioned.setProperty("schemaVersion", currentSchemaVersion, nullptr);

    const auto current = migrateToCurrent(versioned);
    if (current)
        if (auto xml = current->createXml())
            juce::AudioProcessor::copyXmlToBinary(*xml, destination);
}

std::optional<juce::ValueTree> VoxlineState::deserialise(
    const void* data, int sizeInBytes, juce::Identifier expectedRoot)
{
    if (data == nullptr || sizeInBytes <= 0)
        return std::nullopt;

    auto xml = juce::AudioProcessor::getXmlFromBinary(data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName(expectedRoot))
        return std::nullopt;

    auto state = juce::ValueTree::fromXml(*xml);
    if (! state.isValid())
        return std::nullopt;

    return migrateToCurrent(state);
}
