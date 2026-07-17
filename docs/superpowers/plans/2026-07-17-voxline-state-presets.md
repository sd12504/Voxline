# VOXLINE State, Parameters, A/B, and User Presets Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 建立 v3 參數與狀態契約、移除舊功能的聲音影響、保留舊 Session、持久化 A/B，並完成只含使用者 Preset 的資料庫。

**Architecture:** `ParameterRegistry` 是聲音／utility／monitor／retired 分類的唯一來源；`StateMigration` 負責 v1/v2→v3；A/B 與 User Preset 只複製 registry 標記的聲音參數。檔案 I/O 集中在 `UserPresetLibrary`，audio thread 不接觸任何狀態檔案。

**Tech Stack:** C++20、JUCE 8.0.12、APVTS／ValueTree、JUCE UnitTest、CMake／CTest。

## Global Constraints

- BODY／PRESENCE／AIR 與 Advanced LOW／PRES／AIR 共用 `body`／`clarity`／`air` Host IDs。
- LOW、MUD、PRES、AIR gain 為 −12.0…+12.0 dB，step 0.1，default 0。
- 舊 v2 的 −6…+6 dB 值載入 v3 時保持相同 dB，不因 range 擴大而倍增。
- `autoGain`、`cleanMode`、全域 `listen`、`mudAmount`、`lowGain`、`presGain`、`airGain` 是 retired tombstones：舊 state 可讀，新 DSP 不讀，新 UI/Preset/A/B 不保存。
- Band Solo 與 Listen S 是 transient monitor state，不是 APVTS parameter，不可保存或自動化。
- 不提供 factory presets；Host program API 合法地保留一個無行為的 `Default` program。
- User Preset 只保存聲音參數，不保存 A/B、Bypass、monitor、meter、clip hold、Advanced 開關或 UI 狀態。
- A/B 的兩個聲音快照與 active slot 保存於 Host session state，但不寫入 User Preset。
- state/preset 壞資料、錯 root、未知欄位不得破壞目前狀態。
- 不修改 DSP 演算法或 UI layout；Processor/Editor 接線在本計畫最後由單一 owner 執行。

---

### Task 1: Parameter registry and v3 active parameter layout

**Files:**
- Create: `Source/Parameters/ParameterRegistry.h`
- Create: `Source/Parameters/ParameterRegistry.cpp`
- Modify: `Source/Parameters/ParameterIDs.h`
- Modify: `Source/Parameters/ParameterLayout.cpp`
- Create: `Tests/ParameterRegistryTests.cpp`

**Produces:**

```cpp
namespace Voxline
{
enum class ParameterRole { sound, utility, retired };

struct ParameterSpec
{
    const char* id;
    ParameterRole role;
    bool saveInPreset;
    bool saveInAb;
};

std::span<const ParameterSpec> parameterRegistry() noexcept;
const ParameterSpec* findParameterSpec(juce::StringRef id) noexcept;
juce::ValueTree copyRegisteredSoundState(const juce::ValueTree&);
}
```

- [ ] **Step 1: Write RED contract tests**

Tests assert:

- `body`、`clarity`、`air`、`mudGain` ranges are −12…+12, interval 0.1, default 0.
- Main/Advanced EQ registry uses the same IDs; no second active LOW/PRES/AIR gain.
- New IDs exist:
  - `hpfEnabled`, `lowEnabled`, `mudEnabled`, `presEnabled`, `airEnabled`, `lpfEnabled`.
  - `compSensitivity`, `compMakeup`, `compAutoMakeup`.
  - `driveOutputTrim`, `driveLevelMatch`.
  - `spaceSize`, `spaceFeedback`, `spaceMonoSafety`.
- `spaceType` choices are Room／Plate／Hall／Slap／Width.
- Retired IDs are tagged retired and excluded by `copyRegisteredSoundState()`.
- Bypass is utility, excluded from User Preset and A/B.

- [ ] **Step 2: Verify RED**

Expected: compile FAIL on missing `ParameterRegistry.h` and new IDs.

- [ ] **Step 3: Add stable IDs**

Append these exact Host IDs without renaming existing active IDs:

```cpp
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
```

