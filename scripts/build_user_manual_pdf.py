#!/usr/bin/env python3
"""Build the illustrated Traditional Chinese VOXLINE 2.0 user manual."""

from __future__ import annotations

import html
import re
import sys
from dataclasses import dataclass
from pathlib import Path

try:
    from PIL import Image as PILImage
    from reportlab.lib import colors
    from reportlab.lib.enums import TA_CENTER, TA_LEFT
    from reportlab.lib.pagesizes import A4
    from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
    from reportlab.lib.units import mm
    from reportlab.pdfbase import pdfmetrics
    from reportlab.pdfbase.ttfonts import TTFont
    from reportlab.platypus import (
        BaseDocTemplate,
        CondPageBreak,
        Flowable,
        Frame,
        HRFlowable,
        Image,
        KeepTogether,
        PageBreak,
        PageTemplate,
        Paragraph,
        Spacer,
        Table,
        TableStyle,
    )
except ModuleNotFoundError as error:
    print(
        "錯誤：缺少 PDF 產生套件。請先執行 "
        "`python3 -m pip install -r scripts/requirements-manual.txt`。",
        file=sys.stderr,
    )
    raise SystemExit(1) from error


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "docs" / "USER_MANUAL_ZH-TW.md"
OUTPUT = ROOT / "docs" / "VOXLINE-2.0-使用說明書.pdf"
FONT_MEDIUM = Path("/System/Library/Fonts/STHeiti Medium.ttc")
FONT_LIGHT = Path("/System/Library/Fonts/STHeiti Light.ttc")
EXPECTED_IMAGES = (
    "voxline-main.png",
    "voxline-vocal-eq.png",
    "voxline-comp.png",
    "voxline-de-ess.png",
    "voxline-drive.png",
    "voxline-space.png",
)

PAGE_W, PAGE_H = A4
MARGIN_X = 18 * mm
MARGIN_TOP = 20 * mm
MARGIN_BOTTOM = 18 * mm
CONTENT_W = PAGE_W - 2 * MARGIN_X
CONTENT_H = PAGE_H - MARGIN_TOP - MARGIN_BOTTOM

INK = colors.HexColor("#181918")
MUTED = colors.HexColor("#686862")
PAPER = colors.HexColor("#F5F1E9")
PAPER_ALT = colors.HexColor("#EBE4D9")
CHARCOAL = colors.HexColor("#111312")
PANEL = colors.HexColor("#1A1D1B")
ORANGE = colors.HexColor("#EF6C32")
ORANGE_DARK = colors.HexColor("#A63F1D")
PURPLE = colors.HexColor("#876B99")
WHITE = colors.HexColor("#F7F3EC")


def fail(message: str) -> "None":
    print(f"錯誤：{message}", file=sys.stderr)
    raise SystemExit(1)


def validate_inputs() -> None:
    if not SOURCE.is_file():
        fail(f"找不到內容來源：{SOURCE}")
    for font in (FONT_MEDIUM, FONT_LIGHT):
        if not font.is_file():
            fail(f"找不到繁體中文字型：{font}")
    missing = [
        ROOT / "docs" / "images" / name
        for name in EXPECTED_IMAGES
        if not (ROOT / "docs" / "images" / name).is_file()
    ]
    if missing:
        joined = "\n  ".join(str(path) for path in missing)
        fail(f"缺少手冊圖片：\n  {joined}")


def register_fonts() -> None:
    pdfmetrics.registerFont(TTFont("STHeiti-Medium", str(FONT_MEDIUM), subfontIndex=0))
    pdfmetrics.registerFont(TTFont("STHeiti-Light", str(FONT_LIGHT), subfontIndex=0))


