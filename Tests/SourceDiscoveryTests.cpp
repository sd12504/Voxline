#include <JuceHeader.h>

namespace
{
class SourceDiscoveryTests final : public juce::UnitTest
{
public:
    SourceDiscoveryTests()
        : juce::UnitTest("Source discovery", "VOXLINE") {}

    void runTest() override
    {
        beginTest("recursive test source is compiled");
        expect(true);
    }
};

SourceDiscoveryTests sourceDiscoveryTests;
}
