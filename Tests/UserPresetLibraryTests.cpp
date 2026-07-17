#include <JuceHeader.h>

#include "../Source/Parameters/ParameterIDs.h"
#include "../Source/Parameters/ParameterRegistry.h"
#include "../Source/State/UserPresetLibrary.h"

namespace
{
class ScopedDirectory
{
public:
    ScopedDirectory()
        : directory(juce::File::getSpecialLocation(
                        juce::File::tempDirectory)
                        .getNonexistentChildFile(
                            "voxline-preset-tests", {}, true))
    {
        directory.createDirectory();
    }

    ~ScopedDirectory() { directory.deleteRecursively(); }

    juce::File directory;
};

juce::ValueTree parameterNode(const char* id, float value)
{
    juce::ValueTree node("PARAM");
    node.setProperty("id", id, nullptr);
    node.setProperty("value", value, nullptr);
    return node;
}

juce::ValueTree soundState()
{
    juce::ValueTree state("VOXLINEState");
    auto index = 1;
    for (const auto& spec : Voxline::parameterRegistry())
        if (spec.saveInPreset)
            state.appendChild(
                parameterNode(spec.id, static_cast<float>(index++) / 100.0f),
                nullptr);
    return state;
}

juce::ValueTree findParameter(const juce::ValueTree& state,
                              const juce::String& id)
{
    for (const auto& child : state)
        if (child.getProperty("id").toString() == id)
            return child;
    return {};
}

float parameterValue(const juce::ValueTree& state, const char* id)
{
    return static_cast<float>(
        findParameter(state, id).getProperty("value"));
}

juce::File fixture(const juce::String& name)
{
    auto directory = juce::File::getCurrentWorkingDirectory();
    for (auto depth = 0; depth < 12; ++depth)
    {
        const auto candidate =
            directory.getChildFile("Tests")
                .getChildFile("Fixtures")
                .getChildFile("Presets")
                .getChildFile(name);
        if (candidate.existsAsFile())
            return candidate;
        directory = directory.getParentDirectory();
    }
    return {};
}

class UserPresetLibraryTests final : public juce::UnitTest
{
public:
    UserPresetLibraryTests()
        : juce::UnitTest("User preset library", "VOXLINE") {}

