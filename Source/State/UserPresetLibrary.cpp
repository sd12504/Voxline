#include "UserPresetLibrary.h"

#include "../Parameters/ParameterRegistry.h"
#include "StateMigration.h"

namespace
{
constexpr auto presetRootType = "VOXLINEUserPreset";
constexpr auto presetExtension = ".vxpreset";

bool isWindowsDeviceName(const juce::String& name)
{
    const auto stem = name.upToFirstOccurrenceOf(".", false, false)
                         .toUpperCase();
    if (stem == "CON" || stem == "PRN" || stem == "AUX"
        || stem == "NUL")
        return true;

    for (auto index = 1; index <= 9; ++index)
        if (stem == "COM" + juce::String(index)
            || stem == "LPT" + juce::String(index))
            return true;
    return false;
}
} // namespace

VoxlineState::UserPresetLibrary::UserPresetLibrary(
    juce::File rootDirectory)
    : root(std::move(rootDirectory))
{
}

juce::Result VoxlineState::UserPresetLibrary::initialise()
{
    if (root.exists() && ! root.isDirectory())
        return juce::Result::fail("Preset location is not a directory");
    return root.createDirectory();
}

juce::Result VoxlineState::UserPresetLibrary::normaliseName(
    juce::String& name)
{
    name = name.trim();
    if (name.isEmpty() || name == "." || name == ".."
        || name.equalsIgnoreCase("Untitled"))
        return juce::Result::fail("Invalid preset name");

    if (name.endsWithChar('.')
        || name.containsAnyOf("<>:\"/\\|?*"))
        return juce::Result::fail("Invalid preset name");

    for (const auto character : name)
        if (character < 32 || character == 127)
            return juce::Result::fail("Invalid preset name");

    if (isWindowsDeviceName(name))
        return juce::Result::fail("Invalid preset name");

    return juce::Result::ok();
}

juce::File VoxlineState::UserPresetLibrary::fileFor(
    const juce::String& name) const
{
    return root.getChildFile(name + presetExtension);
}

juce::StringArray VoxlineState::UserPresetLibrary::listNames() const
{
    juce::StringArray names;
    if (! root.isDirectory())
        return names;

    for (const auto& file :
         root.findChildFiles(juce::File::findFiles, false))
    {
        if (! file.getFileExtension().equalsIgnoreCase(presetExtension))
            continue;

        auto name = file.getFileNameWithoutExtension();
        if (normaliseName(name).wasOk())
            names.addIfNotAlreadyThere(name, true);
    }

    names.sortNatural();
    return names;
}

juce::Result VoxlineState::UserPresetLibrary::saveAs(
    juce::String name, const juce::ValueTree& soundState)
{
    if (const auto result = normaliseName(name); result.failed())
        return result;
    if (const auto result = initialise(); result.failed())
        return result;
    if (listNames().contains(name, true))
        return juce::Result::fail("Preset already exists");

    const auto sound = Voxline::copyRegisteredSoundState(soundState);
    if (! sound.isValid() || sound.getNumChildren() == 0)
        return juce::Result::fail("Preset contains no sound parameters");

    juce::ValueTree payload(presetRootType);
    payload.setProperty("formatVersion", currentSchemaVersion, nullptr);
    for (const auto& child : sound)
        payload.appendChild(child.createCopy(), nullptr);

    const auto xml = payload.createXml();
    if (xml == nullptr)
        return juce::Result::fail("Could not encode preset");

    const auto target = fileFor(name);
    juce::TemporaryFile temporary(target);
    auto stream = temporary.getFile().createOutputStream();
    if (stream == nullptr)
        return juce::Result::fail("Could not write preset");

    xml->writeTo(*stream);
    stream->flush();
    const auto writeResult = stream->getStatus();
    stream.reset();
    if (writeResult.failed())
        return writeResult;

    juce::ValueTree verified;
    if (const auto result = read(temporary.getFile(), verified);
        result.failed())
        return result;
    if (! verified.isEquivalentTo(sound))
        return juce::Result::fail("Preset verification failed");

    if (! temporary.overwriteTargetFileWithTemporary())
        return juce::Result::fail("Could not replace preset");
    return juce::Result::ok();
}

juce::Result VoxlineState::UserPresetLibrary::read(
    const juce::File& file, juce::ValueTree& destination) const
{
    const auto xml = juce::XmlDocument::parse(file);
    if (xml == nullptr || ! xml->hasTagName(presetRootType))
        return juce::Result::fail("Invalid preset file");

    const auto payload = juce::ValueTree::fromXml(*xml);
    if (! payload.isValid())
        return juce::Result::fail("Invalid preset data");

    juce::ValueTree migrationState("VOXLINEState");
    if (payload.hasProperty("formatVersion"))
        migrationState.setProperty(
            "schemaVersion", payload.getProperty("formatVersion"), nullptr);

    for (auto index = 0; index < payload.getNumProperties(); ++index)
    {
        const auto property = payload.getPropertyName(index);
        if (property != juce::Identifier("formatVersion"))
            migrationState.setProperty(
                property, payload.getProperty(property), nullptr);
    }

    for (const auto& child : payload)
        migrationState.appendChild(child.createCopy(), nullptr);

    const auto migrated = migrateToCurrent(migrationState);
    if (! migrated)
        return juce::Result::fail("Unsupported or corrupt preset");

    const auto sound = Voxline::copyRegisteredSoundState(*migrated);
    if (! sound.isValid() || sound.getNumChildren() == 0)
        return juce::Result::fail("Preset contains no sound parameters");

    destination = sound;
    return juce::Result::ok();
}

juce::Result VoxlineState::UserPresetLibrary::load(
    juce::StringRef requestedName, juce::ValueTree& destination) const
{
    auto name = juce::String(requestedName);
    if (const auto result = normaliseName(name); result.failed())
        return result;
    return read(fileFor(name), destination);
}

juce::Result VoxlineState::UserPresetLibrary::rename(
    juce::StringRef requestedFrom, juce::String to)
{
    auto from = juce::String(requestedFrom);
    if (const auto result = normaliseName(from); result.failed())
        return result;
    if (const auto result = normaliseName(to); result.failed())
        return result;

    const auto source = fileFor(from);
    if (! source.existsAsFile())
        return juce::Result::fail("Preset does not exist");
    if (listNames().contains(to, true))
        return from == to ? juce::Result::ok()
                          : juce::Result::fail("Preset already exists");

    return source.moveFileTo(fileFor(to))
               ? juce::Result::ok()
               : juce::Result::fail("Could not rename preset");
}

juce::Result VoxlineState::UserPresetLibrary::remove(
    juce::StringRef requestedName)
{
    auto name = juce::String(requestedName);
    if (const auto result = normaliseName(name); result.failed())
        return result;

    const auto file = fileFor(name);
    if (! file.existsAsFile())
        return juce::Result::fail("Preset does not exist");
    return file.deleteFile() ? juce::Result::ok()
                             : juce::Result::fail("Could not delete preset");
}