- [ ] **Step 4: Implement registry**

Registry rules:

- Every audible active parameter: `sound, true, true`.
- `bypass`: `utility, false, false`.
- `autoGain`, `cleanMode`, `listen`, `mudAmount`, `lowGain`, `presGain`, `airGain`: `retired, false, false`.
- Tombstones stay at their legacy ParameterLayout positions for Host/index compatibility and are never consumed by DSP/UI；new v3 parameter IDs are appended after every legacy ID.

- [ ] **Step 5: Update ParameterLayout**

Exact defaults:

- HPF/MUD/LPF enabled false; LOW/PRES/AIR true.
- Comp Sensitivity 0%, Makeup 0 dB, Auto Makeup true.
- Drive Output Trim 0 dB, Level Match true.
- Space Type Plate; Size 62%; Feedback 20%; Mono Safety true.
- `body`/`clarity`/`air`/`mudGain`: −12…+12 dB, 0.1 step, 0 default.
- Retired tombstones use their legacy types/ranges/defaults so old Host state can bind, but registry excludes them.

- [ ] **Step 6: Verify GREEN and commit**

Run Release tests and CTest. Commit only Parameters and registry test:

```bash
git commit -m "feat: define VOXLINE v3 parameter registry"
```

---

### Task 2: v1/v2/v3 state migration

**Files:**
- Create: `Source/State/StateMigration.h`
- Create: `Source/State/StateMigration.cpp`
- Modify: `Source/State/StateSchema.h`
- Modify: `Source/State/StateSchema.cpp`
- Create: `Tests/StateMigrationTests.cpp`
- Create: `Tests/Fixtures/State/v1-percent.xml`
- Create: `Tests/Fixtures/State/v2-minus6-plus6.xml`
- Create: `Tests/Fixtures/State/v2-duplicate-only.xml`
- Create: `Tests/Fixtures/State/v2-active-wins.xml`
- Create: `Tests/Fixtures/State/v3-unknown-fields.xml`

**Produces:**

```cpp
namespace VoxlineState
{
inline constexpr int currentSchemaVersion = 3;

std::optional<juce::ValueTree> migrateToCurrent(
    const juce::ValueTree& source);
juce::ValueTree migrateV1ToV3(juce::ValueTree);
juce::ValueTree migrateV2ToV3(juce::ValueTree);
}
```

- [ ] **Step 1: Write RED fixture tests**

- v1 without schema or schema=1: tone values 0/50/100 map to −12/0/+12 dB using `(value - 50) * 0.24`.
- v2 schema=2: −6/0/+6 remain −6/0/+6 dB.
- Duplicate-only: active value at its default and changed duplicate uses duplicate value, clamped to ±12.
- Both active and duplicate changed: active value wins.
- v3 values remain unchanged.
- Unknown v3 fields are ignored when applying APVTS but parsing succeeds.
- Future schema >3 and corrupt/wrong-root return `nullopt` and leave live state unchanged.
- Migrated result contains no saved retired values.

- [ ] **Step 2: Verify RED**

Expected: compile FAIL on missing migration API or fixture expectations.

- [ ] **Step 3: Implement migration**

Use ID-based ValueTree child lookup; never depend on parameter index. v2 range expansion preserves actual dB:

```cpp
const auto migratedDb = juce::jlimit(-12.0f, 12.0f, oldDb);
```

Duplicate precedence:

```cpp
if (activeWasChanged)
    result = activeValue;
else if (duplicateWasChanged)
    result = duplicateValue;
else
    result = 0.0f;
```

Remove retired children/properties after their values have been used for migration.

- [ ] **Step 4: Make StateSchema call migration**

`deserialise()` validates root, reads schema, calls `migrateToCurrent()`, and returns only a v3 tree. `serialise()` always writes schemaVersion 3.

- [ ] **Step 5: Verify GREEN and commit**

Run fixture suite and full CTest. Commit:

```bash
git commit -m "feat: migrate VOXLINE state to schema v3"
```

---

### Task 3: Processor-owned A/B and transient monitor state