def inline_markup(text: str) -> str:
    """Convert the small inline Markdown subset used by the manual."""
    placeholders: list[str] = []

    def hold(value: str) -> str:
        placeholders.append(value)
        return f"@@MARKUP{len(placeholders) - 1}@@"

    escaped = html.escape(text.strip(), quote=False)
    escaped = re.sub(
        r"`([^`]+)`",
        lambda m: hold(f'<font name="STHeiti-Medium" color="#6F4B7D">{m.group(1)}</font>'),
        escaped,
    )
    escaped = re.sub(r"\*\*([^*]+)\*\*", lambda m: hold(f"<b>{m.group(1)}</b>"), escaped)
    escaped = re.sub(r"\[([^\]]+)\]\(([^)]+)\)", lambda m: hold(f'<link href="{m.group(2)}" color="#A63F1D">{m.group(1)}</link>'), escaped)
    for index, value in enumerate(placeholders):
        escaped = escaped.replace(f"@@MARKUP{index}@@", value)
    return escaped


class NumberedCanvasMixin:
    """Canvas callbacks shared by every page template."""

    @staticmethod
    def metadata(canvas) -> None:
        canvas.setTitle("VOXLINE 2.0 使用說明書")
        canvas.setAuthor("VOXLINE")
        canvas.setSubject("VOXLINE 2.0 VST3 人聲效果器繁體中文使用說明")
        canvas.setCreator("VOXLINE Manual Builder")


def draw_body_page(canvas, doc) -> None:
    NumberedCanvasMixin.metadata(canvas)
    canvas.saveState()
    canvas.setFillColor(PAPER)
    canvas.rect(0, 0, PAGE_W, PAGE_H, fill=1, stroke=0)
    canvas.setStrokeColor(colors.HexColor("#D8CFC2"))
    canvas.setLineWidth(0.45)
    canvas.line(MARGIN_X, PAGE_H - 13 * mm, PAGE_W - MARGIN_X, PAGE_H - 13 * mm)
    canvas.setFont("STHeiti-Medium", 7.5)
    canvas.setFillColor(MUTED)
    canvas.drawString(MARGIN_X, PAGE_H - 10 * mm, "VOXLINE 2.0")
    canvas.setFillColor(ORANGE_DARK)
    canvas.drawRightString(PAGE_W - MARGIN_X, PAGE_H - 10 * mm, "使用說明書")
    canvas.setStrokeColor(colors.HexColor("#D8CFC2"))
    canvas.line(MARGIN_X, 12 * mm, PAGE_W - MARGIN_X, 12 * mm)
    canvas.setFont("STHeiti-Light", 7.2)
    canvas.setFillColor(MUTED)
    canvas.drawString(MARGIN_X, 7.5 * mm, "VOXLINE · COMPLETE VOCAL CHANNEL")
    canvas.drawRightString(PAGE_W - MARGIN_X, 7.5 * mm, f"{doc.page}")
    canvas.restoreState()


def draw_cover_page(canvas, doc) -> None:
    NumberedCanvasMixin.metadata(canvas)
    canvas.saveState()
    canvas.setFillColor(CHARCOAL)
    canvas.rect(0, 0, PAGE_W, PAGE_H, fill=1, stroke=0)
    canvas.setFillColor(PANEL)
    canvas.circle(PAGE_W + 12 * mm, PAGE_H - 36 * mm, 58 * mm, fill=1, stroke=0)
    canvas.setFillColor(colors.HexColor("#28202C"))
    canvas.circle(PAGE_W + 9 * mm, PAGE_H - 34 * mm, 36 * mm, fill=1, stroke=0)
    canvas.setFillColor(ORANGE)
    canvas.rect(18 * mm, PAGE_H - 30 * mm, 15 * mm, 1.5 * mm, fill=1, stroke=0)
    canvas.setStrokeColor(colors.HexColor("#2D302E"))
    canvas.setLineWidth(0.5)
    for y in range(34, 132, 8):
        canvas.line(18 * mm, y * mm, PAGE_W - 18 * mm, y * mm)
    canvas.setFont("STHeiti-Light", 7.5)
    canvas.setFillColor(colors.HexColor("#A9AAA5"))
    canvas.drawString(18 * mm, 14 * mm, "VERSION 2.0.0  ·  TRADITIONAL CHINESE")
    canvas.restoreState()


