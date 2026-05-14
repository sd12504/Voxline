#include <JuceHeader.h>

#include "../Source/PluginProcessor.h"

namespace
{
class Phase1ParameterTests final : public juce::UnitTest
{
public:
    Phase1ParameterTests()
        : juce::UnitTest("VOXLINE Phase 1 parameters", "VOXLINE")
    {
    }

    void runTest() override
    {
        beginTest("processor exposes the full Phase 1 parameter set");

        {
            VoxlineAudioProcessor processor;
            expectEquals(processor.getParameters().size(), 12);
        }

        beginTest("processor state round-trips parameter values");

        {
            VoxlineAudioProcessor sourceProcessor;
            expectEquals(sourceProcessor.getParameters().size(), 12);

            auto* firstParam = sourceProcessor.getParameters()[0];
            firstParam->setValueNotifyingHost(1.0f);

            juce::MemoryBlock state;
            sourceProcessor.getStateInformation(state);

            expect(state.getSize() > 0);

            VoxlineAudioProcessor restoredProcessor;
            restoredProcessor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

            auto* restoredFirstParam = restoredProcessor.getParameters()[0];
            expectWithinAbsoluteError(restoredFirstParam->getValue(), 1.0f, 0.001f);
        }
    }
};

static Phase1ParameterTests phase1ParameterTests;
} // namespace

int main()
{
    juce::UnitTestRunner runner;
    runner.runAllTests();

    for (int i = 0; i < runner.getNumResults(); ++i)
        if (const auto* result = runner.getResult(i); result != nullptr && result->failures > 0)
            return 1;

    return 0;
}
