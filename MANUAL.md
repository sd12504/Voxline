# VOXLINE — Complete Vocal Processing Plugin

**版本** 1.0.0 | **開發者** ONETAKE / SADTONY | **架構** JUCE / C++20

---

## 概述

VOXLINE 是一款完整人聲處理插件，讓創作者快速完成可用、乾淨、有風格的人聲鏈。核心工作流：

```
選 Preset → 調 POLISH → 微調 Tone → 完成
```

不用懂混音，不用掛一串插件。一個 VOXLINE 搞定 vocal 處理。

---

## V2 介面總覽

VOXLINE V2 主畫面為 1080×720；展開 Advanced 後高度為 940。主畫面只保留快速完成一條人聲鏈所需的控制：

- **INPUT**：輸入增益、輸入電平與 Auto Gain。
- **POLISH**：中央 hero macro，控制人聲核心處理強度，狀態會顯示 **Natural / Pushed / Intense**。
- **OUTPUT**：Peak、RMS、輸出電平、GR 與 Output Gain。
- **快速控制列**：Body、Presence、Air、De-ess、Comp、Drive、Space。Body、Presence、Air 會顯示 -6／0／+6 的雙向刻度。
- **ADVANCED**：可展開 Vocal EQ、Comp、De-ess、Drive 的完整設定。

---

## 系統需求

| 平台 | 格式 | 最低 OS |
|---|---|---|
| macOS | VST3、AU（Intel x86_64／Universal） | macOS 11+ |
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

解壓縮 `VOXLINE-1.0.0-Windows-x64-VST3.zip`，把 `VOXLINE.vst3` 放到：
```
C:\Program Files\Common Files\VST3\
```

---

## 參數說明

| 控制 | 範圍 / 選項 | 功能 |
|---|---|---|
| **INPUT GAIN** | -24 ~ +24 dB | 控制進入效果鏈前的人聲音量 |
| **BODY** | -6 ~ +6 dB | 以 0 dB 為中心增減人聲厚度 |
| **PRESENCE** | -6 ~ +6 dB | 以 0 dB 為中心增減咬字與前進感 |
| **AIR** | -6 ~ +6 dB | 以 0 dB 為中心增減高頻空氣感 |
| **DE-ESS** | 0–100% | 控制齒音抑制量 |
| **POLISH** | 0–100% | 中央 macro。0–33% 顯示 Natural，34–66% 顯示 Pushed，67–100% 顯示 Intense |
| **COMP** | 0–100% | 壓縮深度 |
| **DRIVE** | 0–100% | 飽和／髒感 |
| **SPACE AMOUNT** | 0–100% | 控制空間效果量，獨立於 POLISH |
| **SPACE TYPE** | Tight Ambience / Filtered Slap / Stereo Wide | 選擇短 ambience、濾波 slap 或立體聲加寬空間 |
| **OUTPUT GAIN** | -24 ~ +24 dB | 控制最終輸出音量 |

| 按鈕 | 功能 |
|---|---|
| **Auto Gain** | 自動輸出補償，根據處理強度協助穩定輸出音量 |
| **Bypass** | 旁路效果鏈（5ms 平滑過渡，無爆音） |
| **Listen** | 差異監聽：輸出 `(processed - dry) × 2`，讓你只聽到處理的差別 |
| **A/B** | 雙槽參數快照。按一下切換兩組設定，方便對比 |

---

## Vocal EQ

V2 的 **VOCAL EQ** 面板提供 6 段人聲 EQ。曲線依照實際濾波器係數計算，並疊加即時輸出頻譜；節點、旋鈕與可輸入的數值欄位會同步更新。

| Band | 類型 | 用途 |
|---|---|---|
| **HPF** | High-pass | 清掉低頻 rumble，可調頻率與斜率 |
| **LOW** | Bell | 控制厚度與胸腔感 |
| **MUD** | Bell | 削減混濁、箱體感 |
| **PRES** | Bell | 增加存在感與咬字 |
| **AIR** | Shelf | 增加空氣感與亮度 |
| **LPF** | Low-pass | 收斂過亮高頻，可調頻率與斜率 |

點選 HPF / LOW / MUD / PRES / AIR / LPF 按鈕會切換目前選中的 band。下方 FREQ 與 GAIN/SLOPE 區域會顯示該 band 的目前數值；曲線上的亮點會跟著參數變動更新。

主畫面的 BODY、PRESENCE、AIR 與 Advanced EQ 共用同一組 gain，因此兩邊永遠同步。Bell band 另可調 Q；HPF/LPF 則使用 slope。

## Advanced Dynamics / De-ess / Drive

- **COMP**：Threshold、Ratio、Attack、Release、Mix，五個參數完整分欄顯示。
- **DE-ESS**：Frequency、Threshold、最大 Range、Split/Wide 模式，以及即時 reduction 讀值。
- **DRIVE**：Tone、Mix，以及 Clean/Warm/Edge Character。

主畫面的旋鈕控制「處理量」，Advanced 控制「處理方式」。所有 Advanced 參數都支援 DAW automation、狀態保存與 A/B 快照。

---

## 訊號鏈

```
Input
  → Smoothed Input Gain
  → Split-band De-ess
  → Vocal EQ（HPF / LOW / MUD / PRES / AIR / LPF）
  → POLISH macro scaling for vocal core processing
  → Dynamics / Color（Compression、Drive）
  → SPACE（Tight Ambience / Filtered Slap / Stereo Wide）
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
- **CPU**：低；即時頻譜使用 2048-point FFT
- **介面**：1080×720，展開 Advanced 為 1080×940
- **主題**：深色模式
