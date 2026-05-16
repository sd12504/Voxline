# UI_V2_CENTER_POLISH_PLAN.md — VOXLINE V2 Centered Polish Interface

## 產品定位

**VOXLINE 2.0 — Complete Vocal Channel**

不再是 Fast Vocal Channel Strip。V2 定位為完整的桌面 vocal 處理器。

核心 workflow：
```
Preset → POLISH → Fine-tune EQ / Dynamics / Color / Space → Output
```

## 介面尺寸

```
Editor: 1400 × 900
V1: 1100 × 760
```

V2 放大以容納更多模組，同時給 POLISH 足夠的視覺空間。

---

## 佈局架構

整個 editor 不分四個 panel。POLISH 在正中央。其他模組環繞：

```
┌─────────────────────────────────────────────────────┐
│  Top Bar: Logo | Title | Preset ▼ | A/B | Listen | ☀│
├────┬──────────────────────────────────────┬─────────┤
│    │                                      │         │
│ IN │          POLISH (center)            │ OUT     │
│    │          ╭──────────╮               │ P/R     │
│ CL │          │   65 %   │               │ ▓▓▓▓▓▓ │
│    │          │ POLISH   │               │ ▓▓▓▓▓▓ │
│    │          ╰──────────╯               │ Gain    │
│    │                                      │         │
├────┴──────────────────────────────────────┴─────────┤
│  Vocal EQ: HPF | LOW | MUD | PRES | AIR | LPF      │
│  [ EQ curve display area ]                          │
├─────────────────────────────────────────────────────┤
│  Dynamics/Color: COMP ● | DRIVE ● | Soft Clip       │
│  SPACE: [Tight ▼] ━━━●━━━ 24%                       │
├─────────────────────────────────────────────────────┤
│              VOXLINE 2.0  |  SADTONY                 │
└─────────────────────────────────────────────────────┘
```

---

## 模組清單

### 1. Top Bar
```
[logo] VOXLINE  Complete Vocal Channel    [Preset ▼] [A/B] [Listen] [☀]
```
- Preset dropdown（現有邏輯保留）
- A/B 比較
- Listen
- Theme toggle

### 2. INPUT / CLEAN（左欄，垂直排列）
- GAIN knob（保留 inputGain APVTS ID）
- AUTO GAIN toggle
- Clean mode indicator
- Input LED dots

### 3. POLISH（中央，Hero）
- 大 hero knob，視覺最大
- Value: 65%
- Status 文字: Natural / Pushed / Intense
- 不影響 SPACE
- 影響: Clean, Vocal EQ macro, Dynamics, Color, Output comp

### 4. OUTPUT（右欄，垂直排列）
- PEAK / RMS 數字
- OUT meter（保留）
- GR meter（保留）
- Output Gain knob（保留 outputGain APVTS ID）

### 5. Vocal EQ（橫幅，POLISH 下方）
旋鈕排列: HPF | LOW | MUD | PRES | AIR | LPF
- 上方有 EQ curve 可視化顯示
- 第一版 curve 可以 visual-only placeholder
- 保留現有 body/clarity/air/smooth APVTS ID，映射到新 EQ 名

APVTS mapping:
| 新名稱 | 舊 APVTS ID |
|---|---|
| HPF | （新增）|
| LOW | body |
| MUD | （新增）|
| PRES | clarity |
| AIR | air |
| LPF | smooth |

### 6. Dynamics / Color
- COMP knob（保留 comp APVTS ID）
- DRIVE knob（保留 drive APVTS ID）
- Soft Clip indicator

### 7. SPACE
- Type dropdown: Tight / Slap / Wide
- Amount slider（保留 spaceAmount APVTS ID）
- 獨立於 POLISH

### 8. Footer
- VOXLINE 2.0 | SADTONY

---

## APVTS 規則

- 不刪除任何現有 parameter ID
- 保留所有 V1 參數以確保舊 session 相容
- 新 EQ 參數可以新增，舊的可 map 到新 UI
- body → LOW, clarity → PRES, air → AIR, smooth → LPF

---

## 不要做的事

- 不做 Room reverb
- 不做 BODY / CLARITY / AIR / SMOOTH 舊名顯示
- 不做四個 panel layout
- 不讓 SPACE 搶 POLISH 視覺
- 不做複雜 multi-page
- 不刪 APVTS ID
