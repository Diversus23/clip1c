#!/usr/bin/env python3
"""Собирает AddIn.zip (бандл внешней компоненты 1С) с MANIFEST.XML.

Скрипт ищет в указанном каталоге бинарники для всех поддерживаемых платформ,
переименовывает их с версионным суффиксом, формирует MANIFEST.XML по схеме
http://v8.1c.ru/8.2/addin/bundle и упаковывает всё в один ZIP.

Пример использования в CI после сбора артефактов всех job'ов:

    python tools/make_bundle.py \\
        --artifacts-dir artifacts \\
        --output AddIn.zip
"""

from __future__ import annotations

import argparse
import re
import sys
import zipfile
from pathlib import Path
from typing import List, NamedTuple, Optional
from xml.etree import ElementTree as ET


class Component(NamedTuple):
    os_name: str          # значение атрибута os (Windows/Linux/MacOS)
    arch: str             # значение атрибута arch (i386/x86_64/Universal/...)
    src_pattern: str      # шаблон имени исходного файла (как кладёт CMake)
    out_name: str         # имя файла внутри zip (без версионного суффикса)


def default_components(name: str) -> List[Component]:
    return [
        Component("Windows", "i386",      f"{name}Win32.dll",   f"{name}Win32.dll"),
        Component("Windows", "x86_64",    f"{name}Win64.dll",   f"{name}Win64.dll"),
        Component("Linux",   "i386",      f"{name}Lin32.so",    f"{name}Lin32.so"),
        Component("Linux",   "x86_64",    f"{name}Lin64.so",    f"{name}Lin64.so"),
        Component("MacOS",   "Universal", f"{name}Mac64.dylib", f"{name}Mac.dylib"),
    ]


def read_version(version_h: Path) -> str:
    text = version_h.read_text(encoding="utf-8")
    m = re.search(r"#define\s+VERSION_FULL\s+([\d.]+)", text)
    if not m:
        sys.exit(f"VERSION_FULL не найден в {version_h}")
    return m.group(1)


def find_file(root: Path, filename: str) -> Optional[Path]:
    """Ищет файл по имени рекурсивно (актуально для download-artifact)."""
    matches = list(root.rglob(filename))
    if not matches:
        return None
    # Если файл найден в нескольких местах, берём самый свежий по mtime
    return max(matches, key=lambda p: p.stat().st_mtime)


def add_versioned_suffix(filename: str, version: str) -> str:
    postfix = "_" + version.replace(".", "-")
    stem, dot, ext = filename.rpartition(".")
    if not dot:
        return filename + postfix
    return f"{stem}{postfix}.{ext}"


def build_manifest(components: List[tuple[Component, Path, str]]) -> bytes:
    """Создаёт MANIFEST.XML по схеме http://v8.1c.ru/8.2/addin/bundle."""
    ns = "http://v8.1c.ru/8.2/addin/bundle"
    ET.register_namespace("", ns)
    bundle = ET.Element(f"{{{ns}}}bundle")
    for comp, _src, arch_name in components:
        ET.SubElement(bundle, f"{{{ns}}}component", attrib={
            "type": "native",
            "os":   comp.os_name,
            "arch": comp.arch,
            "path": arch_name,
        })
    ET.indent(bundle, space="\t")
    xml = ET.tostring(bundle, encoding="utf-8", xml_declaration=True)
    return xml


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--name", default="Clip1C",
                   help="Базовое имя бинарников (по умолчанию Clip1C)")
    p.add_argument("--source-root", default=".",
                   help="Корень репозитория для поиска version.h")
    p.add_argument("--artifacts-dir", required=True,
                   help="Каталог с собранными бинарниками (рекурсивный поиск)")
    p.add_argument("--output", default="AddIn.zip",
                   help="Имя итогового ZIP-файла")
    p.add_argument("--strict", action="store_true",
                   help="Падать с ошибкой, если не найден хотя бы один бинарник")
    args = p.parse_args()

    root = Path(args.source_root).resolve()
    artifacts = Path(args.artifacts_dir).resolve()
    if not artifacts.is_dir():
        sys.exit(f"Каталог артефактов не найден: {artifacts}")

    version = read_version(root / "version.h")
    print(f"Версия: {version}")
    print(f"Поиск артефактов в: {artifacts}")

    selected: List[tuple[Component, Path, str]] = []
    missing: List[Component] = []
    for comp in default_components(args.name):
        src = find_file(artifacts, comp.src_pattern)
        if src is None:
            missing.append(comp)
            print(f"  - {comp.os_name}/{comp.arch}: НЕ НАЙДЕНО ({comp.src_pattern})")
            continue
        archive_name = add_versioned_suffix(comp.out_name, version)
        selected.append((comp, src, archive_name))
        print(f"  + {comp.os_name}/{comp.arch}: {src} -> {archive_name}")

    if not selected:
        sys.exit("Не найдено ни одного бинарника")

    if args.strict and missing:
        sys.exit(f"strict-режим: отсутствуют {len(missing)} платформ(ы)")

    manifest = build_manifest(selected)

    output = Path(args.output).resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED) as zf:
        zf.writestr("MANIFEST.XML", manifest)
        for _comp, src, archive_name in selected:
            zf.write(src, archive_name)

    print(f"\n{output} собран ({output.stat().st_size:,} байт)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