**Files:**
- Create: `Source/State/ABState.h`
- Create: `Source/State/ABState.cpp`
- Create: `Source/State/MonitorState.h`
- Create: `Source/State/MonitorState.cpp`
- Create: `Tests/ABMonitorStateTests.cpp`

**Produces:**

```cpp
namespace VoxlineState
{
enum class AbSlot { a, b };

class AbStateManager
{
public:
    explicit AbStateManager(juce::AudioProcessorValueTreeState&);
    void initialiseFromCurrentSound();
    void captureActiveSlot();
    void select(AbSlot);
    AbSlot activeSlot() const noexcept;
    juce::ValueTree toValueTree() const;
    void restore(const juce::ValueTree&);
};

enum class MonitorMode { none, eqBandSolo, deEssListenS };

class MonitorState
{
public:
    void setEqBandSolo(int band) noexcept;
    void setDeEssListen(bool enabled) noexcept;
    void clear() noexcept;
    MonitorMode mode() const noexcept;
    int eqBand() const noexcept;
};
}
```

- [ ] **Step 1: Write RED tests**

- A and B independently round-trip every registry `saveInAb=true` parameter.
- Selecting a slot does not change Bypass, retired tombstones or monitor state.
- `toValueTree()` contains active slot plus two sound snapshots only.
- Restore from missing/corrupt A/B child safely initialises both from current sound.
- Monitor EQ band range is 0…5; invalid band clears.
- Monitor state has no serialisation API and does not appear in A/B ValueTree.

- [ ] **Step 2: Verify RED**

Expected: compile FAIL on missing classes.

- [ ] **Step 3: Implement**

Use `copyRegisteredSoundState()` for capture. Apply only registered sound IDs through parameters' `convertTo0to1()` and `setValueNotifyingHost()` so existing smoothers receive normal parameter changes.

- [ ] **Step 4: Verify GREEN and commit**

```bash
git commit -m "feat: persist VOXLINE A B sound snapshots"
```

---

### Task 4: User Preset library

**Files:**
- Create: `Source/State/UserPresetLibrary.h`
- Create: `Source/State/UserPresetLibrary.cpp`
- Create: `Tests/UserPresetLibraryTests.cpp`
- Create: `Tests/Fixtures/Presets/v1-percent.vxpreset`
- Create: `Tests/Fixtures/Presets/v3-unknown-fields.vxpreset`

**Produces:**

```cpp
namespace VoxlineState
{
class UserPresetLibrary
{
public:
    explicit UserPresetLibrary(juce::File rootDirectory);
    juce::Result initialise();
    juce::StringArray listNames() const;
    juce::Result saveAs(juce::String name,
                        const juce::ValueTree& soundState);
    juce::Result load(juce::StringRef name,
                      juce::ValueTree& destination) const;
    juce::Result rename(juce::StringRef from, juce::String to);
    juce::Result remove(juce::StringRef name);
};
}
```

- [ ] **Step 1: Write RED tests using `juce::TemporaryDirectory`**

- Empty list.
- Save As then exact sound-state round-trip.
- Names trim surrounding whitespace; empty, `Untitled`, separators, control characters and duplicate names reject.
- Sorted list uses case-insensitive natural order.
- Rename preserves payload and rejects duplicate target.
- Remove deletes only selected preset.
- Preset payload root `VOXLINEUserPreset`, formatVersion=3, contains registered sound state only.
- v1/v2 fixtures migrate through StateMigration.
- A/B, Bypass, monitor, meter, clip hold and UI fields never appear.
- Interrupted temp write never replaces an existing valid preset.

- [ ] **Step 2: Verify RED**

Expected: compile FAIL on missing library.

- [ ] **Step 3: Implement**

Production root:

```cpp
juce::File::getSpecialLocation(
    juce::File::userApplicationDataDirectory)
    .getChildFile("ONETAKE")
    .getChildFile("VOXLINE")
    .getChildFile("Presets");
```

Write to a sibling temporary file, flush, parse it back, then replace destination atomically. Extension is `.vxpreset`.

- [ ] **Step 4: Verify GREEN and commit**

```bash
git commit -m "feat: add VOXLINE user preset library"
```

---

### Task 5: Preset session controller