def make_styles() -> dict[str, ParagraphStyle]:
    sample = getSampleStyleSheet()
    return {
        "cover_brand": ParagraphStyle(
            "CoverBrand",
            parent=sample["Normal"],
            fontName="STHeiti-Medium",
            fontSize=12,
            leading=15,
            textColor=ORANGE,
            spaceAfter=20 * mm,
            wordWrap="CJK",
        ),
        "cover_title": ParagraphStyle(
            "CoverTitle",
            parent=sample["Title"],
            fontName="STHeiti-Medium",
            fontSize=35,
            leading=43,
            textColor=WHITE,
            alignment=TA_LEFT,
            spaceAfter=5 * mm,
            wordWrap="CJK",
        ),
        "cover_subtitle": ParagraphStyle(
            "CoverSubtitle",
            parent=sample["Normal"],
            fontName="STHeiti-Light",
            fontSize=14,
            leading=22,
            textColor=colors.HexColor("#C8C4BD"),
            wordWrap="CJK",
        ),
        "h1": ParagraphStyle(
            "H1",
            parent=sample["Heading1"],
            fontName="STHeiti-Medium",
            fontSize=23,
            leading=30,
            textColor=INK,
            spaceAfter=6 * mm,
            wordWrap="CJK",
        ),
        "h2": ParagraphStyle(
            "H2",
            parent=sample["Heading2"],
            fontName="STHeiti-Medium",
            fontSize=17,
            leading=23,
            textColor=INK,
            spaceBefore=5 * mm,
            spaceAfter=3.2 * mm,
            keepWithNext=True,
            wordWrap="CJK",
        ),
        "h3": ParagraphStyle(
            "H3",
            parent=sample["Heading3"],
            fontName="STHeiti-Medium",
            fontSize=11.5,
            leading=16,
            textColor=ORANGE_DARK,
            spaceBefore=3.5 * mm,
            spaceAfter=1.7 * mm,
            keepWithNext=True,
            wordWrap="CJK",
        ),
        "body": ParagraphStyle(
            "Body",
            parent=sample["BodyText"],
            fontName="STHeiti-Light",
            fontSize=9.6,
            leading=15.5,
            textColor=INK,
            spaceAfter=2.3 * mm,
            splitLongWords=True,
            wordWrap="CJK",
            allowWidows=0,
            allowOrphans=0,
        ),
        "bullet": ParagraphStyle(
            "Bullet",
            parent=sample["BodyText"],
            fontName="STHeiti-Light",
            fontSize=9.3,
            leading=14.5,
            textColor=INK,
            leftIndent=5 * mm,
            firstLineIndent=-3.5 * mm,
            bulletIndent=0,
            spaceAfter=1.2 * mm,
            wordWrap="CJK",
        ),
        "callout": ParagraphStyle(
            "Callout",
            parent=sample["BodyText"],
            fontName="STHeiti-Medium",
            fontSize=9.2,
            leading=14.5,
            textColor=colors.HexColor("#432115"),
            borderColor=ORANGE,
            borderWidth=0.8,
            borderPadding=(3.5 * mm, 4 * mm, 3.5 * mm, 4 * mm),
            backColor=colors.HexColor("#F4DFD2"),
            spaceBefore=2 * mm,
            spaceAfter=3 * mm,
            wordWrap="CJK",
        ),
        "caption": ParagraphStyle(
            "Caption",
            parent=sample["BodyText"],
            fontName="STHeiti-Light",
            fontSize=7.7,
            leading=11,
            textColor=MUTED,
            alignment=TA_CENTER,
            spaceBefore=1.2 * mm,
            spaceAfter=4 * mm,
            wordWrap="CJK",
        ),
        "toc_title": ParagraphStyle(
            "TocTitle",
            parent=sample["Heading1"],
            fontName="STHeiti-Medium",
            fontSize=22,
            leading=29,
            textColor=INK,
            spaceAfter=7 * mm,
            wordWrap="CJK",
        ),
        "toc": ParagraphStyle(
            "Toc",
            parent=sample["BodyText"],
            fontName="STHeiti-Light",
            fontSize=10.5,
            leading=19,
            textColor=INK,
            borderColor=colors.HexColor("#D8CFC2"),
            borderWidth=0,
            borderPadding=0,
            spaceAfter=0.8 * mm,
            wordWrap="CJK",
        ),
        "table_header": ParagraphStyle(
            "TableHeader",
            parent=sample["BodyText"],
            fontName="STHeiti-Medium",
            fontSize=8.2,
            leading=11.5,
            textColor=WHITE,
            wordWrap="CJK",
        ),
        "table_cell": ParagraphStyle(
            "TableCell",
            parent=sample["BodyText"],
            fontName="STHeiti-Light",
            fontSize=7.9,
            leading=11.8,
            textColor=INK,
            wordWrap="CJK",
        ),
    }


