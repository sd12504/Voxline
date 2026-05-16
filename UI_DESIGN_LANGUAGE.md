# UI_DESIGN_LANGUAGE.md — VOXLINE V2 Design Language

## 1. 核心原則

- **POLISH 永遠是主角**：最大、最亮、最中央
- **一頁式**：不 scroll、不分頁、不 tab
- **Creator-friendly**：不需要懂混音就能用
- **Dark/Light 雙主題**：所有 token 都有兩套值

## 2. 色彩系統

### Light Theme
| Token | Hex | 用途 |
|---|---|---|
| editorBg | #F3EFE8 | Editor 背景 |
| mainCardBg | #FDFBF8 | 主卡片 |
| panelBg | #F7F0E7 | 面板填充 |
| panelBorder | #DDD2C4 | 面板邊框 |
| textPrimary | #26222B | 主文字 |
| textSecondary | #6B6576 | 副文字 |
| textMuted | #A09AA8 | 弱文字 |
| accentRose | #D86F96 | 主 accent |
| accentLavender | #B8A6F3 | 副 accent |
| inactiveArc | #D8CFC4 | 未啟動 arc |

### Dark Theme
| Token | Hex | 用途 |
|---|---|---|
| editorBg | #09080D | Editor 背景 |
| mainCardBg | #100F17 | 主卡片 |
| panelBg | #181622 | 面板填充 |
| panelBorder | #2A2635 | 面板邊框 |
| textPrimary | #F5F0EA | 主文字 |
| textSecondary | #9D99A8 | 副文字 |
| textMuted | #5E5B66 | 弱文字 |
| accentRose | #E98BAA | 主 accent |
| accentLavender | #C7B7FF | 副 accent |
| inactiveArc | #2E2B38 | 未啟動 arc |

## 3. 字型

- 標題：24px bold
- 面板標題：12px bold
- Knob label：11px bold
- Knob value：12.5px bold
- POLISH value：28px bold
- Footer：10px
- 使用 JUCE 預設 sans-serif（系統字型）

## 4. Knob 規格

### Hero（POLISH）
- 直徑：約 190px
- arc inset：10px
- arc 寬度：7px
- pointer 長度：58% of radius
- 陰影：6px drop
- 暗色主題：radial glow

### Standard
- 直徑：約 60–70px
- arc inset：10px
- arc 寬度：5px
- pointer 長度：52% of radius
- 陰影：4px drop

### Compact
- 直徑：約 40px
- arc inset：8px
- arc 寬度：3.5px
- pointer 長度：48% of radius

## 5. 間距

- Panel 之間：16–20px
- Knob 之間水平：24px
- Label 到 knob：8px
- Knob 到 value：6px
- 面板 corner radius：22px
- 按鈕 corner radius：8px

## 6. POLISH 特殊規則

- POLISH 永遠在畫面中央
- POLISH 的 accent 色是 rose
- POLISH 暗色模式有 glow
- POLISH 不顯示內部 label（面板標題已說 POLISH）
- POLISH value 顯示在 knob 下方

## 7. SPACE 規則

- SPACE 不跟 POLISH 共用區域
- SPACE 有自己的 compact 控制
- SPACE accent 色不直接使用 rose（避免混淆）
- SPACE 在底部 bar 區域

## 8. EQ Curve Display

- 顯示頻率響應的可視化預覽
- 基於目前 LOW / MUD / PRES / AIR / HPF / LPF 設定
- 使用簡單的路徑繪製
- 顏色：使用 accent 色系但降低飽和度
- V1 可以先 visual-only placeholder

## 9. 動畫

- V1 不做動畫
- 控制在 V2 後期才考慮 smooth transition
- Meter 更新本來就是即時的（不需要額外動畫）

## 10. 禁止的設計模式

- 不使用 tab 分頁
- 不使用 scroll view
- 不使用 popup / modal
- 不使用 collapse / expand 面板
- 不使用 PNG 背景
- 不使用 screenshot UI
- 不使用 default gray JUCE 外觀
- 不使用藍色（blue）
- 不使用超過 3 種 accent 顏色
