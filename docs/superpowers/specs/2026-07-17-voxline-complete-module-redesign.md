# VOXLINE 全模組重整設計規格

## 目標

把 VOXLINE 重整為一套容易理解、數值可信、低延遲且可預測的人聲效果鏈。主畫面保留快速完成聲音的一旋鈕工作流，Advanced 提供真正需要的精細控制；任何主旋鈕都不得在畫面外偷偷改變其他模組。

本次包含 DSP、參數、狀態、Preset、電表、整體介面與驗證的整合更新。介面與功能已逐區塊確認，實作時不得重新加入已移除的功能。

## 核心原則

- 主畫面適合快速操作，Advanced 負責精細調整。
- 顯示值必須反映實際 DSP，不以裝飾值或二次錯誤換算代替。
- EQ、DE-ESS、COMP、POLISH、DRIVE、SPACE 彼此獨立。
- 所有增益、濾波、模式切換、Bypass 與 A/B 切換皆需平滑，避免 click、pop 或音量跳變。
- 採低延遲架構，不使用 lookahead limiter。
- 介面只維持深色主題。

## 訊號流程

`Input Gain → Input Meter → Vocal EQ → DE-ESS → COMP → POLISH → DRIVE → SPACE → Output Gain → Emergency Soft Clip → Output Meter`

- Input Meter 位於 Input Gain 之後、任何音色或動態處理之前。
- Output Meter 位於所有處理之後，顯示真正送回 DAW 的最終訊號。
- Bypass 作用於完整效果鏈，切換時以短交叉淡化避免爆音。
- 目標處理延遲不高於約 1.5 ms；插件必須向 Host 正確回報實際延遲。

## 視窗與整體佈局

- 收合尺寸：1080 × 720。
- Advanced 展開尺寸：1080 × 940。
- 上層主要區域：Input、中央大型 POLISH、Output。
- 快速控制列：
  - Tone：BODY、PRESENCE、AIR。
  - Processing：DE-ESS、COMP、DRIVE、SPACE。
  - 每個動態模組提供必要的迷你狀態或減益回饋。
- Advanced 頁籤：Vocal EQ、Comp、De-Ess、Drive、Space。
- 不顯示長期無作用的狀態徽章或重複資訊。
- 僅保留深色素材與深色樣式；移除使用者可見的亮／暗主題切換。

## 頂部工具列

採用已確認的「A：中央寬選單」配置，由左至右為：

1. VOXLINE 品牌與副標。
2. 上一個 User Preset。
3. 下一個 User Preset。
4. 寬版 User Preset 下拉選單。
5. `SAVE AS`。
6. `A/B`。
7. `BYPASS`。

移除：

- FAV。
- 全域 LISTEN。
- 內建／Factory Preset 選單項目。
- 亮／暗主題按鈕。

## User Preset

- 不提供任何內建／Factory Preset。
- 保留使用者自行建立、載入、重新命名與刪除的 Preset。
- 尚未儲存時目前名稱顯示 `Untitled`，下拉空狀態顯示 `No User Presets`，並引導使用 `SAVE AS`。
- 已修改但尚未儲存的 Preset，以橘色圓點標示 edited 狀態。
- 下拉選單目前項目需明確反白；每列 `•••` 提供重新命名與刪除。
- Preset 名稱不可重複；刪除前需確認。
- 切換 Preset、上一個／下一個、關閉編輯器或載入其他狀態前，如有未儲存修改需提示。
- 監聽工具狀態（例如 EQ Band Solo、De-ess Listen S）不寫入 Preset。
- 舊 Host program API 退化為單一預設 program，以維持插件 API 合法，但不得再暴露九個 factory programs。
- User Preset 儲存在平台對應的 VOXLINE 使用者資料目錄；下拉選單列舉該目錄，不再依賴任意檔案的 Load 對話框。

## A/B 與 Bypass

- A/B 保存兩份完整「聲音參數」快照。
- A/B 不保存暫時監聽狀態、電表狀態、Clip Hold 或 UI 展開狀態。
- A/B 的兩份快照與目前槽位保存在 Host session state，關閉再開 Editor 不會遺失；User Preset 本身不包含 A/B 快照。
- A/B 切換採平滑參數過渡；目前 A 或 B 必須清楚顯示。
- Bypass 為全鏈 Bypass，使用乾／濕短交叉淡化。

## Input

### 操作

- 保留 Input Gain。
- 移除 AUTO GAIN 的按鈕、附件與實際處理。
- 移除隱藏的 Clean Mode 與其重複 HPF 處理。
- 為舊 Session 相容而保留的 retired parameter，只可在狀態載入時忽略或遷移，不得重新影響聲音。

