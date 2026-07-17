#include <JuceHeader.h>

#include "../Source/Parameters/ParameterIDs.h"
#include "../Source/Parameters/ParameterLayout.h"
#include "../Source/PluginProcessor.h"

namespace
{
class ParameterContractTests final : public juce::UnitTest
{
public:
    ParameterContractTests()
        : juce::UnitTest("Parameter contract", "VOXLINE") {}

    void runTest() override
    {
        beginTest("stable IDs and extracted layout remain available");
        expectEquals(juce::String(VoxlineParameterIDs::inputGain),
                     juce::String("inputGain"));
        expectEquals(juce::String(VoxlineParameterIDs::polish),
                     juce::String("polish"));
        expectEquals(juce::String(VoxlineParameterIDs::spaceDucking),
                     juce::String("spaceDucking"));

        const auto layout = createVoxlineParameterLayout();
        juce::ignoreUnused(layout);

        VoxlineAudioProcessor processor;
        expectEquals(processor.getParameters().size(), 65);
    }
};

ParameterContractTests parameterContractTests;
}
