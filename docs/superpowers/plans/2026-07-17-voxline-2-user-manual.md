# VOXLINE 2.0 User Manual Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Produce a Traditional Chinese illustrated GitHub manual and verified PDF for VOXLINE 2.0, link both from the repository, and attach the PDF to the `v2.0.0` GitHub Release.

**Architecture:** Curated VOXLINE 2.0 screenshots live under `docs/images` and are referenced by one canonical Markdown manual. A small deterministic PDF build script renders the same approved content into a branded PDF, which is visually inspected page by page. Repository and CI changes expose both formats and attach the PDF once during tagged releases.

**Tech Stack:** Markdown, PNG, Python 3, ReportLab, Poppler PDF rendering, CMake/JUCE metadata, GitHub Actions, GitHub CLI.

## Global Constraints

- All user-facing prose is Traditional Chinese.
- Product version is exactly `2.0.0`; the Git tag is exactly `v2.0.0`.
- Screenshots must show the actual dark VOXLINE 2.0 interface and must not expose unrelated desktop content.
- Deliverables are `docs/USER_MANUAL_ZH-TW.md`, `docs/VOXLINE-2.0-使用說明書.pdf`, and `docs/images/*.png`.
- The manual covers macOS Apple Silicon, macOS Intel, macOS Universal, and Windows x64 VST3.
- Existing untracked `qa/`, `tools/`, `design-qa.md`, and `voxline-editable-figma.svg` content is not staged unless explicitly listed by a task.

---

### Task 1: Curate release screenshots

**Files:**
- Create: `docs/images/voxline-main.png`
- Create: `docs/images/voxline-vocal-eq.png`
- Create: `docs/images/voxline-comp.png`
- Create: `docs/images/voxline-de-ess.png`
- Create: `docs/images/voxline-drive.png`
- Create: `docs/images/voxline-space.png`

**Interfaces:**
- Consumes: approved screenshots under `qa/` and the current VOXLINE 2.0 UI.
- Produces: six stable PNG paths used by both manual formats.

- [ ] **Step 1: Inspect candidate images**

Run:

```bash
sips -g pixelWidth -g pixelHeight \
  qa/voxline-main-qa.png \
  qa/voxline-advanced-eq-final.png \
  qa/voxline-advanced-comp.png \
  qa/voxline-advanced-deess.png \
  qa/voxline-advanced-drive.png \
  qa/voxline-advanced-space-final.png
```

Expected: every source reports non-zero dimensions and shows the dark 2.0 interface when opened.

- [ ] **Step 2: Copy or crop only the plugin area**

Use `sips` losslessly where cropping is required and preserve control labels. Save the six outputs with the exact names listed above. Images containing AudioPluginHost or DAW chrome must be cropped so only VOXLINE remains.

- [ ] **Step 3: Verify image quality and privacy**

Run:

```bash
sips -g pixelWidth -g pixelHeight docs/images/*.png
```

Expected: six readable PNG files, each at least 1000 pixels wide, with no desktop menu bar, username, unrelated plugin list, or DAW content.

### Task 2: Write the canonical GitHub manual

**Files:**
- Create: `docs/USER_MANUAL_ZH-TW.md`

**Interfaces:**
- Consumes: the six image paths from Task 1 and parameter names from `Source/PluginProcessor.cpp` and `Source/PluginEditor.cpp`.
- Produces: the canonical Traditional Chinese user instructions and content source for the PDF.

- [ ] **Step 1: Confirm parameter and installation names**

Run:

```bash
rg -n 'createAndAddParameter|ParameterID|addParameter|VST3|Components|BODY|PRESENCE|AIR|DE-ESS|COMP|DRIVE|SPACE' \
  Source README.md MANUAL.md CMakeLists.txt
```

Expected: every control and installation path used in the manual maps to current source or release documentation.

- [ ] **Step 2: Write the complete Markdown structure**

Create these headings in this order:

```markdown
# VOXLINE 2.0 使用說明書
## 1. VOXLINE 是什麼
## 2. 系統需求與安裝
## 3. 60 秒快速開始
## 4. 主介面
## 5. Vocal EQ
## 6. Compressor
## 7. De-Esser
## 8. Drive
## 9. Space
## 10. 建議人聲工作流程
## 11. 常見問題
## 12. 版本與支援
```

The installation section must name all four 2.0 artifacts. Each control section must state purpose, audible effect, safe starting range, and a warning against over-processing. Include all six images using relative paths such as `![VOXLINE 主介面](images/voxline-main.png)`.

- [ ] **Step 3: Check links, images, and version text**

Run:

