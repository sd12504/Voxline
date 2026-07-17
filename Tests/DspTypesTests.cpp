#include <JuceHeader.h>

#include "../Source/DSP/DspTypes.h"

namespace
{
class DspTypesTests final : public juce::UnitTest
{
public:
    DspTypesTests() : juce::UnitTest("DSP types", "VOXLINE") {}

    void runTest() override
    {
        beginTest("linear gain converts to bounded dBFS");
        expectWithinAbsoluteError(Voxline::Dsp::gainToDbfs(0.5f),
                                  -6.0206f, 0.001f);
        expectEquals(Voxline::Dsp::gainToDbfs(0.0f),
                     Voxline::Dsp::silenceFloorDbfs);

        beginTest("smoothing coefficient represents the requested sample time");
        const auto coefficient =
            Voxline::Dsp::smoothingCoefficient(48000.0, 100.0f);
        expectWithinAbsoluteError(std::pow(coefficient, 4800.0f),
                                  std::exp(-1.0f), 1.0e-4f);
    }
};

DspTypesTests dspTypesTests;
}