### Input Meter

- 量測位置：Input Gain 後、Vocal EQ 前。
- 顯示水平電表及兩個數值：
  - Peak，單位 dBFS。
  - RMS，單位 dBFS。
- 範圍：−60 至 0 dBFS。
- RMS 使用主要填色，Peak 使用獨立標記。
- 顯示 −18 dBFS 目標參考。
- 接近 −6 dBFS 進入警示區，0 dBFS 觸發 Clip Hold。
- DSP 對 UI 傳遞單一、明確的單位；禁止先正規化成 0–1 又在 UI 當成線性振幅轉 dB。
- attack／release 時間常數依實際更新週期計算，不得把 per-sample 係數直接用在 per-block 更新。
- 靜音時電表回到明確的 silence floor；不得以固定最小填色假裝仍有輸入。
- DSP 與 GUI 不可用兩套互不一致的固定係數重複平滑。

## Vocal EQ

### 主畫面

- BODY：低頻重量。
- PRESENCE：人聲前進感與清晰度。
- AIR：頂端空氣感。
- 三顆旋鈕直接共用 Advanced 的 LOW、PRES、AIR 參數，不建立第二套幽靈增益。

### Advanced

六頻段：

1. HPF。
2. LOW。
3. MUD。
4. PRES。
5. AIR。
6. LPF。

每頻段提供：

- 獨立 On／Off。
- 圖形節點拖曳。
- FREQ 旋鈕。
- GAIN 或 SLOPE 旋鈕。
- 適用頻段的 Q 旋鈕。
- 精確數值輸入。
- Reset Band。

全頁提供 EQ On／Off 與 Reset All。

### 行為

- HPF、MUD、LPF 初始可以真正關閉，不得有隱藏預設處理。
- LOW、PRES、AIR 的 Advanced 數值與主畫面必須即時同步。
- 曲線使用目前 sample rate 計算，不得硬編碼 48 kHz。
- 係數更新需平滑並避免 zipper noise。
- 移除或退休不參與聲音的 `lowGain`、`presGain`、`airGain` 等重複參數。

### Band Solo

- Band Solo 是暫時監聽工具，不可自動化、不可寫入 Preset。
- Bell／Shelf 類頻段監聽該頻率與 Q 附近的內容。
- HPF Solo 監聽被 HPF 移除的低頻。
- LPF Solo 監聽被 LPF 移除的高頻。
- 啟用時顯示清楚警告與目前監聽頻段。
- 切換 Advanced 頁籤、載入 Preset、關閉或重開編輯器時自動解除。

## DE-ESS

### 主畫面

- 一顆 Amount 旋鈕。
- Reduction 電表與 dB 數字。
- 偵測到齒音時顯示 `S ACTIVE`。
- 顯示目前 Focus Frequency，讓單一旋鈕的作用可理解。

### Advanced

- Frequency。
- Sensitivity。
- Max Range。
- Split／Wide 模式。
- Listen S。

### 行為

- Amount 主要決定目標去齒音強度。
- 自然工作區的典型減益為 2–5 dB；超過 9 dB 顯示過度處理警示。
- Split 模式只壓低齒音頻帶；Wide 模式在偵測到齒音時降低全頻。
- Listen S 僅供暫時監聽偵測內容，不保存、不自動化，離開頁面時自動解除。
- 左右聲道使用 linked detector，避免立體聲像漂移。
- 包絡時間需與 sample rate、block size 無關。
- DE-ESS 不受 POLISH 影響。

## COMP

### 主畫面

- 一顆 Amount 旋鈕，控制「目標人聲穩定程度」，而不是暗中任意改變其他模組。
- 狀態文字：
  - 低量：`LIGHT`。
  - 中量：`CONTROLLED`。
  - 高量：`FIRM`。
- 顯示 Gain Reduction 電表與 dB。
- 目標減益曲線：
  - 0：Off。
  - 25：約 1–2 dB。
  - 50：約 3–4 dB。
  - 75：約 5–7 dB。
  - 100：約 8–10 dB。

### Advanced

- Sensitivity。
- Ratio。
- Attack。
- Release。
- Mix。
- Makeup／Auto Makeup。

不另設一顆直接 Threshold 控制；Sensitivity 與 Amount 共同建立可理解的自動目標工作點，避免 Threshold 與主旋鈕互相打架。

### 行為

