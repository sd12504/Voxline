# VOXLINE Parallel-Ready Foundation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 建立不改變現有聲音與畫面的平行開發基礎，讓 DSP、UI、Preset/State 與測試工作不再同時爭用四個巨型檔案。

**Architecture:** 將參數 ID、ParameterLayout、版本化狀態與測試共用工具抽成單一責任檔案；CMake 自動收集 `Source/` 與 `Tests/` 下的新 C++ 檔，使後續 Sub Agent 可只擁有自己的模組檔。Processor 與 Editor 在本計畫維持現有行為，重構前後以單元測試與固定尺寸截圖確認沒有回歸。

**Tech Stack:** C++20、JUCE 8.0.12、CMake 3.22+、CTest、macOS AU/VST3。

## Global Constraints

- 目標處理延遲不高於約 1.5 ms；插件必須向 Host 正確回報實際延遲。
- 支援 44.1、48、88.2、96 kHz，mono 與 stereo。
- audio thread 不配置記憶體、不做檔案 I/O、不持有 UI lock。
- 收合尺寸 1080 × 720；Advanced 展開尺寸 1080 × 940。
- 僅維持深色主題。
- 本 Foundation 不改變聲音、參數數量、factory program 數量或 UI；後續 State/Preset 計畫才移除退役功能。
- 不修改或提交 `.superpowers/`、`design-qa.md`、`tools/`、`voxline-editable-figma.svg`。

---

## File Structure

本計畫建立：

- `Source/Parameters/ParameterIDs.h`：所有穩定 Host parameter ID。
- `Source/Parameters/ParameterLayout.h/.cpp`：唯一 ParameterLayout 建立入口。
- `Source/State/StateSchema.h/.cpp`：Host state 的版本標記、序列化與安全解析。
- `Tests/TestSupport.h/.cpp`：測試信號、參數設定、buffer 比較共用工具。
- `Tests/ProcessorContractTests.cpp`：參數、program、state、bypass 的基準契約。
- `Tests/EditorContractTests.cpp`：尺寸、控制存在性與 screenshot CLI。
- `Tests/TestMain.cpp`：唯一測試入口。

本計畫修改：

- `Source/PluginProcessor.h/.cpp`：改用抽出的 Parameter IDs/Layout/StateSchema，不改 DSP。
- `Tests/Phase1Tests.cpp`：移除；其案例搬到上述小型測試檔。
- `CMakeLists.txt`：集中宣告共用 source collection，避免後續 Agent 同時編輯兩份 source list。

---

### Task 1: Make source discovery parallel-safe

**Files:**
- Create: `Tests/SourceDiscoveryTests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `Source/**/*.cpp`, `Source/**/*.h`, `Tests/**/*.cpp`, `Tests/**/*.h`
- Produces: `VOXLINE_SHARED_SOURCES`, `VOXLINE_TEST_SOURCES`

- [ ] **Step 1: Add a test file that the old explicit source list cannot discover**

`Tests/SourceDiscoveryTests.cpp`：

```cpp
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
```

- [ ] **Step 2: Verify the old build does not discover the new test**

Run:

```bash
cmake --build build --config Release --target VOXLINEPhase1Tests --parallel
! build/VOXLINEPhase1Tests_artefacts/Release/VOXLINEPhase1Tests 2>&1 \
  | grep -q "Source discovery"
```

Expected: the shell expression succeeds because the current explicit CMake source list does not compile `SourceDiscoveryTests.cpp`.

- [ ] **Step 3: Replace duplicated explicit source lists with configured recursive collections**

在 `juce_add_plugin()` 後加入：

```cmake
file(GLOB_RECURSE VOXLINE_SHARED_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
)