```bash
test "$(rg -o 'images/[^)]+' docs/USER_MANUAL_ZH-TW.md | wc -l | tr -d ' ')" -eq 6
rg -n '1\.0\.0|v1\.0\.0' docs/USER_MANUAL_ZH-TW.md && exit 1 || true
for image in $(rg -o 'images/[^)]+' docs/USER_MANUAL_ZH-TW.md); do test -f "docs/$image"; done
```

Expected: six image references resolve and no 1.0 version string remains.

### Task 3: Build and visually verify the PDF

**Files:**
- Create: `scripts/build_user_manual_pdf.py`
- Create: `docs/VOXLINE-2.0-使用說明書.pdf`
- Create during verification only: `qa/manual-pages/*.png`

**Interfaces:**
- Consumes: the approved manual content and six screenshots.
- Produces: a branded, self-contained PDF suitable for GitHub Release download.

- [ ] **Step 1: Implement deterministic PDF generation**

Use ReportLab with an embedded Traditional Chinese-capable system font. The script must define A4 page size, margins, cover, heading hierarchy, orange callout style, tables, image scaling, footer page numbers, and metadata title `VOXLINE 2.0 使用說明書`. It must fail clearly when any source image or font is missing.

- [ ] **Step 2: Generate the PDF**

Run:

```bash
python3 scripts/build_user_manual_pdf.py
```

Expected: `docs/VOXLINE-2.0-使用說明書.pdf` exists and is non-empty.

- [ ] **Step 3: Validate PDF structure and text**

Run:

```bash
pdfinfo docs/VOXLINE-2.0-使用說明書.pdf
pdftotext docs/VOXLINE-2.0-使用說明書.pdf - | rg 'VOXLINE 2.0|Vocal EQ|Compressor|De-Esser|Drive|Space'
```

Expected: valid A4 PDF metadata and searchable text containing every advanced module.

- [ ] **Step 4: Render every page for visual QA**

Run:

```bash
mkdir -p qa/manual-pages
pdftoppm -png -r 120 docs/VOXLINE-2.0-使用說明書.pdf qa/manual-pages/page
```

Inspect every rendered page. Expected: no cropped text, overlap, missing glyphs, stretched images, blank pages, widows, or unreadably small labels. Fix the script and repeat Steps 2–4 until clean.

### Task 4: Integrate documentation and tagged release

**Files:**
- Modify: `README.md`
- Modify: `.github/workflows/build.yml`

**Interfaces:**
- Consumes: the final Markdown and PDF paths.
- Produces: discoverable repository documentation and one PDF attachment on tagged releases.

- [ ] **Step 1: Add README manual links**

Add a `Documentation` section linking to:

```markdown
- [繁體中文圖文使用說明](docs/USER_MANUAL_ZH-TW.md)
- [下載 VOXLINE 2.0 PDF 使用說明書](docs/VOXLINE-2.0-使用說明書.pdf)
```

- [ ] **Step 2: Attach the PDF once on tag builds**

Add a `release-docs` job to `.github/workflows/build.yml` that runs only for `refs/tags/v*`, depends on `windows-x64` and `macos`, checks out the tagged source, verifies the PDF exists, and uses `softprops/action-gh-release@v2` with:

```yaml
files: docs/VOXLINE-2.0-使用說明書.pdf
```

- [ ] **Step 3: Run repository verification**

Run:

```bash
ruby -e "require 'yaml'; YAML.load_file('.github/workflows/build.yml'); puts 'Workflow YAML: OK'"
git diff --check
cmake --build build-ci --parallel 6
ctest --test-dir build-ci --output-on-failure
```

Expected: YAML parses, diff check is clean, build succeeds, and tests report 100% pass.

- [ ] **Step 4: Commit and publish the documentation changes**

Stage only:

```bash
git add README.md .github/workflows/build.yml docs/USER_MANUAL_ZH-TW.md \
  docs/VOXLINE-2.0-使用說明書.pdf docs/images scripts/build_user_manual_pdf.py \
  docs/superpowers/plans/2026-07-17-voxline-2-user-manual.md
git commit -m "docs: add illustrated VOXLINE 2.0 manual"
git push
```

Wait for all four PR jobs to pass. Merge PR #1, create annotated tag `v2.0.0` on the merged `main`, and push it. Wait for the tagged workflow to pass, then verify the GitHub Release contains these five files:

```text
VOXLINE-2.0.0-Windows-x64-VST3.zip
VOXLINE-2.0.0-macOS-Apple-Silicon.dmg
VOXLINE-2.0.0-macOS-Intel.dmg
VOXLINE-2.0.0-macOS-Universal.dmg
VOXLINE-2.0-使用說明書.pdf
```
