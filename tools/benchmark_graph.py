#!/usr/bin/env python3
"""Generate the benchmark summary SVG from benchmark output."""

from __future__ import annotations

import argparse
import html
import math
import pathlib
import re
import sys
from dataclasses import dataclass


SECTION_TITLES = {
    "scalar benchmarks": "scalar",
    "DDA line benchmarks": "line",
    "sprite scaler benchmarks": "sprite",
    "rotoscaler benchmarks (incremental inverse mapping)": "rotoscale_incremental",
    "rotoscaler benchmarks (direct inverse mapping)": "rotoscale_direct",
}

SCALAR_SUMMARY_ROWS = (
    "sum",
    "floor -> fixed",
    "multiply by int",
    "value -> float",
    "multiply",
    "float -> fixed round",
    "reciprocal -> fixed",
    "divide",
    "rational -> fixed",
)

WORKLOAD_SUMMARY_ROWS = (
    ("sprite scaler", "sprite"),
    ("rotoscaler incremental", "rotoscale_incremental"),
    ("DDA line", "line"),
    ("rotoscaler direct", "rotoscale_direct"),
)

ROW_RE = re.compile(
    r"^(?P<label>.*?)\s+"
    r"(?P<fixed>[0-9]+(?:\.[0-9]+)?)\s+"
    r"(?P<base>[0-9]+(?:\.[0-9]+)?)\s+"
    r"(?P<ratio>[0-9]+(?:\.[0-9]+)?)\s*$"
)


@dataclass(frozen=True)
class BenchmarkResult:
    section: str
    label: str
    fixed: float
    base: float
    ratio: float


@dataclass(frozen=True)
class GraphRow:
    label: str
    ratio: float


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Read fix32 benchmark output from a file or stdin and regenerate "
            "the README benchmark summary SVG."
        )
    )
    parser.add_argument(
        "input",
        nargs="?",
        default="-",
        help="benchmark output to parse, or '-' for stdin",
    )
    parser.add_argument(
        "-o",
        "--output",
        default="docs/benchmark_summary.svg",
        help="SVG output path",
    )
    parser.add_argument(
        "--source-label",
        default=None,
        help="source label written into the SVG footer",
    )
    return parser.parse_args()


def read_input(path: str) -> str:
    if path == "-":
        return sys.stdin.read()
    return pathlib.Path(path).read_text(encoding="utf-8")


def parse_results(text: str) -> list[BenchmarkResult]:
    results: list[BenchmarkResult] = []
    section: str | None = None

    for line in text.splitlines():
        stripped = line.strip()
        if stripped.startswith("## "):
            section = None
            continue
        if stripped in SECTION_TITLES:
            section = SECTION_TITLES[stripped]
            continue
        if section is None:
            continue

        match = ROW_RE.match(line)
        if match is None:
            continue

        results.append(
            BenchmarkResult(
                section=section,
                label=match.group("label").strip(),
                fixed=float(match.group("fixed")),
                base=float(match.group("base")),
                ratio=float(match.group("ratio")),
            )
        )

    return results


def mean_ratio(results: list[BenchmarkResult], section: str) -> float:
    ratios = [result.ratio for result in results if result.section == section]
    if not ratios:
        raise ValueError(f"no benchmark rows found for section {section!r}")
    return sum(ratios) / len(ratios)


def scalar_ratio(results: list[BenchmarkResult], label: str) -> float:
    for result in results:
        if result.section == "scalar" and result.label == label:
            return result.ratio
    raise ValueError(f"no scalar benchmark row found for {label!r}")


def build_graph_rows(
    results: list[BenchmarkResult],
) -> list[tuple[str, list[GraphRow]]]:
    workload_rows = [
        GraphRow(label, mean_ratio(results, section))
        for label, section in WORKLOAD_SUMMARY_ROWS
    ]
    scalar_rows = [
        GraphRow(label, scalar_ratio(results, label))
        for label in SCALAR_SUMMARY_ROWS
    ]
    return [
        ("Workloads, mean ratio", workload_rows),
        ("Scalar rows, representative operations", scalar_rows),
    ]


def color_class(ratio: float) -> str:
    if ratio < 0.95:
        return "faster"
    if ratio <= 1.05:
        return "neutral"
    return "slower"


def tick_values(max_ratio: float) -> list[float]:
    ticks: list[float] = []
    value = 0.0
    while value <= max_ratio + 0.001:
        ticks.append(value)
        value += 0.5
    if all(abs(tick - 1.0) > 0.001 for tick in ticks):
        ticks.append(1.0)
    return sorted(ticks)


def fmt_ratio(ratio: float) -> str:
    return f"{ratio:.2f}"


def svg_text(text: str) -> str:
    return html.escape(text, quote=False)


