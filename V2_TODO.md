# V2_TODO.md — VOXLINE V2 Roadmap

## Stage 0 — 規劃文件 ✅
- [x] UI_V2_CENTER_POLISH_PLAN.md
- [x] UI_DESIGN_LANGUAGE.md
- [x] V2_TODO.md

**Commit**: docs: plan VOXLINE V2 centered polish interface

---

## Stage 1 — 1400×900 Layout Shell
1. 改 editor 尺寸為 1400×900
2. 移除四個 panel (inputPanel, tonePanel, polishPanel, outputPanel)
3. 建立 V2 佈局架構：Top Bar / Center POLISH / Left Input / Right Output / Bottom EQ / Bottom Bar
4. 不使用現有 Layout.h 的 panel 座標
5. 先放 placeholder rects 確認佈局

**Commit**: ui: create V2 1400x900 layout shell

---

## Stage 2 — 搬移現有 Controls
1. POLISH hero knob → 中央
2. Input Gain + Auto Gain → 左欄
3. Output Gain + Meter → 右欄
4. Comp + Drive → Dynamics/Color 區
5. SPACE control → 底部 bar
6. Preset dropdown / A/B / Listen / Theme → Top Bar
7. Footer: VOXLINE 2.0 | SADTONY
8. 移除 BODY / CLARITY / AIR / SMOOTH knobs 的 UI 文字
9. 保留所有 APVTS 參數

**Commit**: ui: migrate controls to V2 layout

---

## Stage 3 — Vocal EQ UI
1. 在 POLISH 下方加入 EQ control bar
2. EQ knob labels: HPF / LOW / MUD / PRES / AIR / LPF
3. LOW maps to body APVTS ID
4. PRES maps to clarity APVTS ID
5. AIR maps to air APVTS ID
6. LPF maps to smooth APVTS ID
7. Add new HPF and MUD APVTS parameters
8. 加 EQ curve display（visual-only V1）
9. 連動現有 DSP filter

**Commit**: ui: add vocal EQ controls and curve display

---

## Stage 4 — EQ DSP
1. 實作 HPF filter
2. 實作 MUD filter（low-mid bell）
3. 更新 body/clarity/air/smooth DSP mapping 對應新 EQ
4. POLISH macro 影響 EQ intensity（不影響 SPACE）
5. 確認所有 EQ 參數聽感正常

**Commit**: feat: implement vocal EQ DSP

---

## Stage 5 — Visual Polish
1. EQ curve 接真實參數
2. POLISH status text（Natural / Pushed / Intense）
3. 整體間距微調
4. Dark/Light 主題最終調整
5. 移除所有 V1 殘留 UI

**Commit**: style: V2 visual polish pass

---

## Stage 6 — 打包
1. 更新 README / MANUAL 到 V2
2. Build Release VST3 + AU
3. 包 DMG
4. GitHub Actions CI
5. 最終 commit + tag v2.0.0

**Commit**: release: VOXLINE 2.0.0
