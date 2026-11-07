#!/usr/bin/env python3
"""Generate the zh-CN catalog and the embedded UI font subset.

The English overlay strings stay the lookup keys. This script checks printf
conversions, forbids '#' in translations, and writes:

  OptiScaler/menu/locale/zh_cn_catalog.inc
  OptiScaler/menu/locale/zh_cn_font_subset.h
  a coverage report on stdout and, when --report is set, a text file.
"""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
MENU = ROOT / "OptiScaler" / "menu" / "menu_common.cpp"
CATALOG = ROOT / "OptiScaler" / "menu" / "locale" / "zh_cn_catalog.inc"
FONT_HEADER = ROOT / "OptiScaler" / "menu" / "locale" / "zh_cn_font_subset.h"
MAP_PATH = ROOT / "tools" / "zh_cn_map.json"
NOTO = pathlib.Path("/tmp/fonts/NotoSansSC.ttf")

SPEC = re.compile(
    r"%(?:%|[-+ #0]*(?:\d+|\*)?(?:\.(?:\d+|\*))?(?:hh|h|ll|l|j|z|t|L)?[diuoxXfFeEgGaAcspn])"
)

# Shown in English on purpose: api names, backend ids, preset letters, D3D states,
# Reflex markers whose printf width is ASCII, and pure id/format tokens.
SKIP = {
    "",
    "R",
    "DLSS",
    "XeSS",
    "DLAA",
    "HDR",
    "RCAS",
    "MFG",
    "DLSSG",
    "FSR",
    "XeFG",
    "FSR1",
    "FSR 2.1.2",
    "FSR 2.2.1",
    "FSR 3.X",
    "FSR 3.1 FG",
    "FSR FG",
    "FSR3/4 FG",
    "KPSS",
    "SPLAT",
    "MODEL_3",
    "MODEL_4",
    "MODEL_5",
    "MODEL_6",
    "2X",
    "3X",
    "4X",
    "5X",
    "6X",
    "RTV",
    "SRV",
    "UAV",
    "OM",
    "Ups",
    "SCR",
    "SGR",
    "DI",
    "DII",
    "Disp",
    "VKD3D",
    "VLK",
    "D3D11",
    "D3D12",
    "fakenvapi",
    "AntiLag 2",
    "LatencyFlex",
    "XeLL",
    "Reflex",
    "Vulkan AntiLag",
    " (RTSS)",
    "Lanczos2",
    "Lanczos3",
    "Kaiser2",
    "Kaiser3",
    "Catmull-Rom",
    "MAGIC",
    "Bicubic",
    "Simulation",
    "RenderSubmit",
    "Present",
    "Driver",
    "RenderQueue",
    "GpuRender",
    "%-12s %4.1fms",
    "(Q)",
    "(OP)",
    "(DXVK) ",
    "(?) ",
    "(?)",
    " | ",
    "- %s",
    "%s - %s %s %s %s",
    "dlss",
    "dlssd",
    "DLSSD",
    "xess",
    "xess_12",
    "fsr",
    "fsr21",
    "fsr22",
    "fsr31",
    "fsr21_12",
    "fsr22_12",
    "fsr31_12",
    "main",
    "fgSelection",
    "plots",
    "init",
    "init2",
    "drs",
    "HudlessTable",
    "##FrameTimeGraph",
    "##UpscalerFrameTimeGraph",
    "https://github.com/optiscaler/OptiScaler/wiki",
    "AUTO",
    "COMMON",
    "VERTEX_AND_CONSTANT_BUFFER",
    "INDEX_BUFFER",
    "RENDER_TARGET",
    "UNORDERED_ACCESS",
    "DEPTH_WRITE",
    "DEPTH_READ",
    "NON_PIXEL_SHADER_RESOURCE",
    "PIXEL_SHADER_RESOURCE",
    "STREAM_OUT",
    "INDIRECT_ARGUMENT",
    "COPY_DEST",
    "COPY_SOURCE",
    "RESOLVE_DEST",
    "RESOLVE_SOURCE",
    "RAYTRACING_ACCELERATION_STRUCTURE",
    "SHADING_RATE_SOURCE",
    "GENERIC_READ",
    "ALL_SHADER_RESOURCE",
    "PRESENT",
    "PREDICATION",
    "VIDEO_DECODE_READ",
    "VIDEO_DECODE_WRITE",
    "VIDEO_PROCESS_READ",
    "VIDEO_PROCESS_WRITE",
    "VIDEO_ENCODE_READ",
    "VIDEO_ENCODE_WRITE",
    "Menu key pressed, will be switching menu",
    "Menu key pressed, will be switching FPS",
    "Menu key pressed, will be switching FG mode",
    "Menu key pressed, will be switching FPS mode",
    "en",
    "zh-CN",
    "zh-cn",
    "pch.h",
    "menu_common.h",
    "font/Hack_Compressed.h",
    "user32.dll",
    "SetPhysicalCursorPos",
    "SetCursorPos",
    "ClipCursor",
    "mouse_event",
    "SendInput",
    "SendMessageW",
    "dlssg_to_fsr3_amd_is_better.dll",
}


