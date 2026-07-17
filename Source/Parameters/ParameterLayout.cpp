#include "ParameterLayout.h"

#include "ParameterIDs.h"

namespace
{
juce::AudioParameterFloatAttributes makeDbAttributes()
{
    juce::AudioParameterFloatAttributes attributes;
    return attributes.withStringFromValueFunction([](float value, int)
                                                  { return juce::String(value, 1) + " dB"; });
}

juce::AudioParameterFloatAttributes makePercentAttributes()
{
    juce::AudioParameterFloatAttributes attributes;
    return attributes.withStringFromValueFunction([](float value, int)
                                                  { return juce::String(value, 0) + " %"; });
}
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout createVoxlineParameterLayout()
{
    auto params = std::vector<std::unique_ptr<juce::RangedAudioParameter>>{};
    const auto gainRange = juce::NormalisableRange<float>{-24.0f, 24.0f, 0.1f};
    const auto percentRange = juce::NormalisableRange<float>{0.0f, 100.0f, 1.0f};
    const auto toneGainRange = juce::NormalisableRange<float>{-12.0f, 12.0f, 0.1f};

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::inputGain, 1}, "Input Gain", gainRange, 0.0f, makeDbAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{VoxlineParameterIDs::autoGain, 1}, "Auto Gain", true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::polish, 1}, "Polish", percentRange, 65.0f, makePercentAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::body, 1}, "Body", toneGainRange, 0.0f, makeDbAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::clarity, 1}, "Presence", toneGainRange, 0.0f, makeDbAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::air, 1}, "Air", toneGainRange, 0.0f, makeDbAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::smooth, 1}, "Smooth", percentRange, 32.0f, makePercentAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::comp, 1}, "Comp", percentRange, 42.0f, makePercentAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::drive, 1}, "Drive", percentRange, 18.0f, makePercentAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::outputGain, 1}, "Output Gain", gainRange, 0.0f, makeDbAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{VoxlineParameterIDs::bypass, 1}, "Bypass", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{VoxlineParameterIDs::cleanMode, 1}, "Clean Mode", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{VoxlineParameterIDs::listen, 1}, "Listen", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::spaceAmount, 1}, "Space Amount", percentRange, 0.0f, makePercentAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{VoxlineParameterIDs::spaceType, 1}, "Space Type",
        juce::StringArray{"Room", "Plate", "Hall", "Slap", "Width"}, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::spaceTime, 1}, "Space Time",
        juce::NormalisableRange<float>{40.0f, 2000.0f, 1.0f, 0.42f}, 1200.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::spacePreDelay, 1}, "Space Pre-delay",
        juce::NormalisableRange<float>{0.0f, 120.0f, 1.0f}, 28.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::spaceWidth, 1}, "Space Width",
        juce::NormalisableRange<float>{0.0f, 200.0f, 1.0f}, 135.0f, makePercentAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::spaceTone, 1}, "Space Tone",
        juce::NormalisableRange<float>{-100.0f, 100.0f, 1.0f}, 12.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::spaceDecay, 1}, "Space Decay",
        juce::NormalisableRange<float>{0.1f, 2.5f, 0.01f, 0.55f}, 1.6f,
        juce::AudioParameterFloatAttributes().withLabel("s")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::spaceDucking, 1}, "Space Ducking",
        percentRange, 42.0f, makePercentAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::hpfFreq, 1}, "HPF Freq", juce::NormalisableRange<float>{20.0f, 300.0f, 1.0f}, 80.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{VoxlineParameterIDs::hpfSlope, 1}, "HPF Slope", juce::StringArray{"12", "24", "36", "48"}, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::mudAmount, 1}, "Mud Amount", percentRange, 0.0f, makePercentAttributes()));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{VoxlineParameterIDs::eqEnabled, 1}, "EQ Enabled", true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::lowFreq, 1}, "Low Freq", juce::NormalisableRange<float>{80.0f, 250.0f, 1.0f}, 160.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::lowGain, 1}, "Low Gain", juce::NormalisableRange<float>{-6.0f, 6.0f, 0.1f}, 1.5f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::lowQ, 1}, "Low Q", juce::NormalisableRange<float>{0.4f, 2.0f, 0.05f}, 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::mudFreq, 1}, "Mud Freq", juce::NormalisableRange<float>{200.0f, 700.0f, 1.0f}, 350.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::mudGain, 1}, "Mud Gain", toneGainRange, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::mudQ, 1}, "Mud Q", juce::NormalisableRange<float>{0.5f, 3.0f, 0.05f}, 1.1f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::presFreq, 1}, "Pres Freq", juce::NormalisableRange<float>{1000.0f, 5000.0f, 10.0f}, 2500.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::presGain, 1}, "Pres Gain", juce::NormalisableRange<float>{-3.0f, 6.0f, 0.1f}, 2.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::presQ, 1}, "Pres Q", juce::NormalisableRange<float>{0.5f, 3.0f, 0.05f}, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::airFreq, 1}, "Air Freq", juce::NormalisableRange<float>{6000.0f, 16000.0f, 100.0f}, 10000.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::airGain, 1}, "Air Gain", juce::NormalisableRange<float>{-3.0f, 6.0f, 0.1f}, 1.5f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::airQ, 1}, "Air Q", juce::NormalisableRange<float>{0.4f, 2.0f, 0.05f}, 0.7f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::lpfFreq, 1}, "LPF Freq", juce::NormalisableRange<float>{8000.0f, 20000.0f, 100.0f}, 18000.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{VoxlineParameterIDs::lpfSlope, 1}, "LPF Slope", juce::StringArray{"12", "24"}, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::compThreshold, 1}, "Comp Threshold",
        juce::NormalisableRange<float>{-40.0f, 0.0f, 0.1f}, -18.0f, makeDbAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::compRatio, 1}, "Comp Ratio",
        juce::NormalisableRange<float>{1.0f, 10.0f, 0.1f}, 3.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::compAttack, 1}, "Comp Attack",
        juce::NormalisableRange<float>{0.5f, 100.0f, 0.5f, 0.45f}, 15.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::compRelease, 1}, "Comp Release",
        juce::NormalisableRange<float>{20.0f, 500.0f, 1.0f, 0.45f}, 80.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::compMix, 1}, "Comp Mix", percentRange, 100.0f, makePercentAttributes()));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::deEssFreq, 1}, "De-Ess Frequency",
        juce::NormalisableRange<float>{3000.0f, 12000.0f, 10.0f, 0.45f}, 6500.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::deEssThreshold, 1}, "De-Ess Threshold",
        juce::NormalisableRange<float>{-40.0f, 0.0f, 0.1f}, -18.0f, makeDbAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::deEssRange, 1}, "De-Ess Range",
        juce::NormalisableRange<float>{0.0f, 12.0f, 0.1f}, 6.0f, makeDbAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{VoxlineParameterIDs::deEssMode, 1}, "De-Ess Mode",
        juce::StringArray{"Split", "Wide"}, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::driveTone, 1}, "Drive Tone",
        juce::NormalisableRange<float>{-100.0f, 100.0f, 1.0f}, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::driveMix, 1}, "Drive Mix", percentRange, 70.0f, makePercentAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{VoxlineParameterIDs::driveCharacter, 1}, "Drive Character",
        juce::StringArray{"Clean", "Warm", "Edge"}, 1));

    // v3 additions stay after all legacy parameters to preserve Host indices.
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{VoxlineParameterIDs::hpfEnabled, 1}, "HPF Enabled", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{VoxlineParameterIDs::lowEnabled, 1}, "Low Enabled", true));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{VoxlineParameterIDs::mudEnabled, 1}, "Mud Enabled", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{VoxlineParameterIDs::presEnabled, 1}, "Presence Enabled", true));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{VoxlineParameterIDs::airEnabled, 1}, "Air Enabled", true));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{VoxlineParameterIDs::lpfEnabled, 1}, "LPF Enabled", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::compSensitivity, 1}, "Comp Sensitivity",
        percentRange, 0.0f, makePercentAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::compMakeup, 1}, "Comp Makeup",
        juce::NormalisableRange<float>{-12.0f, 12.0f, 0.1f}, 0.0f, makeDbAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{VoxlineParameterIDs::compAutoMakeup, 1}, "Comp Auto Makeup", true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::driveOutputTrim, 1}, "Drive Output Trim",
        juce::NormalisableRange<float>{-12.0f, 12.0f, 0.1f}, 0.0f, makeDbAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{VoxlineParameterIDs::driveLevelMatch, 1}, "Drive Level Match", true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::spaceSize, 1}, "Space Size",
        percentRange, 62.0f, makePercentAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{VoxlineParameterIDs::spaceFeedback, 1}, "Space Feedback",
        percentRange, 20.0f, makePercentAttributes()));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{VoxlineParameterIDs::spaceMonoSafety, 1}, "Space Mono Safety", true));

    return {params.begin(), params.end()};
}