@dataclass
class MarkdownImage:
    alt: str
    path: Path


def read_markdown() -> tuple[str, list[str]]:
    text = SOURCE.read_text(encoding="utf-8")
    title_match = re.search(r"^#\s+(.+)$", text, re.MULTILINE)
    if not title_match:
        fail(f"{SOURCE} 缺少一級標題")
    headings = re.findall(r"^##\s+(.+)$", text, re.MULTILINE)
    if not headings:
        fail(f"{SOURCE} 缺少章節標題")
    return text, headings


def scaled_image(path: Path, alt: str, styles: dict[str, ParagraphStyle]) -> list[Flowable]:
    with PILImage.open(path) as bitmap:
        pixel_w, pixel_h = bitmap.size
    if pixel_w < 1000:
        fail(f"圖片寬度低於 1000 px：{path} ({pixel_w} px)")
    max_w = CONTENT_W
    max_h = 116 * mm
    scale = min(max_w / pixel_w, max_h / pixel_h)
    image = Image(str(path), width=pixel_w * scale, height=pixel_h * scale)
    image.hAlign = "CENTER"
    return [image, Paragraph(f"圖｜{inline_markup(alt)}", styles["caption"])]


def parse_table(lines: list[str], start: int, styles: dict[str, ParagraphStyle]) -> tuple[Table, int]:
    rows: list[list[str]] = []
    index = start
    while index < len(lines) and lines[index].strip().startswith("|"):
        row = [cell.strip() for cell in lines[index].strip().strip("|").split("|")]
        rows.append(row)
        index += 1
    if len(rows) >= 2 and all(re.fullmatch(r":?-{3,}:?", cell.replace(" ", "")) for cell in rows[1]):
        rows.pop(1)
    width = max(len(row) for row in rows)
    rows = [row + [""] * (width - len(row)) for row in rows]
    data = []
    for row_index, row in enumerate(rows):
        style = styles["table_header"] if row_index == 0 else styles["table_cell"]
        data.append([Paragraph(inline_markup(cell), style) for cell in row])
    col_widths = [CONTENT_W / width] * width
    table = Table(data, colWidths=col_widths, repeatRows=1, hAlign="LEFT")
    table.setStyle(
        TableStyle(
            [
                ("BACKGROUND", (0, 0), (-1, 0), CHARCOAL),
                ("TEXTCOLOR", (0, 0), (-1, 0), WHITE),
                ("BACKGROUND", (0, 1), (-1, -1), colors.HexColor("#EEE8DE")),
                ("ROWBACKGROUNDS", (0, 1), (-1, -1), [colors.HexColor("#F4F0E8"), colors.HexColor("#EDE6DC")]),
                ("GRID", (0, 0), (-1, -1), 0.35, colors.HexColor("#CFC5B7")),
                ("VALIGN", (0, 0), (-1, -1), "TOP"),
                ("LEFTPADDING", (0, 0), (-1, -1), 3 * mm),
                ("RIGHTPADDING", (0, 0), (-1, -1), 3 * mm),
                ("TOPPADDING", (0, 0), (-1, -1), 2.2 * mm),
                ("BOTTOMPADDING", (0, 0), (-1, -1), 2.2 * mm),
            ]
        )
    )
    return table, index