- Compressor detector 取自 DE-ESS 後、COMP 前的訊號，與文件中的訊號流程一致。
- 顯示 GR 必須來自實際 compressor gain envelope。
- Auto Makeup 只補償壓縮造成的平均音量差，不以變大聲製造「更好聽」錯覺。
- COMP 不受 POLISH 影響。
- Output 不再重複顯示 compressor GR。

## POLISH

- POLISH 是產品核心的一旋鈕人聲強化層。
- 不提供 Advanced 頁或額外 Focus／Density／Shine 控制。
- 顯示百分比與狀態：
  - 0–33：`NATURAL`。
  - 34–72：`POLISHED`。
  - 73–100：`PUSHED`。
- 0% 必須完全不加入 POLISH 專屬處理。
- 50% 為自然、完整、適合大多數人聲的預設工作點。
- 100% 更集中、更前進，但仍維持可用且避免明顯失真。
- POLISH 只控制自己的核心層：
  - 輕度動態整形。
  - 少量暖度／密度。
  - 極輕微 Presence／Air 強化。
  - 內部音量補償。
- 不顯示 `Level Compensated` 標籤；補償是內部正常行為。
- POLISH 不得改變 Input Gain、手動 EQ、DE-ESS、COMP、DRIVE、SPACE 或 Output Gain。

## DRIVE

### 主畫面

- 維持一顆 Amount 旋鈕。
- 0：中性／關閉。
- 1–40：暖度與密度。
- 41–75：Grit 與更前進的質感。
- 76–100：明顯、創意型失真。
- 以簡短 Character／Color 狀態協助理解，不堆疊長期無必要的徽章。

### Advanced

- Character：Clean／Warm／Edge。
- Tone。
- Mix。
- Output Trim。
- Level Match。

### 行為

- DRIVE 是有意識的音色選擇，與乾淨的 POLISH 分工。
- Level Match 使旁通比較不被音量誤導。
- 非線性演算法需處理 aliasing；可採按模式或 Amount 啟用的 oversampling，但必須符合整體低延遲目標並正確回報。
- DRIVE 不受 POLISH 影響。

## SPACE

### 主畫面

- 一顆 Amount 旋鈕。
- 顯示目前模式。
- 模式：Room、Plate、Hall、Slap、Width。

### Advanced

Room／Plate／Hall：

- Pre-delay。
- Size。
- Decay。
- Tone。
- Width。
- Ducking。

Slap：

- Time。
- Feedback。
- Tone。
- Width。
- Ducking。

Width：

- Delay／Spread。
- Tone。
- Mono Safety。

### 行為

- Room、Plate、Hall 使用真正對應的 reverb 結構，不以幾個固定 delay taps 假裝 reverb。
- Plate 使用較明亮、平滑且具有擴散感的尾韻。
- Hall 使用較長尾韻，並帶適量 modulation 避免金屬感。
- Pre-delay、Size、Decay、Tone、Width、Ducking 的六欄標籤、旋鈕與數值固定對齊，標籤不得斷行。
- 切換模式需平滑，避免 delay buffer 造成 click。
- Ducking 由乾人聲控制空間回授／濕聲，保留字句清楚度。
- SPACE 不受 POLISH 影響。

## Output

### 電表與操作

- 雙聲道 L／R 最終電表。
- 每聲道顯示 RMS 填色與 Peak 標記。
- 顯示 True Peak 數值與 RMS 數值。
- 保留 Output Gain。
- 保留 Clip Hold 與 Clip Clear。
- 不顯示固定 `SAFETY · −1 dBTP`。
- 不顯示固定 `NO LIMITING`。
- 只有 limiter／soft clip 或 clip 真正發生時才顯示警告。

### True Peak 與安全處理

- True Peak 只負責量測與警示，不使用 lookahead。
- 使用低延遲、連續的 emergency soft clip 保護極端超載。
- 取代目前在門檻處不連續的 soft clip；函數值與一階斜率在接合區需連續，不能在 0.98 附近產生跳變。
- 正常操作仍以 Output Gain 避免 clipping；soft clip 不是常態 loudness limiter。
- True Peak、RMS、Peak 的單位與 ballistic 規則需有數值測試。

## 參數、狀態與相容性

- 建立單一參數定義來源，讓 DSP、UI attachment、A/B、Preset 與測試共用。
- 保留仍有意義且 Host 已認得的 parameter ID；不要只為整理名稱而破壞舊 automation。
- 已移除功能的舊 ID 採 retired／migration 策略：
  - 舊 Session 可載入。
  - 不出現在新 UI。
  - 不再影響聲音。
