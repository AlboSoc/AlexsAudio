#!/usr/bin/env python3
"""
Split a server log into ESP-NOW receive -> playback-finished segments and
summarize the timings inside each segment.
"""

from __future__ import annotations

import argparse
import re
import statistics
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable


SEGMENT_START = "ESP-NOW recv from 40:91:51:BE:FC:18"
SEGMENT_END_PREFIX = "Playback finished for sound ID "

RX_RE = re.compile(
    r"LATENCY rx transport=(?P<transport>\S+) cmd=(?P<cmd>\d+) "
    r"sound=(?P<sound>\d+) seq=(?P<seq>\d+) t_us=(?P<t_us>\d+)"
)

ARMED_RE = re.compile(
    r"LATENCY playback-armed transport=(?P<transport>\S+) sound=(?P<sound>\d+) "
    r"seq=(?P<seq>\d+) from_rx_us=(?P<from_rx_us>\d+) open_us=(?P<open_us>\d+) "
    r"decoder_us=(?P<decoder_us>\d+) total_prepare_us=(?P<total_prepare_us>\d+)"
)

FIRST_COPY_RE = re.compile(
    r"LATENCY first-copy(?:-end)? transport=(?P<transport>\S+) sound=(?P<sound>\d+) "
    r"seq=(?P<seq>\d+) from_rx_us=(?P<from_rx_us>\d+)(?: copy_call_us=(?P<copy_call_us>\d+))?"
)

PLAYBACK_FINISHED_RE = re.compile(r"Playback finished for sound ID (?P<sound>\d+)")
TIMESTAMPED_MTB_RE = re.compile(r"^\[(?P<ts_us>\d+) us\] (?P<msg>_?mtb_.*)$")


@dataclass
class MtbEvent:
    ts_us: int
    msg: str


@dataclass
class Segment:
    index: int
    lines: list[str] = field(default_factory=list)
    rx: dict[str, int | str] | None = None
    playback_armed: dict[str, int | str] | None = None
    first_copy: dict[str, int | str] | None = None
    playback_finished_sound: int | None = None
    mtb_lines: list[str] = field(default_factory=list)
    mtb_write_lines: list[str] = field(default_factory=list)
    mtb_events: list[MtbEvent] = field(default_factory=list)

    def parse(self) -> None:
        for line in self.lines:
            if self.rx is None:
                match = RX_RE.search(line)
                if match:
                    self.rx = _normalize_match(match)
                    continue

            if self.playback_armed is None:
                match = ARMED_RE.search(line)
                if match:
                    self.playback_armed = _normalize_match(match)
                    continue

            if self.first_copy is None:
                match = FIRST_COPY_RE.search(line)
                if match:
                    self.first_copy = _normalize_match(match)
                    continue

            ts_match = TIMESTAMPED_MTB_RE.match(line)
            if ts_match:
                msg = ts_match.group("msg")
                self.mtb_lines.append(msg)
                self.mtb_events.append(MtbEvent(ts_us=int(ts_match.group("ts_us")), msg=msg))
                if msg.startswith("mtb_wm8960_write"):
                    self.mtb_write_lines.append(msg)
                continue

            if line.startswith("mtb_") or line.startswith("_mtb_"):
                self.mtb_lines.append(line)
                if line.startswith("mtb_wm8960_write"):
                    self.mtb_write_lines.append(line)

            match = PLAYBACK_FINISHED_RE.search(line)
            if match:
                self.playback_finished_sound = int(match.group("sound"))


def _normalize_match(match: re.Match[str]) -> dict[str, int | str]:
    result: dict[str, int | str] = {}
    for key, value in match.groupdict().items():
        if value is None:
            continue
        result[key] = int(value) if value.isdigit() else value
    return result


def load_segments(lines: Iterable[str]) -> list[Segment]:
    segments: list[Segment] = []
    current: Segment | None = None

    for raw_line in lines:
        line = raw_line.rstrip("\r\n")

        if line == SEGMENT_START:
            if current is not None and current.lines:
                segments.append(current)
            current = Segment(index=len(segments) + 1, lines=[line])
            continue

        if current is None:
            continue

        current.lines.append(line)
        if line.startswith(SEGMENT_END_PREFIX):
            segments.append(current)
            current = None

    if current is not None and current.lines:
        segments.append(current)

    for segment in segments:
        segment.parse()

    return segments


def summarize_metric(name: str, values: list[int]) -> str:
    if not values:
        return f"{name}: n/a"
    if len(values) == 1:
        return f"{name}: {values[0]} us"
    return (
        f"{name}: min={min(values)} us, max={max(values)} us, "
        f"mean={statistics.mean(values):.1f} us"
    )