def markdown_story(text: str, styles: dict[str, ParagraphStyle]) -> list[Flowable]:
    lines = text.splitlines()
    story: list[Flowable] = []
    paragraph_buffer: list[str] = []

    def flush_paragraph() -> None:
        if paragraph_buffer:
            story.append(Paragraph(inline_markup(" ".join(paragraph_buffer)), styles["body"]))
            paragraph_buffer.clear()

    index = 0
    seen_h1 = False
    while index < len(lines):
        raw = lines[index]
        line = raw.strip()
        if not line:
            flush_paragraph()
            index += 1
            continue
        image_match = re.fullmatch(r"!\[([^\]]*)\]\(([^)]+)\)", line)
        if image_match:
            flush_paragraph()
            image_path = (SOURCE.parent / image_match.group(2)).resolve()
            if not image_path.is_file():
                fail(f"Markdown 圖片不存在：{image_path}")
            story.extend(scaled_image(image_path, image_match.group(1), styles))
        elif line.startswith("# "):
            flush_paragraph()
            seen_h1 = True
        elif line.startswith("## "):
            flush_paragraph()
            heading = line[3:].strip()
            if heading.startswith("12."):
                story.append(PageBreak())
            else:
                story.append(CondPageBreak(48 * mm))
            story.append(
                KeepTogether(
                    [
                        HRFlowable(width=14 * mm, thickness=2, color=ORANGE, hAlign="LEFT", spaceAfter=3 * mm),
                        Paragraph(inline_markup(heading), styles["h2"]),
                    ]
                )
            )
        elif line.startswith("### "):
            flush_paragraph()
            story.append(Paragraph(inline_markup(line[4:]), styles["h3"]))
        elif line.startswith(">"):
            flush_paragraph()
            callout = line.lstrip("> ").strip()
            index += 1
            while index < len(lines) and lines[index].strip().startswith(">"):
                callout += " " + lines[index].strip().lstrip("> ").strip()
                index += 1
            story.append(Paragraph(inline_markup(callout), styles["callout"]))
            continue
        elif re.match(r"^[-*]\s+", line):
            flush_paragraph()
            story.append(Paragraph(inline_markup(re.sub(r"^[-*]\s+", "", line)), styles["bullet"], bulletText="•"))
        elif re.match(r"^\d+\.\s+", line):
            flush_paragraph()
            number, item = line.split(".", 1)
            story.append(Paragraph(inline_markup(item.strip()), styles["bullet"], bulletText=f"{number}."))
        elif line.startswith("|") and index + 1 < len(lines) and lines[index + 1].strip().startswith("|"):
            flush_paragraph()
            table, index = parse_table(lines, index, styles)
            story.extend([table, Spacer(1, 3 * mm)])
            continue
        elif line in ("---", "***"):
            flush_paragraph()
            story.append(HRFlowable(width="100%", thickness=0.6, color=colors.HexColor("#D8CFC2"), spaceBefore=2 * mm, spaceAfter=3 * mm))
        else:
            paragraph_buffer.append(line)
        index += 1
    flush_paragraph()
    if not seen_h1:
        fail("Markdown 內容缺少一級標題")
    return story


def cover_story(styles: dict[str, ParagraphStyle]) -> list[Flowable]:
    return [
        Spacer(1, 42 * mm),
        Paragraph("VOXLINE", styles["cover_brand"]),
        Paragraph("2.0<br/>使用說明書", styles["cover_title"]),
        Paragraph("完整人聲效果鏈 · VST3<br/>繁體中文版", styles["cover_subtitle"]),
        Spacer(1, 74 * mm),
        Table(
            [[Paragraph("深色介面", styles["cover_subtitle"]), Paragraph("精準控制", styles["cover_subtitle"]), Paragraph("跨平台", styles["cover_subtitle"])]],
            colWidths=[CONTENT_W / 3] * 3,
            style=TableStyle(
                [
                    ("TEXTCOLOR", (0, 0), (-1, -1), WHITE),
                    ("VALIGN", (0, 0), (-1, -1), "MIDDLE"),
                    ("ALIGN", (0, 0), (-1, -1), "LEFT"),
                    ("LINEABOVE", (0, 0), (-1, -1), 0.7, colors.HexColor("#474A47")),
                    ("TOPPADDING", (0, 0), (-1, -1), 4 * mm),
                    ("LEFTPADDING", (0, 0), (-1, -1), 0),
                ]
            ),
        ),
        PageBreak(),
    ]