- 重複 EQ 參數需遷移到實際使用的 Body／Presence／Air 參數。
- 狀態載入必須檢查版本，對缺少的新參數套用安全預設。
- User Preset 使用版本化狀態格式，未知欄位可忽略，避免未來版本無法載入。
- 舊版 `body`／`clarity`／`air` 若與新版沿用相同 ID 但數值語意不同，migration 必須依來源版本明確轉換，禁止把舊 0–100 值直接當成新版 dB 值。

## Sample Rate、聲道與即時安全

- 支援 44.1、48、88.2、96 kHz。
- 支援 mono 與 stereo；linked dynamics 不得造成聲像偏移。
- audio thread 不配置記憶體、不做檔案 I/O、不持有 UI lock。
- 所有 delay／reverb buffer 在 `prepareToPlay` 配置。
- 所有時間常數與濾波係數使用實際 sample rate。
- 參數自動化、A/B、Preset、Bypass 與模式切換均需防 denormal 並平滑。

## 驗證與完成條件

### DSP 數值測試

- Input／Output Peak、RMS 的已知正弦波與靜音測試。
- True Peak 的 inter-sample peak 測試。
- meter attack／release 在不同 block size 的時間一致性。
- soft clip 接合處連續性、有限輸出與極端輸入測試。
- EQ 頻率響應、On／Off、主畫面與 Advanced 參數一致性。
- De-ess 齒音頻帶減益、Split／Wide 與 linked stereo。
- Compressor Amount 對目標 GR 的單調性、Attack／Release、Mix 與 makeup。
- POLISH 0% null／近似 null 測試，並確認不改寫其他模組參數。
- DRIVE 0% null、Level Match 與 aliasing 基準。
- SPACE 五模式、tail、pre-delay、ducking、mono safety。

### 狀態與相容性測試

- 所有參數 round-trip。
- 舊版 state／preset migration。
- User Preset 建立、改名、刪除、重複名稱與 edited 提示。
- 無 factory presets。
- A/B 排除監聽狀態。
- Band Solo／Listen S 不保存且在指定事件自動解除。

### 整合與 UI 驗證

- 44.1／48／88.2／96 kHz，mono／stereo，多種 block size。
- 快速 automation、Preset、A/B、Bypass、模式切換無 click／pop。
- 1080×720 與 1080×940 無遮擋、截字或控制錯位。
- PRE-DELAY 等 Advanced 標籤保持單行對齊。
- 深色主題所有狀態具可讀對比；移除控制不再出現。
- VST3 與 macOS AU 完成建置及基本載入測試。
- AU／VST3 bundle、Host 顯示與發佈檔的版本號必須一致。

## 非目標

- 本輪不製作內建 Preset。
- 本輪不恢復 FAV、全域 LISTEN、AUTO GAIN、Clean Mode 或亮色主題。
- 本輪不加入 lookahead brickwall limiter。
- 本輪不把 POLISH 改成多旋鈕模組。
- 本輪不更新完整使用說明書內容；介面完成後另行同步文件與圖片。

## 製作策略

實作採多 Sub Agent 並行，但只在檔案所有權清楚時同步修改：

- DSP Agent：依獨立模組拆分演算法與單元測試。
- UI Agent：元件、佈局、Advanced 頁與電表呈現。
- State/Preset Agent：參數定義、migration、User Preset、A/B。
- Verification Agent：數值測試、建置矩陣與整合驗證。

共用的 Processor 參數定義、主訊號鏈與 Editor 組裝由整合階段集中修改。每批合併後執行完整測試，避免多 Agent 同時覆寫 `PluginProcessor.cpp` 或 `PluginEditor.cpp`。

因現有 DSP 與 UI 分別集中在單一 Processor／Editor 檔，製作分為以下批次：

1. 單一整合 owner 先建立版本化狀態契約、模組 DSP 介面、共用 smoothing／sample-rate context 與分離的測試檔。
2. 單一 UI owner 把 Editor 拆成 Top Bar、Meter、Main Macro、Advanced 與 User Preset Manager，並選定唯一 layout source；不再同時維護硬編碼座標、舊 layout 類別與 JSON 三套來源。
3. 邊界完成後，DSP 模組與 UI 元件由不同 Sub Agent 平行實作，各自只修改分配到的檔案。
4. 由整合 owner 序列接回主訊號鏈、APVTS attachments、migration、A/B 與 Preset。
5. DSP 數值、State migration、UI smoke 可平行驗證；最後集中跑完整 sample-rate／聲道／block-size 與平台建置矩陣。
