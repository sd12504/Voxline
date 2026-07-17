#include <JuceHeader.h>

#include "../Source/Parameters/ParameterIDs.h"
#include "../Source/State/StateMigration.h"
#include "../Source/State/StateSchema.h"

namespace
{
juce::File fixture(const juce::String& name)
{
    const std::array searchRoots {
        juce::File::getCurrentWorkingDirectory(),
        juce::File::getSpecialLocation(
            juce::File::currentExecutableFile)};

    for (const auto& root : searchRoots)
    {
        auto directory = root;
        for (auto depth = 0; depth < 12; ++depth)
        {
            const auto candidate = directory.getChildFile("Tests")
                                       .getChildFile("Fixtures")
                                       .getChildFile("State")
                                       .getChildFile(name);
            if (candidate.existsAsFile())
                return candidate;

            const auto parent = directory.getParentDirectory();
            if (parent == directory)
                break;
            directory = parent;
        }
    }

    return {};
}

juce::ValueTree readFixture(const juce::String& name)
{
    const auto file = fixture(name);
    if (auto xml = juce::XmlDocument::parse(file))
        return juce::ValueTree::fromXml(*xml);

    return {};
}

juce::ValueTree parameterNode(const juce::String& id, float value)
{
    juce::ValueTree parameter("PARAM");
    parameter.setProperty("id", id, nullptr);
    parameter.setProperty("value", value, nullptr);
    return parameter;
}

juce::ValueTree findParameter(const juce::ValueTree& state,
                              const juce::String& id)
{
    for (const auto& child : state)
        if (child.getProperty("id").toString() == id)
            return child;

    return {};
}

float parameterValue(const juce::ValueTree& state, const juce::String& id)
{
    const auto child = findParameter(state, id);
    return child.isValid() ? static_cast<float>(child.getProperty("value"))
                           : std::numeric_limits<float>::quiet_NaN();
}

bool hasParameter(const juce::ValueTree& state, const juce::String& id)
{
    return findParameter(state, id).isValid();
}

juce::MemoryBlock asBinary(const juce::ValueTree& state)
{
    juce::MemoryBlock bytes;
    if (auto xml = state.createXml())
        juce::AudioProcessor::copyXmlToBinary(*xml, bytes);
    return bytes;
}

class StateMigrationTests final : public juce::UnitTest
{
public:
    StateMigrationTests()
        : juce::UnitTest("State migration v1 v2 v3", "VOXLINE") {}