    void runTest() override
    {
        beginTest("new library starts empty and creates its directory");
        {
            ScopedDirectory temporary;
            const auto root = temporary.directory.getChildFile("Presets");
            VoxlineState::UserPresetLibrary library(root);
            expect(library.initialise().wasOk());
            expect(root.isDirectory());
            expect(library.listNames().isEmpty());
        }

        beginTest("Save As round-trips sound and writes a filtered v3 payload");
        {
            ScopedDirectory temporary;
            VoxlineState::UserPresetLibrary library(temporary.directory);
            expect(library.initialise().wasOk());

            auto source = soundState();
            source.appendChild(
                parameterNode(VoxlineParameterIDs::bypass, 1.0f), nullptr);
            source.appendChild(
                parameterNode(VoxlineParameterIDs::autoGain, 1.0f), nullptr);
            source.appendChild(juce::ValueTree("AB_STATE"), nullptr);
            source.appendChild(juce::ValueTree("UI_STATE"), nullptr);
            source.setProperty("meterPeak", 1.0f, nullptr);
            source.setProperty("clipHold", true, nullptr);
            source.setProperty("advancedOpen", true, nullptr);

            expect(library.saveAs("  My Vocal  ", source).wasOk());
            expect(library.listNames()
                   == juce::StringArray {"My Vocal"});

            juce::ValueTree loaded("Before");
            expect(library.load("My Vocal", loaded).wasOk());
            expect(loaded.isEquivalentTo(
                Voxline::copyRegisteredSoundState(source)));

            const auto file =
                temporary.directory.getChildFile("My Vocal.vxpreset");
            const auto xml = juce::XmlDocument::parse(file);
            expect(xml != nullptr);
            if (xml != nullptr)
            {
                const auto payload = juce::ValueTree::fromXml(*xml);
                expect(payload.hasType("VOXLINEUserPreset"));
                expectEquals(
                    static_cast<int>(payload.getProperty("formatVersion")),
                    3);
                expectEquals(
                    payload.getNumChildren(),
                    Voxline::copyRegisteredSoundState(source)
                        .getNumChildren());
                expect(! payload.hasProperty("meterPeak"));
                expect(! payload.hasProperty("clipHold"));
                expect(! payload.hasProperty("advancedOpen"));
                for (const auto& child : payload)
                {
                    const auto* spec = Voxline::findParameterSpec(
                        child.getProperty("id").toString());
                    expect(spec != nullptr);
                    if (spec != nullptr)
                        expect(spec->saveInPreset);
                }
            }
        }

        beginTest("invalid and duplicate names are rejected");
        {
            ScopedDirectory temporary;
            VoxlineState::UserPresetLibrary library(temporary.directory);
            expect(library.initialise().wasOk());
            const auto source = soundState();
            expect(library.saveAs("Valid", source).wasOk());

            for (const auto& invalid :
                 {juce::String(), juce::String("   "),
                  juce::String("Untitled"), juce::String("untitled"),
                  juce::String("A/B"), juce::String("A\\B"),
                  juce::String("Bad") + juce::String::charToString(1)})
                expect(library.saveAs(invalid, source).failed(), invalid);

            expect(library.saveAs(" valid ", source).failed());
            expectEquals(library.listNames().size(), 1);
        }

        beginTest("names use case-insensitive natural sorting");
        {
            ScopedDirectory temporary;
            VoxlineState::UserPresetLibrary library(temporary.directory);
            expect(library.initialise().wasOk());
            const auto source = soundState();
            expect(library.saveAs("Vocal 10", source).wasOk());
            expect(library.saveAs("vocal 2", source).wasOk());
            expect(library.saveAs("Alpha", source).wasOk());
            expect(library.listNames()
                   == juce::StringArray {
                       "Alpha", "vocal 2", "Vocal 10"});
        }

        beginTest("rename preserves payload and remove touches only its target");
        {
            ScopedDirectory temporary;
            VoxlineState::UserPresetLibrary library(temporary.directory);
            expect(library.initialise().wasOk());
            const auto source = soundState();
            expect(library.saveAs("First", source).wasOk());
            expect(library.saveAs("Keep", source).wasOk());
            expect(library.rename("First", " Renamed ").wasOk());
            expect(library.rename("Renamed", "keep").failed());

            juce::ValueTree loaded;
            expect(library.load("Renamed", loaded).wasOk());
            expect(loaded.isEquivalentTo(source));
            expect(library.remove("Renamed").wasOk());
            expect(library.load("Renamed", loaded).failed());
            expect(library.load("Keep", loaded).wasOk());
        }

        beginTest("v1 and v2 presets migrate while v3 unknown fields are ignored");
        {
            ScopedDirectory temporary;
            const auto v1 = fixture("v1-percent.vxpreset");
            const auto v3 = fixture("v3-unknown-fields.vxpreset");
            expect(v1.copyFileTo(
                temporary.directory.getChildFile("Legacy 1.vxpreset")));
            expect(v3.copyFileTo(
                temporary.directory.getChildFile("Modern.vxpreset")));
            juce::ValueTree legacyV2("VOXLINEUserPreset");
            legacyV2.setProperty("formatVersion", 2, nullptr);
            legacyV2.setProperty(
                VoxlineParameterIDs::body, -6.0f, nullptr);
            legacyV2.setProperty(
                VoxlineParameterIDs::clarity, 0.0f, nullptr);
            legacyV2.setProperty(
                VoxlineParameterIDs::air, 6.0f, nullptr);
            if (auto xml = legacyV2.createXml())
                xml->writeTo(temporary.directory.getChildFile(
                    "Legacy 2.vxpreset"));

            VoxlineState::UserPresetLibrary library(temporary.directory);
            expect(library.initialise().wasOk());

            juce::ValueTree loaded;
            expect(library.load("Legacy 1", loaded).wasOk());
            expectWithinAbsoluteError(
                parameterValue(loaded, VoxlineParameterIDs::body),
                -12.0f, 0.001f);
            expectWithinAbsoluteError(
                parameterValue(loaded, VoxlineParameterIDs::clarity),
                0.0f, 0.001f);
            expectWithinAbsoluteError(
                parameterValue(loaded, VoxlineParameterIDs::air),
                12.0f, 0.001f);
            expect(! findParameter(
                        loaded, VoxlineParameterIDs::lowGain)
                        .isValid());

            expect(library.load("Legacy 2", loaded).wasOk());
            expectWithinAbsoluteError(
                parameterValue(loaded, VoxlineParameterIDs::body),
                -6.0f, 0.001f);
            expectWithinAbsoluteError(
                parameterValue(loaded, VoxlineParameterIDs::air),
                6.0f, 0.001f);

            expect(library.load("Modern", loaded).wasOk());
            expectWithinAbsoluteError(
                parameterValue(loaded, VoxlineParameterIDs::body),
                4.0f, 0.001f);
            expect(! findParameter(loaded, "futureControl").isValid());
        }

        beginTest("stray temporary and corrupt files never replace live data");
        {
            ScopedDirectory temporary;
            VoxlineState::UserPresetLibrary library(temporary.directory);
            expect(library.initialise().wasOk());
            const auto source = soundState();
            expect(library.saveAs("Safe", source).wasOk());

            temporary.directory.getChildFile("Safe.vxpreset.tmp")
                .replaceWithText("<broken");
            temporary.directory.getChildFile("Broken.vxpreset")
                .replaceWithText("<broken");

            juce::ValueTree loaded("Before");
            expect(library.load("Safe", loaded).wasOk());
            expect(loaded.isEquivalentTo(source));

            const auto before = loaded.createCopy();
            expect(library.load("Broken", loaded).failed());
            expect(loaded.isEquivalentTo(before));
            expect(! library.listNames().contains(
                "Safe.vxpreset", true));
        }
    }
};

UserPresetLibraryTests userPresetLibraryTests;
} // namespace