file(GLOB_RECURSE VOXLINE_TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Tests/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Tests/*.h"
)
```

把 plugin 的 `target_sources(VOXLINE ...)` 改成：

```cmake
target_sources(VOXLINE PRIVATE ${VOXLINE_SHARED_SOURCES})
```

把 test target 的 source list 改成：

```cmake
target_sources(VOXLINEPhase1Tests
    PRIVATE
        ${VOXLINE_SHARED_SOURCES}
        ${VOXLINE_TEST_SOURCES}
)
```

- [ ] **Step 4: Reconfigure and verify the new test is discovered**

Run:

```bash
cmake -S . -B build
cmake --build build --config Release --target VOXLINEPhase1Tests --parallel
build/VOXLINEPhase1Tests_artefacts/Release/VOXLINEPhase1Tests 2>&1 \
  | grep -q "Source discovery"
ctest --test-dir build -C Release --output-on-failure
```

Expected: the executable output contains `Source discovery`, CMake configure succeeds, and 1/1 CTest passes.

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt Tests/SourceDiscoveryTests.cpp
git commit -m "build: make VOXLINE source discovery modular"
```

---

### Task 2: Extract the stable parameter contract

**Files:**
- Create: `Source/Parameters/ParameterIDs.h`
- Create: `Source/Parameters/ParameterLayout.h`
- Create: `Source/Parameters/ParameterLayout.cpp`
- Create: `Tests/ParameterContractTests.cpp`
- Modify: `Source/PluginProcessor.h`
- Modify: `Source/PluginProcessor.cpp`

**Interfaces:**
- Consumes: JUCE `AudioProcessorValueTreeState::ParameterLayout`
- Produces:
  - `namespace VoxlineParameterIDs`
  - `juce::AudioProcessorValueTreeState::ParameterLayout createVoxlineParameterLayout()`

- [ ] **Step 1: Add a failing test against the new free-function interface**

`Tests/ParameterContractTests.cpp`：

```cpp
#include <JuceHeader.h>
#include "../Source/Parameters/ParameterIDs.h"
#include "../Source/Parameters/ParameterLayout.h"

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

        auto layout = createVoxlineParameterLayout();
        juce::ignoreUnused(layout);
        expect(true);
    }
};

ParameterContractTests parameterContractTests;
}
```

- [ ] **Step 2: Run the test before extraction and confirm the new headers are missing**

Run:

```bash
cmake --build build --config Release --target VOXLINEPhase1Tests --parallel
```

Expected: build FAILS because `Source/Parameters/ParameterIDs.h` and `ParameterLayout.h` do not exist.

- [ ] **Step 3: Move parameter IDs into `ParameterIDs.h`**

`Source/Parameters/ParameterIDs.h` 必須包含目前 `PluginProcessor.h` 中完整的 `VoxlineParameterIDs` namespace，保留每個字串不變：

```cpp
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
}
```

這一步的完整實作必須逐項搬移現有 51 個 ID，不新增、不刪除、不重新命名。

- [ ] **Step 4: Move `createParameterLayout()` into a free function**

`Source/Parameters/ParameterLayout.h`：

```cpp
#pragma once

#include <JuceHeader.h>

juce::AudioProcessorValueTreeState::ParameterLayout createVoxlineParameterLayout();
```

`Source/Parameters/ParameterLayout.cpp` 搬移原本 `PluginProcessor.cpp` 中 `makeDbAttributes()`、`makePercentAttributes()` 與 `VoxlineAudioProcessor::createParameterLayout()` 的完整函式體。只把最後一個函式的限定名稱改為：

```cpp
juce::AudioProcessorValueTreeState::ParameterLayout createVoxlineParameterLayout()
```

搬移範圍從 `juce::AudioParameterFloatAttributes makeDbAttributes()` 起，到原函式的 `return {params.begin(), params.end()};` 及結尾大括號止。`percentToUnit()` 留在 Processor。原本 51 個參數的 range、default、version、label 與順序不得改動。

- [ ] **Step 5: Wire Processor to the extracted contract**

`PluginProcessor.h`：

```cpp
#include "Parameters/ParameterIDs.h"

// Remove the in-header VoxlineParameterIDs namespace.
// Remove the static createParameterLayout declaration.
```

`PluginProcessor.cpp`：

```cpp
#include "Parameters/ParameterLayout.h"

VoxlineAudioProcessor::VoxlineAudioProcessor()
    : AudioProcessor(/* keep the existing BusesProperties expression */),
      apvts(*this, nullptr, "VOXLINEState", createVoxlineParameterLayout())
{
}
```

刪除舊的 `VoxlineAudioProcessor::createParameterLayout()` 定義，不改其他 DSP。

- [ ] **Step 6: Verify exact parameter compatibility**

Run:

```bash
cmake --build build --config Release --target VOXLINEPhase1Tests --parallel
ctest --test-dir build -C Release --output-on-failure
```

Expected: 1/1 passes; test still reports 51 parameters, Body range −6…+6 dB, 9 programs.

- [ ] **Step 7: Commit**

```bash
git add Source/Parameters Source/PluginProcessor.h Source/PluginProcessor.cpp \
  Tests/ParameterContractTests.cpp