def specs(text: str):
    found = []
    i = 0
    while i < len(text):
        if text[i] != "%":
            i += 1
            continue
        match = SPEC.match(text, i)
        if not match:
            return None
        found.append(match.group(0))
        i = match.end()
    return found


def skip_ws_comments(s, i):
    n = len(s)
    while i < n:
        if s[i] in " \t\r\n":
            i += 1
            continue
        if s.startswith("//", i):
            j = s.find("\n", i)
            i = n if j < 0 else j + 1
            continue
        if s.startswith("/*", i):
            j = s.find("*/", i + 2)
            i = n if j < 0 else j + 2
            continue
        break
    return i


def parse_string(s, i):
    parts = []
    n = len(s)
    while True:
        j = i + 1
        buf = []
        while j < n:
            if s[j] == "\\":
                esc = s[j + 1]
                mapping = {"n": "\n", "t": "\t", "r": "\r", "\\": "\\", '"': '"', "'": "'"}
                buf.append(mapping.get(esc, esc))
                j += 2
                continue
            if s[j] == '"':
                j += 1
                break
            buf.append(s[j])
            j += 1
        parts.append("".join(buf))
        k = skip_ws_comments(s, j)
        if k < n and s[k] == '"':
            i = k
            continue
        return "".join(parts), j


CALLS = [
    "ShowHelpMarker",
    "ShowTooltip",
    "SeparatorWithHelpMarker",
    "ScopedCollapsingHeader",
    "ImGui::TextColored",
    "ImGui::TextDisabled",
    "ImGui::TextWrapped",
    "ImGui::TextLinkOpenURL",
    "ImGui::Text",
    "ImGui::Button",
    "ImGui::Checkbox",
    "ImGui::RadioButton",
    "ImGui::BeginCombo",
    "ImGui::Selectable",
    "ImGui::SeparatorText",
    "ImGui::SliderFloat",
    "ImGui::SliderInt",
    "ImGui::InputFloat",
    "ImGui::InputInt",
    "ImGui::InputScalar",
    "ImGui::TreeNode",
    "ImGui::ColorEdit3",
    "ImGui::Combo",
    "ImGui::SetTooltip",
    "ImGui::Begin",
    "ImGui::CalcTextSize",
    "PopulateCombo",
    "AddDLSSRenderPreset",
    "AddDLSSDRenderPreset",
    "AddResourceBarrier",
    "StrFmt",
    "Keybind",
]