def render_segment(segment: Segment) -> str:
    parts = [f"Segment {segment.index}"]

    if segment.rx:
        parts.append(
            f"  rx: sound={segment.rx.get('sound')} seq={segment.rx.get('seq')} "
            f"t_us={segment.rx.get('t_us')}"
        )
    if segment.playback_armed:
        parts.append(
            "  playback-armed: "
            f"from_rx_us={segment.playback_armed.get('from_rx_us')} "
            f"open_us={segment.playback_armed.get('open_us')} "
            f"decoder_us={segment.playback_armed.get('decoder_us')} "
            f"total_prepare_us={segment.playback_armed.get('total_prepare_us')}"
        )
    if segment.first_copy:
        copy_call = segment.first_copy.get("copy_call_us")
        copy_call_text = f" copy_call_us={copy_call}" if copy_call is not None else ""
        parts.append(
            f"  first-copy: from_rx_us={segment.first_copy.get('from_rx_us')}{copy_call_text}"
        )

    parts.append(
        f"  mtb_lines={len(segment.mtb_lines)} mtb_wm8960_write={len(segment.mtb_write_lines)}"
    )
    if segment.mtb_events:
        span_us = segment.mtb_events[-1].ts_us - segment.mtb_events[0].ts_us
        parts.append(f"  mtb_span_us={span_us}")
        largest_gaps = top_mtb_gaps(segment, limit=3)
        for gap in largest_gaps:
            parts.append(
                f"  mtb_gap_us={gap['delta_us']} after='{gap['after']}' before='{gap['before']}'"
            )

    if segment.playback_finished_sound is not None:
        parts.append(f"  playback-finished: sound={segment.playback_finished_sound}")

    return "\n".join(parts)


def render_summary(segments: list[Segment]) -> str:
    armed_values = [
        int(segment.playback_armed["from_rx_us"])
        for segment in segments
        if segment.playback_armed and "from_rx_us" in segment.playback_armed
    ]
    open_values = [
        int(segment.playback_armed["open_us"])
        for segment in segments
        if segment.playback_armed and "open_us" in segment.playback_armed
    ]
    decoder_values = [
        int(segment.playback_armed["decoder_us"])
        for segment in segments
        if segment.playback_armed and "decoder_us" in segment.playback_armed
    ]
    prepare_values = [
        int(segment.playback_armed["total_prepare_us"])
        for segment in segments
        if segment.playback_armed and "total_prepare_us" in segment.playback_armed
    ]
    first_copy_values = [
        int(segment.first_copy["from_rx_us"])
        for segment in segments
        if segment.first_copy and "from_rx_us" in segment.first_copy
    ]
    copy_call_values = [
        int(segment.first_copy["copy_call_us"])
        for segment in segments
        if segment.first_copy and "copy_call_us" in segment.first_copy
    ]
    mtb_counts = [len(segment.mtb_lines) for segment in segments]
    mtb_write_counts = [len(segment.mtb_write_lines) for segment in segments]
    mtb_spans = [
        segment.mtb_events[-1].ts_us - segment.mtb_events[0].ts_us
        for segment in segments
        if len(segment.mtb_events) >= 2
    ]
    largest_gap_values = [
        top_mtb_gaps(segment, limit=1)[0]["delta_us"]
        for segment in segments
        if top_mtb_gaps(segment, limit=1)
    ]

    lines = [
        f"Segments found: {len(segments)}",
        summarize_metric("playback-armed.from_rx_us", armed_values),
        summarize_metric("playback-armed.open_us", open_values),
        summarize_metric("playback-armed.decoder_us", decoder_values),
        summarize_metric("playback-armed.total_prepare_us", prepare_values),
        summarize_metric("first-copy.from_rx_us", first_copy_values),
        summarize_metric("first-copy.copy_call_us", copy_call_values),
        f"mtb_lines per segment: {sorted(set(mtb_counts)) if mtb_counts else 'n/a'}",
        f"mtb_wm8960_write per segment: {sorted(set(mtb_write_counts)) if mtb_write_counts else 'n/a'}",
        summarize_metric("mtb_span_us", mtb_spans),
        summarize_metric("largest_mtb_gap_us", largest_gap_values),
    ]
    return "\n".join(lines)


def top_mtb_gaps(segment: Segment, limit: int = 3) -> list[dict[str, int | str]]:
    if len(segment.mtb_events) < 2:
        return []

    gaps: list[dict[str, int | str]] = []
    for prev, cur in zip(segment.mtb_events, segment.mtb_events[1:]):
        gaps.append(
            {
                "delta_us": cur.ts_us - prev.ts_us,
                "after": prev.msg,
                "before": cur.msg,
            }
        )
    gaps.sort(key=lambda item: int(item["delta_us"]), reverse=True)
    return gaps[:limit]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "logfile",
        nargs="?",
        default=Path(__file__).resolve().parents[1] / "data" / "server.log",
        type=Path,
        help="Path to server log file",
    )
    parser.add_argument(
        "--summary-only",
        action="store_true",
        help="Print only the aggregate summary",
    )
    args = parser.parse_args()

    with args.logfile.open("r", encoding="utf-8") as handle:
        segments = load_segments(handle)

    print(render_summary(segments))
    if not args.summary_only:
        print()
        for segment in segments:
            print(render_segment(segment))
            print()

    if not segments:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