git commit -m "refactor: extract VOXLINE parameter contract"
```

---

### Task 3: Add a versioned state envelope without changing current restores

**Files:**
- Create: `Source/State/StateSchema.h`
- Create: `Source/State/StateSchema.cpp`
- Modify: `Source/PluginProcessor.cpp`
- Test: `Tests/Phase1Tests.cpp`

**Interfaces:**
- Consumes: APVTS root type `VOXLINEState`
- Produces:
  - `VoxlineState::currentSchemaVersion`
  - `VoxlineState::serialise(const juce::ValueTree&, juce::MemoryBlock&)`
  - `VoxlineState::deserialise(const void*, int, juce::Identifier)`

- [ ] **Step 1: Write failing state-schema tests**

在 `Tests/Phase1Tests.cpp` 的 state 測試後加入：

```cpp
beginTest("state schema rejects corrupt and wrong-root data");
{
    VoxlineAudioProcessor processor;
    const auto before = processor.getAPVTS().copyState();

    const std::array<std::byte, 4> corrupt {
        std::byte{0x56}, std::byte{0x4f}, std::byte{0x58}, std::byte{0x00}
    };
    processor.setStateInformation(corrupt.data(), static_cast<int>(corrupt.size()));
    expect(processor.getAPVTS().copyState().isEquivalentTo(before));

    juce::ValueTree wrong("WrongRoot");
    std::unique_ptr<juce::XmlElement> xml(wrong.createXml());
    juce::MemoryBlock bytes;
    juce::AudioProcessor::copyXmlToBinary(*xml, bytes);
    processor.setStateInformation(bytes.getData(), static_cast<int>(bytes.getSize()));
    expect(processor.getAPVTS().copyState().isEquivalentTo(before));
}

beginTest("new state writes a schema version");
{
    VoxlineAudioProcessor processor;
    juce::MemoryBlock bytes;
    processor.getStateInformation(bytes);
    auto xml = juce::AudioProcessor::getXmlFromBinary(bytes.getData(),
        static_cast<int>(bytes.getSize()));
    expect(xml != nullptr);
    expect(xml->hasAttribute("schemaVersion"));
    expectEquals(xml->getIntAttribute("schemaVersion"), 2);
}
```

- [ ] **Step 2: Run to verify the version test fails**

Run:

```bash
cmake --build build --config Release --target VOXLINEPhase1Tests --parallel
ctest --test-dir build -C Release --output-on-failure
```

Expected: FAIL at `hasAttribute("schemaVersion")`.

- [ ] **Step 3: Implement `StateSchema`**

`Source/State/StateSchema.h`：

```cpp
#pragma once

#include <JuceHeader.h>
#include <optional>

namespace VoxlineState
{
inline constexpr int currentSchemaVersion = 2;

void serialise(const juce::ValueTree& state, juce::MemoryBlock& destination);
std::optional<juce::ValueTree> deserialise(const void* data,
                                           int sizeInBytes,
                                           juce::Identifier expectedRoot);
}
```

`Source/State/StateSchema.cpp`：

```cpp
#include "StateSchema.h"

void VoxlineState::serialise(const juce::ValueTree& state,
                             juce::MemoryBlock& destination)
{
    auto versioned = state.createCopy();
    versioned.setProperty("schemaVersion", currentSchemaVersion, nullptr);
    if (auto xml = versioned.createXml())
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

    return state;
}
```

- [ ] **Step 4: Use the schema in Processor**

`PluginProcessor.cpp`：

```cpp
#include "State/StateSchema.h"

void VoxlineAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    VoxlineState::serialise(apvts.copyState(), destData);
}

void VoxlineAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto state = VoxlineState::deserialise(
            data, sizeInBytes, apvts.state.getType()))
        apvts.replaceState(*state);
}
```

- [ ] **Step 5: Verify current and legacy unversioned state**

保留原 round-trip 測試，並新增一個沒有 `schemaVersion` 的 `VOXLINEState` XML fixture，確認仍能載入。Run:

```bash
cmake --build build --config Release --target VOXLINEPhase1Tests --parallel
ctest --test-dir build -C Release --output-on-failure
```

Expected: all tests pass; corrupt/wrong-root leave current state unchanged; new state contains schemaVersion=2.

- [ ] **Step 6: Commit**

```bash
git add Source/State Source/PluginProcessor.cpp Tests/Phase1Tests.cpp
git commit -m "feat: version VOXLINE processor state"
```

---

### Task 4: Split tests by responsibility

**Files:**
- Create: `Tests/TestSupport.h`
- Create: `Tests/TestSupport.cpp`
- Create: `Tests/ProcessorContractTests.cpp`
- Create: `Tests/EditorContractTests.cpp`
- Create: `Tests/DspRegressionTests.cpp`
- Create: `Tests/TestMain.cpp`
- Delete: `Tests/Phase1Tests.cpp`

**Interfaces:**
- Produces:
  - `VoxlineTest::fillTestSignal(juce::AudioBuffer<float>&, double)`
  - `VoxlineTest::averageAbsoluteDifference(const juce::AudioBuffer<float>&, const juce::AudioBuffer<float>&)`
  - `VoxlineTest::setFloatParameter(VoxlineAudioProcessor&, const juce::String&, float)`
  - `VoxlineTest::setBoolParameter(VoxlineAudioProcessor&, const juce::String&, bool)`
  - `VoxlineTest::renderEditorScreenshot(const juce::String&, bool)`

- [ ] **Step 1: Move shared helpers without behavior changes**

`Tests/TestSupport.h`：

```cpp
#pragma once

#include <JuceHeader.h>

class VoxlineAudioProcessor;

namespace VoxlineTest
{
void fillTestSignal(juce::AudioBuffer<float>& buffer, double sampleRate);
float averageAbsoluteDifference(const juce::AudioBuffer<float>& a,
                                const juce::AudioBuffer<float>& b);
void setFloatParameter(VoxlineAudioProcessor& processor,
                       const juce::String& parameterID,
                       float plainValue);
void setBoolParameter(VoxlineAudioProcessor& processor,
                      const juce::String& parameterID,
                      bool enabled);
int renderEditorScreenshot(const juce::String& outputPath, bool showAdvanced);
}
```

`TestSupport.cpp` 逐字搬移原 `Phase1Tests.cpp` 中 `fillTestSignal`、`averageAbsoluteDifference`、`setFloatParameter`、`setBoolParameter` 與 `renderEditorScreenshot` 五個函式體，只加入 `VoxlineTest::` namespace；不改公式與 screenshot 行為。

- [ ] **Step 2: Move each existing test into one registered JUCE UnitTest**

- `ProcessorContractTests.cpp`：parameter count/ranges/programs、state round-trip、corrupt state、bypass。
- `EditorContractTests.cpp`：JSON layout、editor 1080×940/720、control counts/bounds。
- `DspRegressionTests.cpp`：default DSP change、extreme output containment、de-ess behavior。

每檔使用此模式註冊；`runTest()` 的內容是下方指定的原始 `beginTest` 區塊，不省略 assertion：

```cpp
namespace
{
class ProcessorContractTests final : public juce::UnitTest
{
public:
    ProcessorContractTests()
        : juce::UnitTest("Processor contract", "VOXLINE") {}