def extract_call_strings(src: str) -> set[str]:
    names = sorted(CALLS, key=len, reverse=True)
    ui = set()
    i = 0
    n = len(src)
    while i < n:
        if src.startswith("//", i):
            j = src.find("\n", i)
            i = n if j < 0 else j + 1
            continue
        if src.startswith("/*", i):
            j = src.find("*/", i + 2)
            i = n if j < 0 else j + 2
            continue
        if src[i] == '"':
            _, i = parse_string(src, i)
            continue
        matched = None
        for name in names:
            if src.startswith(name, i) and (i == 0 or not (src[i - 1].isalnum() or src[i - 1] == "_")):
                after = i + len(name)
                if after < n and (src[after].isalnum() or src[after] == "_"):
                    continue
                j = skip_ws_comments(src, after)
                if j < n and src[j] == "(":
                    matched = name
                    i = j + 1
                    break
        if not matched:
            i += 1
            continue
        depth = 1
        while i < n and depth:
            i = skip_ws_comments(src, i)
            if i >= n:
                break
            if src[i] == '"' and depth == 1:
                text, i = parse_string(src, i)
                ui.add(text)
                continue
            c = src[i]
            if c == "(":
                depth += 1
                i += 1
                continue
            if c == ")":
                depth -= 1
                i += 1
                continue
            if c in "{[":
                open_c = c
                close = "]" if c == "[" else "}"
                d = 1
                i += 1
                while i < n and d:
                    if src.startswith("//", i):
                        j = src.find("\n", i)
                        i = n if j < 0 else j + 1
                        continue
                    if src[i] == '"':
                        _, i = parse_string(src, i)
                        continue
                    if src[i] == open_c:
                        d += 1
                    elif src[i] == close:
                        d -= 1
                    i += 1
                continue
            i += 1
    return ui


def extract_literals(src: str) -> set[str]:
    lines = src.splitlines()
    found = set()
    i = 0
    n = len(src)
    while i < n:
        if src.startswith("//", i):
            j = src.find("\n", i)
            i = n if j < 0 else j + 1
            continue
        if src.startswith("/*", i):
            j = src.find("*/", i + 2)
            i = n if j < 0 else j + 2
            continue
        if src[i] == '"':
            line = src.count("\n", 0, i) + 1
            text, i = parse_string(src, i)
            raw = lines[line - 1]
            if "LOG_" in raw:
                continue
            found.add(text)
        else:
            i += 1
    return found


def visible(text: str) -> str:
    pos = text.find("##")
    return text if pos < 0 else text[:pos]


def looks_technical_token(text: str) -> bool:
    if text in SKIP:
        return True
    if not any(c.isalpha() for c in text):
        return True
    if text.startswith("##") and not any(c.isalpha() for c in visible(text)):
        return True
    if re.fullmatch(r"[%\d. +\-()\[\]|:/\\dxulfs*#]*", text):
        return True
    if text.startswith("http://") or text.startswith("https://"):
        return True
    if text.endswith(".h") or text.endswith(".hpp") or text.endswith(".cpp"):
        return True
    if "{" in text or "0:X" in text or "hWnd" in text:
        return True
    return False


def cpp_string(text: str) -> str:
    return json.dumps(text, ensure_ascii=False)