def toc_story(headings: list[str], styles: dict[str, ParagraphStyle]) -> list[Flowable]:
    items: list[Flowable] = [
        HRFlowable(width=14 * mm, thickness=2, color=ORANGE, hAlign="LEFT", spaceAfter=4 * mm),
        Paragraph("目錄", styles["toc_title"]),
    ]
    for heading in headings:
        number_match = re.match(r"^(\d+)\.\s*(.*)$", heading)
        if number_match:
            label = f'<font color="#A63F1D"><b>{number_match.group(1).zfill(2)}</b></font>　{inline_markup(number_match.group(2))}'
        else:
            label = inline_markup(heading)
        items.append(Paragraph(label, styles["toc"]))
    items.extend(
        [
            Spacer(1, 8 * mm),
            Paragraph(
                "本手冊畫面以 VOXLINE 2.0 深色介面為準。不同宿主軟體的插件清單與視窗外框可能略有差異。",
                styles["callout"],
            ),
            PageBreak(),
        ]
    )
    return items


def closing_story(styles: dict[str, ParagraphStyle]) -> list[Flowable]:
    title = ParagraphStyle(
        "ClosingTitle",
        parent=styles["h2"],
        fontSize=18,
        leading=24,
        textColor=WHITE,
        spaceBefore=0,
        spaceAfter=2 * mm,
    )
    copy = ParagraphStyle(
        "ClosingCopy",
        parent=styles["body"],
        fontSize=9,
        leading=14,
        textColor=colors.HexColor("#C8C4BD"),
        spaceAfter=0,
    )
    panel = Table(
        [
            [Paragraph("讓每一句人聲，都更靠近你想要的樣子。", title)],
            [Paragraph("更新、下載與問題回報：github.com/sd12504/Voxline", copy)],
        ],
        colWidths=[CONTENT_W],
        style=TableStyle(
            [
                ("BACKGROUND", (0, 0), (-1, -1), CHARCOAL),
                ("BOX", (0, 0), (-1, -1), 0.8, colors.HexColor("#303330")),
                ("LINEBEFORE", (0, 0), (0, -1), 3, ORANGE),
                ("LEFTPADDING", (0, 0), (-1, -1), 7 * mm),
                ("RIGHTPADDING", (0, 0), (-1, -1), 7 * mm),
                ("TOPPADDING", (0, 0), (-1, 0), 7 * mm),
                ("BOTTOMPADDING", (0, -1), (-1, -1), 7 * mm),
            ]
        ),
    )
    return [Spacer(1, 12 * mm), panel]


def build() -> None:
    validate_inputs()
    register_fonts()
    text, headings = read_markdown()
    styles = make_styles()
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)

    doc = BaseDocTemplate(
        str(OUTPUT),
        pagesize=A4,
        leftMargin=MARGIN_X,
        rightMargin=MARGIN_X,
        topMargin=MARGIN_TOP,
        bottomMargin=MARGIN_BOTTOM,
        title="VOXLINE 2.0 使用說明書",
        author="VOXLINE",
        subject="VOXLINE 2.0 VST3 人聲效果器繁體中文使用說明",
        pageCompression=1,
    )
    cover_frame = Frame(MARGIN_X, MARGIN_BOTTOM, CONTENT_W, CONTENT_H, id="cover", showBoundary=0)
    body_frame = Frame(MARGIN_X, MARGIN_BOTTOM, CONTENT_W, CONTENT_H, id="body", showBoundary=0)
    doc.addPageTemplates(
        [
            PageTemplate(id="cover", frames=[cover_frame], onPage=draw_cover_page, autoNextPageTemplate="body"),
            PageTemplate(id="body", frames=[body_frame], onPage=draw_body_page),
        ]
    )
    story = cover_story(styles)
    story.extend(toc_story(headings, styles))
    story.extend(markdown_story(text, styles))
    story.extend(closing_story(styles))
    doc.build(story)
    print(f"已產生：{OUTPUT}")


if __name__ == "__main__":
    build()
