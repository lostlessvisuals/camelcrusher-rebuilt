#!/usr/bin/env python3
from __future__ import annotations

import argparse
import gzip
import json
import sys
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Any

LEGACY_UNIQUE_ID = 1130447730
LEGACY_UNIQUE_ID_FOURCC = "CaCr"
LEGACY_PARAMETER_NAMES = [
    "DistOn",
    "DistMech",
    "DistTube",
    "MmFilterOn",
    "MmFilterCutoff",
    "MmFilterRes",
    "CompressOn",
    "CompressAmount",
    "CompressMode",
    "MasterOn",
    "MasterMix",
    "MasterVolume",
    "Unused1",
    "Unused2",
    "Unused3",
    "Unused4",
    "Unused5",
]


def _value(node: ET.Element | None, default: str = "") -> str:
    if node is None:
        return default
    return node.attrib.get("Value", default)


def _int_value(node: ET.Element | None, default: int = 0) -> int:
    raw = _value(node, str(default))
    try:
        return int(raw)
    except ValueError:
        return default


def _float_value(node: ET.Element | None, default: float = 0.0) -> float:
    raw = _value(node, str(default))
    try:
        return float(raw)
    except ValueError:
        return default


def decode_fourcc(value: int) -> str:
    try:
        return value.to_bytes(4, byteorder="big", signed=False).decode("ascii")
    except (OverflowError, UnicodeDecodeError):
        return ""


def compare_parameter_names(names: list[str]) -> list[dict[str, Any]]:
    mismatches: list[dict[str, Any]] = []

    if len(names) != len(LEGACY_PARAMETER_NAMES):
        mismatches.append(
            {
                "kind": "parameter_count",
                "expected": len(LEGACY_PARAMETER_NAMES),
                "actual": len(names),
            }
        )

    for index, expected in enumerate(LEGACY_PARAMETER_NAMES):
        actual = names[index] if index < len(names) else None
        if actual != expected:
            mismatches.append(
                {
                    "kind": "parameter_name",
                    "index": index,
                    "expected": expected,
                    "actual": actual,
                }
            )

    return mismatches


def _buffer_hex(node: ET.Element | None) -> str:
    if node is None:
        return ""

    return "".join("".join(node.itertext()).split())


def _is_camelcrusher(path_value: str, plug_name: str, unique_id: int) -> bool:
    path_name = Path(path_value).name.lower()
    return (
        unique_id == LEGACY_UNIQUE_ID
        or plug_name == "CamelCrusher"
        or path_name == "camelcrusher.vst"
    )


def extract_camelcrusher_instances(
    source_path: Path, source_label: str | None = None
) -> dict[str, Any]:
    with gzip.open(source_path, "rb") as handle:
        root = ET.parse(handle).getroot()

    instances: list[dict[str, Any]] = []

    for device_index, device in enumerate(root.findall(".//PluginDevice")):
        plugin_info = device.find("./PluginDesc/VstPluginInfo")
        if plugin_info is None:
            continue

        plugin_path = _value(plugin_info.find("Path"))
        plugin_name = _value(plugin_info.find("PlugName"))
        unique_id = _int_value(plugin_info.find("UniqueId"))

        if not _is_camelcrusher(plugin_path, plugin_name, unique_id):
            continue

        preset = plugin_info.find("./Preset/VstPreset")
        parameter_nodes = device.findall("./ParameterList/PluginFloatParameter")
        parameters: list[dict[str, Any]] = []

        for parameter in parameter_nodes:
            parameter_name = _value(parameter.find("ParameterName"))
            if not parameter_name:
                continue

            parameters.append(
                {
                    "index": _int_value(parameter.find("VisualIndex")),
                    "id": _int_value(parameter.find("ParameterId")),
                    "name": parameter_name,
                    "manual_value": _float_value(
                        parameter.find("./ParameterValue/Manual")
                    ),
                }
            )

        parameters.sort(key=lambda item: item["index"])
        parameter_names = [item["name"] for item in parameters]
        schema_mismatches = compare_parameter_names(parameter_names)

        instances.append(
            {
                "device_index": device_index,
                "plugin_path": plugin_path,
                "plugin_name": plugin_name,
                "unique_id": unique_id,
                "unique_id_fourcc": decode_fourcc(unique_id),
                "number_of_parameters": _int_value(
                    plugin_info.find("NumberOfParameters")
                ),
                "raw_parameter_node_count": len(parameter_nodes),
                "named_parameter_count": len(parameters),
                "number_of_programs": _int_value(plugin_info.find("NumberOfPrograms")),
                "program_number": _int_value(
                    preset.find("ProgramNumber") if preset is not None else None, -1
                ),
                "preset_parameter_count": _int_value(
                    preset.find("ParameterCount") if preset is not None else None
                ),
                "preset_program_count": _int_value(
                    preset.find("ProgramCount") if preset is not None else None
                ),
                "plugin_version": _int_value(
                    preset.find("PluginVersion") if preset is not None else None
                ),
                "vst_version": _int_value(plugin_info.find("VstVersion")),
                "byte_order": _int_value(
                    preset.find("ByteOrder") if preset is not None else None
                ),
                "buffer_hex": _buffer_hex(
                    preset.find("Buffer") if preset is not None else None
                ),
                "buffer_size_bytes": len(
                    _buffer_hex(preset.find("Buffer") if preset is not None else None)
                )
                // 2,
                "parameters": parameters,
                "schema_match": not schema_mismatches,
                "schema_mismatches": schema_mismatches,
            }
        )

    return {
        "source_file": source_label or str(source_path),
        "source_kind": "ableton_als",
        "instance_count": len(instances),
        "instances": instances,
    }


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Extract CamelCrusher VST2 state from an Ableton .als fixture into JSON."
        )
    )
    parser.add_argument("source", type=Path, help="Path to the Ableton .als file.")
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        help="Optional path to write the extracted JSON report.",
    )
    parser.add_argument(
        "--source-label",
        help=(
            "Optional public-safe source label to store in the JSON instead of the "
            "local absolute file path."
        ),
    )
    parser.add_argument(
        "--pretty",
        action="store_true",
        help="Pretty-print JSON output with indentation.",
    )
    args = parser.parse_args()

    report = extract_camelcrusher_instances(args.source, args.source_label)
    indent = 2 if args.pretty else None

    if args.output is None:
        json.dump(report, sys.stdout, indent=indent)
        if indent is not None:
            sys.stdout.write("\n")
        return 0

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", encoding="utf-8") as handle:
        json.dump(report, handle, indent=indent)
        if indent is not None:
            handle.write("\n")

    print(f"Wrote {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