    void runTest() override
    {
        beginTest("v1 percent tone values map to plus or minus twelve dB");
        {
            const auto migrated = VoxlineState::migrateToCurrent(
                readFixture("v1-percent.xml"));
            expect(migrated.has_value());
            if (migrated)
            {
                expectEquals(static_cast<int>(
                                 migrated->getProperty("schemaVersion")),
                             VoxlineState::currentSchemaVersion);
                expectWithinAbsoluteError(
                    parameterValue(*migrated, VoxlineParameterIDs::body),
                    -12.0f, 0.0001f);
                expectWithinAbsoluteError(
                    parameterValue(*migrated, VoxlineParameterIDs::clarity),
                    0.0f, 0.0001f);
                expectWithinAbsoluteError(
                    parameterValue(*migrated, VoxlineParameterIDs::air),
                    12.0f, 0.0001f);
            }

            auto explicitV1 = readFixture("v1-percent.xml");
            explicitV1.setProperty("schemaVersion", 1, nullptr);
            const auto explicitMigrated =
                VoxlineState::migrateToCurrent(explicitV1);
            expect(explicitMigrated.has_value());
            if (explicitMigrated)
                expectWithinAbsoluteError(
                    parameterValue(*explicitMigrated,
                                   VoxlineParameterIDs::air),
                    12.0f, 0.0001f);
        }

        beginTest("v2 dB values stay in dB when the range expands");
        {
            const auto migrated = VoxlineState::migrateToCurrent(
                readFixture("v2-minus6-plus6.xml"));
            expect(migrated.has_value());
            if (migrated)
            {
                expectWithinAbsoluteError(
                    parameterValue(*migrated, VoxlineParameterIDs::body),
                    -6.0f, 0.0001f);
                expectWithinAbsoluteError(
                    parameterValue(*migrated, VoxlineParameterIDs::clarity),
                    0.0f, 0.0001f);
                expectWithinAbsoluteError(
                    parameterValue(*migrated, VoxlineParameterIDs::air),
                    6.0f, 0.0001f);
            }
        }

        beginTest("changed duplicate supplies a default active EQ value");
        {
            const auto migrated = VoxlineState::migrateToCurrent(
                readFixture("v2-duplicate-only.xml"));
            expect(migrated.has_value());
            if (migrated)
            {
                expectWithinAbsoluteError(
                    parameterValue(*migrated, VoxlineParameterIDs::body),
                    12.0f, 0.0001f);
                expectWithinAbsoluteError(
                    parameterValue(*migrated, VoxlineParameterIDs::clarity),
                    -12.0f, 0.0001f);
                expectWithinAbsoluteError(
                    parameterValue(*migrated, VoxlineParameterIDs::air),
                    4.5f, 0.0001f);
            }
        }

        beginTest("changed active EQ value wins over changed duplicate");
        {
            const auto migrated = VoxlineState::migrateToCurrent(
                readFixture("v2-active-wins.xml"));
            expect(migrated.has_value());
            if (migrated)
            {
                expectWithinAbsoluteError(
                    parameterValue(*migrated, VoxlineParameterIDs::body),
                    -3.0f, 0.0001f);
                expectWithinAbsoluteError(
                    parameterValue(*migrated, VoxlineParameterIDs::clarity),
                    2.5f, 0.0001f);
                expectWithinAbsoluteError(
                    parameterValue(*migrated, VoxlineParameterIDs::air),
                    5.0f, 0.0001f);
            }
        }

        beginTest("v3 values remain unchanged and unknown fields parse");
        {
            const auto source = readFixture("v3-unknown-fields.xml");
            const auto migrated = VoxlineState::migrateToCurrent(source);
            expect(migrated.has_value());
            if (migrated)
            {
                expectWithinAbsoluteError(
                    parameterValue(*migrated, VoxlineParameterIDs::body),
                    10.75f, 0.0001f);
                expectWithinAbsoluteError(
                    parameterValue(*migrated, "futureSoundControl"),
                    0.37f, 0.0001f);
                expectEquals(
                    migrated->getProperty("futureRootField").toString(),
                    juce::String("preserved"));
                expectWithinAbsoluteError(
                    parameterValue(*migrated,
                                   VoxlineParameterIDs::spaceMode),
                    2.0f, 0.0001f);
                expectWithinAbsoluteError(
                    parameterValue(*migrated,
                                   VoxlineParameterIDs::spaceSlapTime),
                    145.0f, 0.0001f);
                expect(! static_cast<bool>(migrated->getProperty(
                    "legacySpaceAutomationBridge", false)));
            }

            auto migratedLegacySession = source.createCopy();
            migratedLegacySession.setProperty(
                "legacySpaceAutomationBridge", true, nullptr);
            const auto restoredLegacySession =
                VoxlineState::migrateToCurrent(migratedLegacySession);
            expect(restoredLegacySession.has_value());
            if (restoredLegacySession)
                expect(static_cast<bool>(
                    restoredLegacySession->getProperty(
                        "legacySpaceAutomationBridge", false)));
        }

        beginTest("legacy Space modes and Slap time migrate with bridge enabled");
        {
            const auto room = VoxlineState::migrateToCurrent(
                readFixture("v1-percent.xml"));
            expect(room.has_value());
            if (room)
            {
                expectWithinAbsoluteError(
                    parameterValue(*room, VoxlineParameterIDs::spaceMode),
                    0.0f, 0.0001f);
                expect(static_cast<bool>(room->getProperty(
                    "legacySpaceAutomationBridge", false)));
            }

            const auto slap = VoxlineState::migrateToCurrent(
                readFixture("v2-minus6-plus6.xml"));
            expect(slap.has_value());
            if (slap)
            {
                expectWithinAbsoluteError(
                    parameterValue(*slap, VoxlineParameterIDs::spaceMode),
                    3.0f, 0.0001f);
                expectWithinAbsoluteError(
                    parameterValue(*slap,
                                   VoxlineParameterIDs::spaceSlapTime),
                    250.0f, 0.0001f);
                expect(static_cast<bool>(slap->getProperty(
                    "legacySpaceAutomationBridge", false)));
            }

            const auto width = VoxlineState::migrateToCurrent(
                readFixture("v2-duplicate-only.xml"));
            expect(width.has_value());
            if (width)
            {
                expectWithinAbsoluteError(
                    parameterValue(*width, VoxlineParameterIDs::spaceMode),
                    4.0f, 0.0001f);
                expect(static_cast<bool>(width->getProperty(
                    "legacySpaceAutomationBridge", false)));
            }
        }

        beginTest("migration removes every retired saved value");
        {
            auto state = readFixture("v2-active-wins.xml");
            state.setProperty(VoxlineParameterIDs::autoGain, true, nullptr);
            state.appendChild(
                parameterNode(VoxlineParameterIDs::autoGain, 1.0f), nullptr);
            state.appendChild(
                parameterNode(VoxlineParameterIDs::cleanMode, 1.0f), nullptr);
            state.appendChild(
                parameterNode(VoxlineParameterIDs::listen, 1.0f), nullptr);
            state.appendChild(
                parameterNode(VoxlineParameterIDs::mudAmount, 80.0f), nullptr);
            state.appendChild(
                parameterNode(VoxlineParameterIDs::compThreshold, -9.0f),
                nullptr);

            const auto migrated = VoxlineState::migrateToCurrent(state);
            expect(migrated.has_value());
            if (migrated)
            {
                for (const auto* id : {
                         VoxlineParameterIDs::autoGain,
                         VoxlineParameterIDs::cleanMode,
                         VoxlineParameterIDs::listen,
                         VoxlineParameterIDs::mudAmount,
                         VoxlineParameterIDs::lowGain,
                         VoxlineParameterIDs::presGain,
                         VoxlineParameterIDs::airGain,
                         VoxlineParameterIDs::compThreshold,
                         VoxlineParameterIDs::spaceType,
                         VoxlineParameterIDs::spaceTime})
                {
                    expect(! hasParameter(*migrated, id), id);
                    expect(! migrated->hasProperty(id), id);
                }
            }
        }

        beginTest("future corrupt and wrong-root states do not replace live state");
        {
            juce::ValueTree live("VOXLINEState");
            live.appendChild(parameterNode(VoxlineParameterIDs::body, 3.0f),
                             nullptr);
            const auto before = live.createCopy();

            auto future = live.createCopy();
            future.setProperty("schemaVersion",
                               VoxlineState::currentSchemaVersion + 1, nullptr);
            if (const auto decoded = VoxlineState::migrateToCurrent(future))
                live = *decoded;
            expect(live.isEquivalentTo(before));

            juce::ValueTree wrongRoot("WrongRoot");
            wrongRoot.setProperty("schemaVersion", 2, nullptr);
            if (const auto decoded =
                    VoxlineState::migrateToCurrent(wrongRoot))
                live = *decoded;
            expect(live.isEquivalentTo(before));

            juce::ValueTree corruptVersion("VOXLINEState");
            corruptVersion.setProperty("schemaVersion", "not-a-version",
                                       nullptr);
            if (const auto decoded =
                    VoxlineState::migrateToCurrent(corruptVersion))
                live = *decoded;
            expect(live.isEquivalentTo(before));

            for (const auto& invalidVersion :
                 {juce::String("3junk"), juce::String("NaN"),
                  juce::String("Inf")})
            {
                auto malformed = before.createCopy();
                malformed.setProperty(
                    "schemaVersion", invalidVersion, nullptr);
                expect(! VoxlineState::migrateToCurrent(malformed)
                            .has_value(),
                       invalidVersion);
            }

            for (const auto& invalidValue :
                 {juce::String("2.5junk"), juce::String("NaN"),
                  juce::String("-Inf")})
            {
                auto malformed = before.createCopy();
                malformed.setProperty("schemaVersion", 3, nullptr);
                malformed.appendChild(
                    parameterNode(VoxlineParameterIDs::spaceMode, 2.0f),
                    nullptr);
                auto mode = findParameter(
                    malformed, VoxlineParameterIDs::spaceMode);
                mode.setProperty("value", invalidValue, nullptr);
                expect(! VoxlineState::migrateToCurrent(malformed)
                            .has_value(),
                       invalidValue);
            }

            const std::array<std::byte, 4> corruptBytes {
                std::byte {0x56}, std::byte {0x4f},
                std::byte {0x58}, std::byte {0x00}};
            expect(! VoxlineState::deserialise(
                        corruptBytes.data(),
                        static_cast<int>(corruptBytes.size()),
                        juce::Identifier("VOXLINEState"))
                        .has_value());

            const auto wrongBytes = asBinary(wrongRoot);
            expect(! VoxlineState::deserialise(
                        wrongBytes.getData(),
                        static_cast<int>(wrongBytes.getSize()),
                        juce::Identifier("VOXLINEState"))
                        .has_value());
        }

        beginTest("serialisation always writes schema version three");
        {
            juce::ValueTree state("VOXLINEState");
            state.setProperty("schemaVersion", 1, nullptr);
            juce::MemoryBlock bytes;
            VoxlineState::serialise(state, bytes);
            auto xml = juce::AudioProcessor::getXmlFromBinary(
                bytes.getData(), static_cast<int>(bytes.getSize()));
            expect(xml != nullptr);
            if (xml != nullptr)
                expectEquals(xml->getIntAttribute("schemaVersion"), 3);
        }
    }
};

StateMigrationTests stateMigrationTests;
} // namespace
