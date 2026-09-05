#!/usr/bin/env python3
"""Create a five-layer Cricut Design Space validation sample.

This is intentionally a standalone prototype. It consumes the existing per-layer
SVG output and does not define the production page-export API.
"""

from __future__ import annotations

import argparse
import re
import xml.etree.ElementTree as ET
from pathlib import Path


NUMBER = re.compile(r"[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?")


def parse_layer(path: Path) -> tuple[float, float, float, float, list[str]]:
    root = ET.parse(path).getroot()
    values = [float(value) for value in root.attrib["viewBox"].split()]
    min_x, min_y, width, height = values
    paths = [element.attrib["d"] for element in root.iter()
             if element.tag.rsplit("}", 1)[-1] == "path"]
    return min_x, min_y, width, height, paths


def transform_path(path: str, source_min_x: float, source_min_y: float,
                   target_x: float, target_y: float, scale: float = 1.0,
                   center_x: float = 0.0, center_y: float = 0.0) -> str:
    numbers = list(NUMBER.finditer(path))
    output: list[str] = []
    cursor = 0
    coordinate = 0
    for match in numbers:
        output.append(path[cursor:match.start()])
        value = float(match.group())
        if coordinate % 2 == 0:
            value = center_x + (value - center_x) * scale
            value += target_x - source_min_x
        else:
            value = center_y + (value - center_y) * scale
            value += target_y - source_min_y
        output.append(f"{value:.9f}")
        cursor = match.end()
        coordinate += 1
    output.append(path[cursor:])
    return "".join(output)


def group(name: str, paths: list[str], fill: str, title: str | None = None,
          **attributes: str) -> str:
    metadata = " ".join(f'{key}="{value}"' for key, value in attributes.items())
    label = title or name
    lines = [f'  <g id="{name}" fill="{fill}" fill-rule="evenodd" '
             f'stroke="none" {metadata}>', f"    <title>{label}</title>"]
    lines.extend(f'    <path d="{path}"/>' for path in paths)
    lines.append("  </g>")
    return "\n".join(lines)


def document(width: float, height: float, groups: list[str], kind: str) -> str:
    body = "\n".join(groups)
    return ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            f'<svg xmlns="http://www.w3.org/2000/svg" width="{width:.9f}mm" '
            f'height="{height:.9f}mm" viewBox="0 0 {width:.9f} {height:.9f}" '
            f'data-cricut-prototype="true" data-output="{kind}">\n'
            f"{body}\n</svg>\n")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input-dir", type=Path, default=Path("output"))
    parser.add_argument("--output-dir", type=Path,
                        default=Path("output/cricut-sample"))
    args = parser.parse_args()

    layers = []
    for index in range(5):
        path = args.input_dir / f"layer_{index:03d}.svg"
        if not path.is_file():
            parser.error(f"missing input layer: {path}")
        layers.append(parse_layer(path))

    page_width = page_height = 304.0
    border = 7.0
    gap = 3.0
    source_min_x, source_min_y, tile_width, tile_height, _ = layers[0]
    for layer in layers[1:]:
        if abs(layer[2] - tile_width) > 1e-6 or abs(layer[3] - tile_height) > 1e-6:
            raise SystemExit("prototype requires equal source layer dimensions")

    args.output_dir.mkdir(parents=True, exist_ok=True)
    cut_groups = []
    for index, (min_x, min_y, width, height, paths) in enumerate(layers):
        column, row = index % 3, index // 3
        target_x = border + column * (tile_width + gap)
        target_y = border + row * (tile_height + gap)
        transformed = [transform_path(path, min_x, min_y, target_x, target_y)
                       for path in paths]
        cut_groups.append(group(f"cut-layer-{index:03d}", transformed, "#222222",
                                 title=f"CUT | Layer {index:03d}",
                                 **{"data-layer-index": str(index)}))

    guide_groups = []
    for index in range(1, len(layers)):
        min_x, min_y, width, height, paths = layers[index]
        center_x, center_y = min_x + width / 2, min_y + height / 2
        column, row = (index - 1) % 3, (index - 1) // 3
        target_x = border + column * (tile_width + gap)
        target_y = border + row * (tile_height + gap)
        transformed = [transform_path(path, min_x, min_y, target_x, target_y,
                                       0.95, center_x, center_y)
                       for path in paths]
        guide_groups.append(group(
            f"pen-guide-{index:03d}-over-{index - 1:03d}", transformed, "#1769aa",
            title=f"PEN GUIDE | Layer {index:03d} over Layer {index - 1:03d} | Prototype",
            **{"data-guide-layer": str(index),
               "data-previous-layer": str(index - 1)}))

    cut_path = args.output_dir / "page_001_layers_000-004.cut.svg"
    guide_path = args.output_dir / "page_001_layers_000-004.guide.svg"
    cut_path.write_text(document(page_width, page_height, cut_groups, "cut"),
                        encoding="utf-8")
    guide_path.write_text(document(page_width, page_height, guide_groups, "guide"),
                          encoding="utf-8")
    print(cut_path)
    print(guide_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