**Files:**
- Create: `Source/State/PresetSessionController.h`
- Create: `Source/State/PresetSessionController.cpp`
- Create: `Tests/PresetSessionControllerTests.cpp`

**Produces:**

```cpp
namespace VoxlineState
{
enum class UnsavedAction { save, discard, cancel };

struct PresetPresentation
{
    juce::String currentName {"Untitled"};
    bool edited {};
    juce::StringArray names;
};

class PresetSessionController
{
public:
    PresetSessionController(UserPresetLibrary&,
                            juce::AudioProcessorValueTreeState&,
                            MonitorState&);
    PresetPresentation presentation() const;
    juce::Result saveAs(juce::String);
    juce::Result renameCurrent(juce::String);
    juce::Result remove(juce::StringRef);
    juce::Result select(juce::StringRef, UnsavedAction);
    juce::Result selectRelative(int delta, UnsavedAction);
    bool isEdited() const;
    void onParameterChanged();
};
}
```

- [ ] **Step 1: Write RED tests**

- Initial presentation is `Untitled`, not edited, empty names.
- Changing sound parameter makes edited true; utility/retired/monitor does not.
- Successful Save As/Load resets edited.
- Select/relative with unsaved state obeys save/discard/cancel.
- Previous/next wraps through user list only.
- Every load, A/B change and close hook clears MonitorState.
- Rename/delete update presentation without affecting unrelated files.

- [ ] **Step 2: Verify RED**

Expected: compile FAIL on missing controller.

- [ ] **Step 3: Implement and verify**

Compare a canonical registered-sound snapshot against the loaded/saved baseline. Do not perform library I/O in parameter callbacks; callbacks only set an atomic/message-thread dirty flag.

- [ ] **Step 4: Commit**

```bash
git commit -m "feat: manage VOXLINE user preset sessions"
```

---

### Task 6: Serial Processor state and host-program integration

**Files:**
- Modify: `Source/PluginProcessor.h`
- Modify: `Source/PluginProcessor.cpp`
- Create: `Tests/HostStateIntegrationTests.cpp`

- [ ] **Step 1: Write RED integration tests**

- Host program API: count 1, current 0, name `Default`; any `setCurrentProgram()` call changes no sound parameter.
- Processor owns A/B; full Host state round-trip restores both slots and active slot.
- User Preset state does not contain A/B.
- v1/v2 Host fixtures migrate and produce the same audible active parameter values.
- Retired tombstone values cannot change rendered output.
- Corrupt/wrong-root/future state leaves live APVTS and A/B unchanged.

- [ ] **Step 2: Verify RED**

Expected: current 9 programs and Editor-owned A/B fail.

- [ ] **Step 3: Integrate**

Host state root contains:

```text
VOXLINEState schemaVersion=3
├── PARAMETERS
└── AB_STATE active="A"
    ├── SLOT_A
    └── SLOT_B
```

Keep APVTS child structure valid for JUCE. `getStateInformation()` captures active A/B slot then serialises. `setStateInformation()` decodes/migrates to temporaries, applies only after complete validation, restores A/B, and clears MonitorState.

Program methods return the single dummy program and never set parameter values.

- [ ] **Step 4: Verify and commit**

Run state/host tests, full CTest, VST3/AU builds. Commit:

```bash
git commit -m "refactor: integrate VOXLINE v3 host state"
```

---

### Task 7: State/Preset final verification

- [ ] Run all state, migration, A/B, library and controller tests.
- [ ] Verify all XML fixtures are committed and parse on macOS/Windows path rules.
- [ ] Confirm `rg "Basement Take|Dirty Lead|Cold Plug|Rage Cut|Muddy Trap|Cyber Vox|Noir Vocal|Tape Rap" Source` returns no factory program data.
- [ ] Confirm retired IDs occur only in ParameterIDs/Layout/Registry/Migration/tests, not active DSP or UI attachment code.
- [ ] Build Release Tests/VST3/AU and request whole-plan code review.

## Parallel Execution Batches

1. Task 1 serial registry contract.
2. Tasks 2–4 parallel.
3. Task 5 after Task 4; Task 3 remains independent.
4. Task 6 serial integration after Tasks 2–5.
5. Task 7 verification/review.
