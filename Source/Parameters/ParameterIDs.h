#pragma once

namespace VoxlineParameterIDs
{
inline constexpr auto inputGain = "inputGain";
inline constexpr auto autoGain = "autoGain";
inline constexpr auto polish = "polish";
inline constexpr auto body = "body";
inline constexpr auto clarity = "clarity";
inline constexpr auto air = "air";
inline constexpr auto smooth = "smooth";
inline constexpr auto comp = "comp";
inline constexpr auto drive = "drive";
inline constexpr auto outputGain = "outputGain";
inline constexpr auto bypass = "bypass";
inline constexpr auto cleanMode = "cleanMode";
inline constexpr auto listen = "listen";
inline constexpr auto spaceAmount = "spaceAmount";
inline constexpr auto spaceType = "spaceType";
inline constexpr auto spaceTime = "spaceTime";
inline constexpr auto spacePreDelay = "spacePreDelay";
inline constexpr auto spaceWidth = "spaceWidth";
inline constexpr auto spaceTone = "spaceTone";
inline constexpr auto spaceDecay = "spaceDecay";
inline constexpr auto spaceDucking = "spaceDucking";
inline constexpr auto hpfFreq = "hpfFreq";
inline constexpr auto mudAmount = "mudAmount";
inline constexpr auto eqEnabled = "eqEnabled";
inline constexpr auto hpfSlope = "hpfSlope";
inline constexpr auto lowFreq = "lowFreq";
inline constexpr auto lowGain = "lowGain";
inline constexpr auto lowQ = "lowQ";
inline constexpr auto mudFreq = "mudFreq";
inline constexpr auto mudGain = "mudGain";
inline constexpr auto mudQ = "mudQ";
inline constexpr auto presFreq = "presFreq";
inline constexpr auto presGain = "presGain";
inline constexpr auto presQ = "presQ";
inline constexpr auto airFreq = "airFreq";
inline constexpr auto airGain = "airGain";
inline constexpr auto airQ = "airQ";
inline constexpr auto lpfFreq = "lpfFreq";
inline constexpr auto lpfSlope = "lpfSlope";
inline constexpr auto compThreshold = "compThreshold";
inline constexpr auto compRatio = "compRatio";
inline constexpr auto compAttack = "compAttack";
inline constexpr auto compRelease = "compRelease";
inline constexpr auto compMix = "compMix";
inline constexpr auto deEssFreq = "deEssFreq";
inline constexpr auto deEssThreshold = "deEssThreshold";
inline constexpr auto deEssRange = "deEssRange";
inline constexpr auto deEssMode = "deEssMode";
inline constexpr auto driveTone = "driveTone";
inline constexpr auto driveMix = "driveMix";
inline constexpr auto driveCharacter = "driveCharacter";

// v3 parameters are deliberately appended after every legacy Host ID.
inline constexpr auto hpfEnabled = "hpfEnabled";
inline constexpr auto lowEnabled = "lowEnabled";
inline constexpr auto mudEnabled = "mudEnabled";
inline constexpr auto presEnabled = "presEnabled";
inline constexpr auto airEnabled = "airEnabled";
inline constexpr auto lpfEnabled = "lpfEnabled";
inline constexpr auto compSensitivity = "compSensitivity";
inline constexpr auto compMakeup = "compMakeup";
inline constexpr auto compAutoMakeup = "compAutoMakeup";
inline constexpr auto driveOutputTrim = "driveOutputTrim";
inline constexpr auto driveLevelMatch = "driveLevelMatch";
inline constexpr auto spaceSize = "spaceSize";
inline constexpr auto spaceFeedback = "spaceFeedback";
inline constexpr auto spaceMonoSafety = "spaceMonoSafety";
} // namespace VoxlineParameterIDs
