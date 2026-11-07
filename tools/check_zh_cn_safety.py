#!/usr/bin/env python3
"""Static checks for the unofficial zh-CN overlay.

This does not compile OptiScaler.dll and does not exercise the in-game menu.
"""

from __future__ import annotations

import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
MAP_PATH = ROOT / "tools" / "zh_cn_map.json"
CATALOG = ROOT / "OptiScaler" / "menu" / "locale" / "zh_cn_catalog.inc"
MENU = ROOT / "OptiScaler" / "menu" / "menu_common.cpp"
VCXPROJ = ROOT / "OptiScaler" / "OptiScaler.vcxproj"
FONT = pathlib.Path("/tmp/OptiScalerUISC.ttf")

SPEC = re.compile(
    r"%(?:%|[-+ #0]*(?:\d+|\*)?(?:\.(?:\d+|\*))?(?:hh|h|ll|l|j|z|t|L)?[diuoxXfFeEgGaAcspn])"
)

RAW_WIDGET = re.compile(
    r"ImGui::(?:TextColored|TextDisabled|TextWrapped|TextLinkOpenURL|SetTooltip|BeginCombo|"
    r"RadioButton|SliderFloat|SliderInt|InputFloat|InputInt|InputScalar|ColorEdit3|"
    r"SeparatorText|CalcTextSize|Selectable|Checkbox|TreeNode|Button|Combo|Begin|Text)\("
)


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


def visible(text: str) -> str:
    pos = text.find("##")
    return text if pos < 0 else text[:pos]


def load_catalog() -> dict[str, str]:
    mapping = {}
    for line in CATALOG.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line.startswith("{"):
            continue
        pair = json.loads("[" + line[1:-2] + "]")
        mapping[pair[0]] = pair[1]
    return mapping


def cmap_codepoints(path: pathlib.Path) -> set[int]:
    from fontTools.ttLib import TTFont

    font = TTFont(str(path))
    points = set()
    for table in font["cmap"].tables:
        points.update(table.cmap)
    return points


def main() -> None:
    errors = []
    source_map = json.loads(MAP_PATH.read_text(encoding="utf-8"))
    catalog = load_catalog()
    if source_map != catalog:
        errors.append(
            f"map/catalog mismatch: map {len(source_map)} catalog {len(catalog)} "
            f"only_map {len(set(source_map) - set(catalog))} only_catalog {len(set(catalog) - set(source_map))}"
        )

    for english, chinese in source_map.items():
        if "#" in chinese:
            errors.append(f"hash in translation: {english!r}")
        left = specs(visible(english))
        right = specs(chinese)
        if left != right:
            errors.append(f"printf mismatch: {english!r}: {left} vs {right}")

    menu = MENU.read_text(encoding="utf-8")
    raw = RAW_WIDGET.findall(menu)
    if raw:
        errors.append(f"unwrapped ImGui UI calls remain: {len(raw)}")

    project = VCXPROJ.read_text(encoding="utf-8")
    if 'Include="menu\\locale\\ui_locale.cpp"' not in project or "/utf-8" not in project:
        errors.append("vcxproj is missing the UTF-8 locale compilation units")

    for path in (MAP_PATH, CATALOG, MENU, ROOT / "OptiScaler" / "menu" / "locale" / "ui_locale.cpp"):
        path.read_text(encoding="utf-8")

    non_ascii = set()
    for chinese in source_map.values():
        non_ascii.update(ord(ch) for ch in chinese if ord(ch) > 127)
    covered = set()
    missing_glyphs = set()
    if FONT.exists():
        covered = cmap_codepoints(FONT)
        missing_glyphs = {cp for cp in non_ascii if cp not in covered}
        if missing_glyphs:
            sample = "".join(chr(cp) for cp in sorted(missing_glyphs)[:24])
            errors.append(f"subset font misses {len(missing_glyphs)} translation codepoints, sample {sample!r}")
    else:
        errors.append(f"subset font missing: {FONT}")

    print(f"catalog entries: {len(source_map)}")
    print(f"printf and hash checks: {'ok' if not any('printf' in e or 'hash' in e for e in errors) else 'failed'}")
    print(f"unwrapped ImGui UI calls: {len(raw)}")
    print(f"non-ascii translation codepoints: {len(non_ascii)}")
    print(f"subset cmap size: {len(covered)}")
    print(f"missing glyphs: {len(missing_glyphs)}")
    if errors:
        print("FAILED")
        for error in errors:
            print(error)
        raise SystemExit(1)
    print("static localization safety checks passed")


if __name__ == "__main__":
    main()