    void runTest() override
    {
        beginTest("processor exposes the full parameter set");
        {
            VoxlineAudioProcessor processor;
            expectEquals(processor.getParameters().size(), 51);
            expectEquals(processor.getNumPrograms(), 9);
            expectEquals(processor.getProgramName(0), juce::String("Clean"));
        }
    }
};

ProcessorContractTests processorContractTests;
}
```

上方顯示註冊與第一個區塊的確切形狀。實際搬移時，所有既有 assertion 必須原樣保留，只允許因 namespace 加上 `VoxlineTest::`；不能以精簡版三個 assertion 取代原完整區塊。

- [ ] **Step 3: Add the single test entry point**

`Tests/TestMain.cpp`：

```cpp
#include <JuceHeader.h>
#include "TestSupport.h"

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI guiScope;

    if (argc >= 2
        && (juce::String(argv[1]) == "--render-editor"
            || juce::String(argv[1]) == "--render-editor-advanced"))
    {
        return VoxlineTest::renderEditorScreenshot(
            argc >= 3 ? juce::String(argv[2]) : juce::String(),
            juce::String(argv[1]) == "--render-editor-advanced");
    }

    juce::UnitTestRunner runner;
    runner.runAllTests();

    for (int i = 0; i < runner.getNumResults(); ++i)
        if (const auto* result = runner.getResult(i);
            result != nullptr && result->failures > 0)
            return 1;

    return 0;
}
```

- [ ] **Step 4: Build and run all moved tests**

Run:

```bash
cmake --build build --config Release --target VOXLINEPhase1Tests --parallel
ctest --test-dir build -C Release --output-on-failure
```

Expected: 1/1 CTest passes and the executable output lists Processor contract, Editor contract, and DSP regression suites.

- [ ] **Step 5: Verify screenshot CLI remains functional**

Run:

```bash
build/VOXLINEPhase1Tests_artefacts/Release/VOXLINEPhase1Tests \
  --render-editor /tmp/voxline-foundation-main.png
build/VOXLINEPhase1Tests_artefacts/Release/VOXLINEPhase1Tests \
  --render-editor-advanced /tmp/voxline-foundation-advanced.png
sips -g pixelWidth -g pixelHeight /tmp/voxline-foundation-main.png \
  /tmp/voxline-foundation-advanced.png
```

Expected:

- main image: 1080 × 940 because the current Editor opens Advanced by default.
- advanced image: 1080 × 940.
- both PNG files are readable.

- [ ] **Step 6: Commit**

```bash
git add Tests
git commit -m "test: split VOXLINE contracts by subsystem"
```

---

### Task 5: Foundation verification gate

**Files:**
- Modify only if a verification failure identifies a foundation regression.

**Interfaces:**
- Produces: verified parallel-ready baseline for downstream DSP/UI/State agents.

- [ ] **Step 1: Run clean test and plugin builds**

Run:

```bash
cmake -S . -B build
cmake --build build --config Release \
  --target VOXLINEPhase1Tests VOXLINE_VST3 VOXLINE_AU --parallel
ctest --test-dir build -C Release --output-on-failure
```

Expected: all three targets build and 1/1 CTest passes.

- [ ] **Step 2: Verify no accidental scope**

Run:

```bash
git status --short
git diff --check HEAD~4..HEAD
git log --oneline -5
```

Expected:

- only Foundation files changed in its commits.
- no whitespace errors.
- user-owned untracked files remain unmodified and uncommitted.

- [ ] **Step 3: Record the downstream ownership rule**

Downstream agents must follow:

- EQ、DeEss、Compressor、Polish、Drive、Space 與 Meter agent 分別只擁有自己名稱對應的 `Source/DSP/` 檔與 `Tests/` 測試檔。
- State/Preset agent owns only `Source/State/**`, `Source/Parameters/**`, and `Tests/StatePresetTests.cpp`.
- UI extraction agent owns `PluginEditor.*` until it creates separate component files.
- No module agent edits `PluginProcessor.*`, `PluginEditor.*`, `CMakeLists.txt`, or another agent's test file.
- Processor/Editor integration is a separate serial task after parallel module work.

- [ ] **Step 4: Commit verification-only fixes if needed**

If and only if Step 1 or 2 required a code correction:

```bash
git add CMakeLists.txt Source/Parameters Source/State \
  Source/PluginProcessor.cpp Source/PluginProcessor.h Tests
git commit -m "fix: stabilize VOXLINE parallel foundation"
```

Expected: no commit when verification passes without correction.
