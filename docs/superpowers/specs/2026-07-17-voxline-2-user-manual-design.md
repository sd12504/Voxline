# VOXLINE 2.0 圖文使用說明書設計

## 目標

為 VOXLINE 2.0 製作一套內容一致的繁體中文說明文件，讓第一次使用人聲效果器的使用者能完成安裝、基本調整與進階模組設定。文件同時提供 GitHub Markdown 與可下載 PDF，並隨 `v2.0.0` Release 發佈。

## 交付物

- `docs/USER_MANUAL_ZH-TW.md`：GitHub 直接閱讀的完整圖文手冊。
- `docs/VOXLINE-2.0-使用說明書.pdf`：排版完成、可下載或列印的 PDF。
- `docs/images/`：手冊使用的壓縮 PNG 圖片，檔名穩定且不依賴本機路徑。
- `README.md`：加入 Markdown 與 PDF 入口。
- GitHub `v2.0.0` Release：附加 PDF 及四個平台安裝檔。

## 內容結構

1. 產品定位與系統需求。
2. macOS Apple Silicon、macOS Intel、macOS Universal 與 Windows x64 安裝方式。
3. 快速開始：輸入電平、POLISH、輸出增益與 A/B／LISTEN／BYPASS。
4. 主控制：BODY、PRESENCE、AIR、DE-ESS、COMP、DRIVE、SPACE。
5. 進階頁籤：Vocal EQ、Comp、De-Ess、Drive、Space。
6. 建議人聲工作流程與安全的起始值。
7. 常見問題、插件掃描位置與疑難排解。

## 圖片策略

- 只使用 VOXLINE 2.0 的實際深色介面，不使用早期概念圖或淺色模式。
- 主介面使用乾淨的插件截圖；各進階頁使用對應頁籤截圖。
- 含 AudioPluginHost 或 DAW 背景的圖片會裁切到插件視窗，避免暴露無關桌面資訊。
- 圖片保留可讀的控制名稱與數值；GitHub 版控制單張寬度，PDF 版依頁面寬度縮放。
- 圖說會說明畫面用途，不把圖片當作唯一操作指引。

## 視覺規格

- PDF 使用深炭黑封面、暖白正文、橘色重點與紫灰輔助色，呼應插件介面並維持長文閱讀性。
- 標題、表格、提示框與頁碼採一致層級；避免滿版深色正文造成列印與閱讀負擔。
- 中文字型以系統可嵌入字型為主，輸出前檢查缺字、截斷、圖片失真與孤行。

## 驗證

- Markdown 的所有相對圖片與 PDF 連結可在 GitHub 開啟。
- PDF 逐頁轉成圖片檢查版面，確認沒有溢出、遮擋或缺字。
- 參數名稱與插件目前程式碼、2.0 版本號及四平台檔名一致。
- Git 差異不包含 `qa/`、本機暫存畫面或其他未授權設計檔。

## 發佈順序

1. 完成並驗證 Markdown、圖片及 PDF。
2. 更新 README，提交並推送 PR。
3. 等待四平台 GitHub Actions 全部通過。
4. 合併 PR，建立並推送 `v2.0.0` 標籤。
5. 等待標籤工作完成，確認四個安裝檔與 PDF 均出現在 GitHub Release。
