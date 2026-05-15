# VOXLINE — Fast Vocal Channel Strip

**版本** 1.0.0 | **開發者** ONETAKE | **架構** JUCE / C++20

---

## 概述

VOXLINE 是一款快速人聲效果鏈插件，專為 demo 製作、臥室製作人和創作者設計。核心工作流：

```
選 Preset → 調 POLISH → 微調 Tone → 完成
```

不用懂混音，不用掛一串插件。一個 VOXLINE 搞定 vocal 處理。

---

## 系統需求

| 平台 | 格式 | 最低 OS |
|---|---|---|
| macOS | VST3、AU（Universal） | macOS 12+ |
| Windows | VST3（x64） | Windows 10+ |

macOS 版同時支援 Apple Silicon（ARM64）和 Intel（x86_64），Rosetta 模式也可用。

---

## 安裝

### macOS

VST3 放到：
```
~/Library/Audio/Plug-Ins/VST3/VOXLINE.vst3
```

AU 放到：
```
~/Library/Audio/Plug-Ins/Components/VOXLINE.component
```

重開 DAW，重新掃描插件。

### Windows

解壓縮 `VOXLINE_Windows_VST3.zip`，把 `VOXLINE.vst3` 放到：
```
C:\Program Files\Common Files\VST3\
```

---

## 參數說明

| 旋鈕 | 範圍 | 功能 |
|---|---|---|
| **GAIN** | -24 ~ +24 dB | 輸入增益，控制進效果鏈前的音量 |
| **BODY** | 0–100% | 低中頻厚度。bell @ 200Hz，-4 到 +5 dB |
| **CLARITY** | 0–100% | 人聲靠前度、咬字清晰度。peak @ 3.5kHz，-3 到 +6 dB |
| **AIR** | 0–100% | 高頻空氣感、亮度。high shelf @ 7kHz，-3 到 +7 dB |
| **SMOOTH** | 0–100% | 降低刺耳感、齒音。high shelf cut @ 6kHz，0 到 -6 dB |
| **COMP** | 0–100% | 壓縮深度。threshold -12 到 -32 dB，ratio 1.2:1 到 5:1 |
| **DRIVE** | 0–100% | 飽和／髒感。tanh saturation，0 到 +12 dB 增益 |
| **POLISH** | 0–100% | 全局 macro。0.35x–1.35x 縮放所有 Tone/Comp/Drive 強度 |
| **OUT** | -24 ~ +24 dB | 輸出增益 |

| 按鈕 | 功能 |
|---|---|
| **Auto Gain** | 自動輸出補償。根據 COMP、DRIVE、POLISH 強度動態調整 OUT |
| **Bypass** | 旁路效果鏈（5ms 平滑過渡，無爆音） |
| **Listen** | 差異監聽：輸出 `(processed - dry) × 2`，讓你只聽到處理的差別 |
| **A/B** | 雙槽參數快照。按一下切換兩組設定，方便對比 |
| **☀/☽** | Light／Dark 主題切換（快速鍵 **T**） |

---

## 訊號鏈

```
Input
  → Smoothed Input Gain
  → Body EQ（bell 200Hz, -4/+5dB）
  → Clarity EQ（peak 3.5kHz, -3/+6dB）
  → Air EQ（high shelf 7kHz, -3/+7dB）
  → Smooth（high shelf cut 6kHz, 0/-6dB）
  → Compressor（one-knob, -12/-32dB thr, 1.2:1-5:1）
  → Drive（tanh saturation, 0-12dB）
  → Auto Gain compensation
  → Smoothed Output Gain
  → Bypass crossfade（5ms）
  → Soft clip protection
  → Output
```

---

## Preset 列表

| Preset | 風格 | 特色 |
|---|---|---|
| **Clean** | 中性 | 低處理量，自然透明 |
| **Basement Take** | 粗糙 demo | 厚、近、有一點髒 |
| **Dirty Lead** | 主 vocal | 貼臉、清楚、髒感、壓縮明顯 |
| **Cold Plug** | pluggnb | 薄、冷、亮、滑 |
| **Rage Cut** | rage rap | 硬、亮、攻擊性強 |
| **Muddy Trap** | 暗 trap | 厚、低中頻多、有態度 |
| **Cyber Vox** | digicore | 電子、亮、尖、前面 |
| **Noir Vocal** | 夜晚感 | 暗、霧、柔但不乾淨 |
| **Tape Rap** | 卡帶 | 暖、壓縮、類比髒感 |

---

## Meter 說明

| 元件 | 顯示 |
|---|---|
| **LED Dots**（Input panel） | 輸入訊號強度，7 段顯示 |
| **PEAK / RMS**（Output panel） | 輸出峰值和 RMS 值（dB） |
| **OUT Meter** | 輸出電平，含 peak hold 指示線 |
| **GR Meter** | 增益衰減量，壓縮器正在壓多少 |

---

## A/B 比較

1. 開啟時 A/B 兩個槽都從目前參數初始化
2. 調整參數會即時更新目前 active slot
3. 按 **A/B** 切換到另一個 slot
4. active slot 用粉紅色高亮顯示

---

## 快速鍵

| 鍵 | 功能 |
|---|---|
| **T** | 切換 Light / Dark 主題 |

---

## DAW 相容性

- Logic Pro（AU、VST3）
- FL Studio（VST3）
- Ableton Live（VST3、AU）
- Reaper（VST3、AU）
- Studio One（VST3、AU）
- Cubase（VST3）
- Bitwig（VST3、AU）

所有 DAW automation 參數皆可自動化。

---

## 技術規格

- **延遲**：0 samples（無延遲處理）
- **取樣率**：44.1 / 48 / 96 kHz
- **CPU**：低（無 oversampling、無 FFT）
- **介面**：1100×760 固定尺寸