def render_svg(
    groups: list[tuple[str, list[GraphRow]]],
    source_label: str,
) -> str:
    left_margin = 40
    label_right = 260
    chart_left = 280
    chart_right = 900
    chart_width = chart_right - chart_left
    top = 120
    row_step = 33
    group_gap = 28
    row_height = 20
    footer_gap = 64

    rows = [row for _, group_rows in groups for row in group_rows]
    if not rows:
        raise ValueError("no benchmark rows found")

    max_ratio = max(2.5, max(row.ratio for row in rows))
    max_ratio = math.ceil(max_ratio * 100.0) / 100.0

    current_y = top
    group_blocks: list[str] = []
    for title, group_rows in groups:
        group_blocks.append(
            f'  <text x="{left_margin}" y="{current_y + 16}" '
            f'class="group">{svg_text(title)}</text>'
        )
        current_y += group_gap
        for row in group_rows:
            ratio_width = max(1, round((row.ratio / max_ratio) * chart_width))
            bar_class = color_class(row.ratio)
            label = svg_text(row.label)
            value = fmt_ratio(row.ratio)
            group_blocks.extend(
                [
                    f'  <text x="{label_right}" y="{current_y + 15}" '
                    f'text-anchor="end" class="label">{label}</text>',
                    f'  <rect x="{chart_left}" y="{current_y}" '
                    f'width="{ratio_width}" height="{row_height}" rx="4" '
                    f'class="{bar_class}"/>',
                    f'  <text x="{chart_left + ratio_width + 10}" '
                    f'y="{current_y + 15}" class="value">{value}</text>',
                ]
            )
            current_y += row_step
        current_y += 16

    axis_top = top
    axis_bottom = current_y - 20
    footer_y = current_y + footer_gap
    height = footer_y + 26

    axis_lines: list[str] = []
    tick_labels: list[str] = []
    for tick in tick_values(max_ratio):
        x = chart_left + round((tick / max_ratio) * chart_width)
        line_class = "parity" if abs(tick - 1.0) < 0.001 else "axis"
        axis_lines.append(
            f'  <line x1="{x}" y1="{axis_top}" x2="{x}" '
            f'y2="{axis_bottom}" class="{line_class}"/>'
        )
        if abs(tick - 1.0) < 0.001:
            label = "1.0 parity"
        elif abs(tick - round(tick)) < 0.001:
            label = str(int(round(tick)))
        else:
            label = f"{tick:.1f}"
        tick_labels.append(
            f'  <text x="{x}" y="{axis_top - 8}" text-anchor="middle" '
            f'class="tick">{label}</text>'
        )

    bottom_axis_y = axis_bottom + 16
    bottom_labels = [
        (
            chart_left + round((tick / max_ratio) * chart_width),
            "1.0" if abs(tick - 1.0) < 0.001 else f"{tick:g}",
        )
        for tick in (0.0, 1.0, max_ratio)
    ]
    bottom_axis = [
        f'  <line x1="{chart_left}" y1="{bottom_axis_y}" '
        f'x2="{chart_right}" y2="{bottom_axis_y}" class="axis"/>'
    ]
    for x, label in bottom_labels:
        bottom_axis.append(
            f'  <text x="{x}" y="{bottom_axis_y + 20}" '
            f'text-anchor="middle" class="tick">{svg_text(label)}</text>'
        )

    source = svg_text(source_label)
    return "\n".join(
        [
            (
                '<svg xmlns="http://www.w3.org/2000/svg" '
                f'width="1000" height="{height}" viewBox="0 0 1000 {height}" '
                'role="img" aria-labelledby="title desc">'
            ),
            '  <title id="title">fix32 benchmark fixed/base ratio summary</title>',
            (
                '  <desc id="desc">Horizontal bar chart showing representative '
                'fixed/base ratios from the benchmark output. Lower ratios '
                'favor the fixed-point path; 1.0 is parity.</desc>'
            ),
            "  <style>",
            "    .title { font: 700 26px Arial, Helvetica, sans-serif; fill: #172033; }",
            "    .subtitle { font: 15px Arial, Helvetica, sans-serif; fill: #566273; }",
            "    .axis { stroke: #d8dee8; stroke-width: 1; }",
            "    .parity { stroke: #172033; stroke-width: 2; stroke-dasharray: 5 5; }",
            "    .label { font: 14px Arial, Helvetica, sans-serif; fill: #233044; }",
            "    .group { font: 700 15px Arial, Helvetica, sans-serif; fill: #172033; }",
            "    .tick { font: 12px Arial, Helvetica, sans-serif; fill: #697587; }",
            "    .value { font: 700 13px Arial, Helvetica, sans-serif; fill: #172033; }",
            "    .note { font: 13px Arial, Helvetica, sans-serif; fill: #566273; }",
            "    .faster { fill: #2f8f83; }",
            "    .slower { fill: #c86b38; }",
            "    .neutral { fill: #6b7280; }",
            "  </style>",
            "",
            f'  <rect width="1000" height="{height}" fill="#ffffff"/>',
            "",
            '  <text x="40" y="42" class="title">Benchmark ratio summary</text>',
            (
                '  <text x="40" y="68" class="subtitle">Fixed-point time '
                'divided by float/base time. Lower is better for the '
                'fixed-point path; 1.0 is parity.</text>'
            ),
            "",
            '  <rect x="40" y="84" width="14" height="14" rx="2" class="faster"/>',
            '  <text x="62" y="96" class="note">fixed faster</text>',
            '  <rect x="170" y="84" width="14" height="14" rx="2" class="neutral"/>',
            '  <text x="192" y="96" class="note">near parity</text>',
            '  <rect x="296" y="84" width="14" height="14" rx="2" class="slower"/>',
            '  <text x="318" y="96" class="note">fixed slower</text>',
            "",
            *axis_lines,
            "",
            *tick_labels,
            "",
            *group_blocks,
            "",
            *bottom_axis,
            (
                f'  <text x="40" y="{footer_y}" class="note">Source: '
                f'{source}. Generated by tools/benchmark_graph.py from '
                'fixed/base ratios.</text>'
            ),
            "</svg>",
            "",
        ]
    )


def main() -> int:
    args = parse_args()
    source_label = args.source_label or (
        "standard input" if args.input == "-" else args.input
    )

    try:
        results = parse_results(read_input(args.input))
        groups = build_graph_rows(results)
        svg = render_svg(groups, source_label)
    except OSError as exc:
        print(f"benchmark_graph.py: {exc}", file=sys.stderr)
        return 1
    except ValueError as exc:
        print(f"benchmark_graph.py: {exc}", file=sys.stderr)
        return 1

    output = pathlib.Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(svg, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