def load_map() -> dict[str, str]:
    data = json.loads(MAP_PATH.read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise SystemExit("zh_cn_map.json must be an object")
    return {str(k): str(v) for k, v in data.items()}


def validate(mapping: dict[str, str]) -> None:
    errors = []
    for english, chinese in mapping.items():
        if not chinese or not chinese.strip():
            errors.append(f"empty translation for {english!r}")
            continue
        if "#" in chinese:
            errors.append(f"'#' in translation for {english!r}")
        left = specs(visible(english))
        right = specs(chinese)
        if left is None or right is None or left != right:
            errors.append(f"printf mismatch for {english!r}: {left} vs {right}")
    if errors:
        print("\n".join(errors), file=sys.stderr)
        raise SystemExit(f"{len(errors)} translation error(s)")


def write_catalog(mapping: dict[str, str]) -> None:
    items = sorted(mapping.items(), key=lambda kv: kv[0].encode("utf-8"))
    lines = ["/* Generated by tools/gen_zh_cn_catalog.py. Unofficial zh-CN overlay. Do not edit by hand. */"]
    for english, chinese in items:
        lines.append(f"    {{{cpp_string(english)}, {cpp_string(chinese)}}},")
    CATALOG.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_font(mapping: dict[str, str]) -> None:
    if not NOTO.exists():
        print(f"font source missing: {NOTO}", file=sys.stderr)
        return
    from fontTools.ttLib import TTFont
    from fontTools.varLib.instancer import instantiateVariableFont
    from fontTools.subset import Subsetter, Options

    chars = set()
    for chinese in mapping.values():
        chars.update(chinese)
    extra = "，。！？、；：（）【】《》「」…—·×“”‘’"
    chars.update(extra)
    text = "".join(sorted(chars))

    font = TTFont(str(NOTO))
    static = instantiateVariableFont(font, {"wght": 400}, inplace=False, overlap=True)
    options = Options()
    options.layout_features = ["kern"]
    options.hinting = False
    options.desubroutinize = True
    options.notdef_outline = True
    options.recommended_glyphs = True
    options.name_IDs = ["*"]
    options.name_legacy = True
    options.name_languages = ["*"]
    options.drop_tables = ["DSIG"]
    subsetter = Subsetter(options=options)
    subsetter.populate(text=text)
    subsetter.subset(static)

    def set_name(name_id, value):
        static["name"].setName(value, name_id, 3, 1, 0x409)

    set_name(1, "OptiScaler UI SC")
    set_name(2, "Regular")
    set_name(3, "OptiScalerUISC-Regular")
    set_name(4, "OptiScaler UI SC")
    set_name(6, "OptiScalerUISC")
    set_name(16, "OptiScaler UI SC")
    set_name(17, "Regular")
    set_name(25, "OptiScalerUISC")

    tmp = pathlib.Path("/tmp/OptiScalerUISC.ttf")
    static.save(str(tmp))
    data = tmp.read_bytes()
    if data[:4] not in (b"\x00\x01\x00\x00", b"true", b"OTTO"):
        raise SystemExit(f"unexpected font signature {data[:4]!r}")

    lines = [
        "#pragma once",
        "// Unofficial UI subset of Noto Sans SC (SIL OFL 1.1).",
        "// Copyright (c) 2014-2021 Adobe, with Reserved Font Name 'Source'.",
        "// This subset is not named Source. License: https://scripts.sil.org/OFL",
        "static const unsigned char ZhCnFontSubset[] = {",
    ]
    row = []
    for i, byte in enumerate(data):
        row.append(f"0x{byte:02x}")
        if len(row) == 16:
            lines.append("    " + ", ".join(row) + ",")
            row = []
    if row:
        lines.append("    " + ", ".join(row) + ",")
    lines.append("};")
    lines.append("static const unsigned int ZhCnFontSubsetSize = sizeof(ZhCnFontSubset);")
    FONT_HEADER.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"embedded font {len(data)} bytes, {len(chars)} codepoints")


def coverage(mapping: dict[str, str], src: str) -> tuple[list[str], list[str], list[str]]:
    calls = extract_call_strings(src)
    literals = extract_literals(src)
    required = set()
    for text in calls | literals:
        if looks_technical_token(text):
            continue
        stem = visible(text)
        if not stem or looks_technical_token(stem):
            continue
        required.add(stem)
    translated = []
    missing = []
    for text in sorted(required):
        if text in mapping:
            translated.append(text)
        else:
            missing.append(text)
    intentional = sorted(t for t in (calls | literals) if looks_technical_token(t) or (visible(t) in SKIP))
    return translated, missing, intentional


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--report", type=pathlib.Path)
    parser.add_argument("--skip-font", action="store_true")
    args = parser.parse_args()

    mapping = load_map()
    validate(mapping)
    write_catalog(mapping)
    if not args.skip_font:
        write_font(mapping)

    src = MENU.read_text(encoding="utf-8")
    translated, missing, intentional = coverage(mapping, src)
    total = len(translated) + len(missing)
    pct = (100.0 * len(translated) / total) if total else 100.0
    summary = [
        f"catalog entries: {len(mapping)}",
        f"ui strings translated: {len(translated)}/{total} ({pct:.1f}%)",
        f"intentionally English technical tokens: {len(intentional)}",
        f"missing: {len(missing)}",
    ]
    if missing:
        summary.append("MISSING:")
        summary.extend(repr(item) for item in missing)
    summary.append("INTENTIONALLY_ENGLISH:")
    summary.extend(repr(item) for item in intentional)
    text = "\n".join(summary) + "\n"
    print(text)
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(text, encoding="utf-8")
    if missing:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
