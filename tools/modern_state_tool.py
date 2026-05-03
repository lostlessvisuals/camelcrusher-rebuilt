#!/usr/bin/env python3
from __future__ import annotations

import argparse
import ctypes
import json
import os
import plistlib
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_BRIDGE_NAMES = (
    "libcamelcrusher_runtime_bridge.dylib",
    "libcamelcrusher_runtime_bridge.so",
    "camelcrusher_runtime_bridge.dll",
)
MAX_ERROR_MESSAGE = 128
MAX_PROGRAM_NAME = 64
VALUE_KIND_NAMES = {
    0: "continuous_normalized",
    1: "switch_like",
}
AU_PRESET_COMPONENT_TYPE = int.from_bytes(b"aufx", "big")
AU_PRESET_COMPONENT_SUBTYPE = int.from_bytes(b"CcrR", "big")
AU_PRESET_COMPONENT_MANUFACTURER = int.from_bytes(b"CmAu", "big")
AU_PRESET_VERSION = 3
AU_PRESET_MANUFACTURER_NAME = "Camel Audio"
AU_PRESET_PLUGIN_NAME = "CamelCrusher"
LEGACY_PARAMETER_LAYOUT = (
    ("DistOn", "switch_like"),
    ("DistMech", "continuous_normalized"),
    ("DistTube", "continuous_normalized"),
    ("MmFilterOn", "switch_like"),
    ("MmFilterCutoff", "continuous_normalized"),
    ("MmFilterRes", "continuous_normalized"),
    ("CompressOn", "switch_like"),
    ("CompressAmount", "continuous_normalized"),
    ("CompressMode", "continuous_normalized"),
    ("MasterOn", "switch_like"),
    ("MasterMix", "continuous_normalized"),
    ("MasterVolume", "continuous_normalized"),
    ("Unused1", "continuous_normalized"),
    ("Unused2", "continuous_normalized"),
    ("Unused3", "continuous_normalized"),
    ("Unused4", "continuous_normalized"),
    ("Unused5", "continuous_normalized"),
)


class RuntimeHandle(ctypes.Structure):
    pass


class PublicParameterDescriptor(ctypes.Structure):
    _fields_ = [
        ("public_index", ctypes.c_size_t),
        ("legacy_index", ctypes.c_size_t),
        ("stable_id", ctypes.c_char_p),
        ("display_name", ctypes.c_char_p),
        ("value_kind", ctypes.c_uint32),
    ]


class TextError(ctypes.Structure):
    _fields_ = [
        ("offset", ctypes.c_size_t),
        ("message", ctypes.c_char * MAX_ERROR_MESSAGE),
    ]


class LegacyImportSummary(ctypes.Structure):
    _fields_ = [
        ("selected_program_present", ctypes.c_int),
        ("selected_program_index", ctypes.c_size_t),
        ("selected_program_name", ctypes.c_char * MAX_PROGRAM_NAME),
        ("preset_count", ctypes.c_size_t),
        ("mismatch_count", ctypes.c_size_t),
    ]


class ProcessSpec(ctypes.Structure):
    _fields_ = [
        ("sample_rate", ctypes.c_double),
        ("max_block_size", ctypes.c_size_t),
        ("channel_count", ctypes.c_size_t),
    ]


def _decode_c_string(raw: bytes) -> str:
    return raw.split(b"\0", 1)[0].decode("utf-8")


def _legacy_parameter_display(value_kind: str, value: float) -> str:
    if value_kind == "switch_like":
        return "On" if value >= 0.5 else "Off"
    return f"{round(value * 100.0):d}%"


def _resolve_bridge_path(explicit_path: str | None) -> Path:
    candidates: list[Path] = []
    if explicit_path:
        candidates.append(Path(explicit_path))

    env_path = os.environ.get("CAMELCRUSHER_BRIDGE_LIB")
    if env_path:
        candidates.append(Path(env_path))

    for name in DEFAULT_BRIDGE_NAMES:
        candidates.append(ROOT / "build" / name)

    for candidate in candidates:
        if candidate.exists():
            return candidate.resolve()

    searched = "\n".join(str(path) for path in candidates)
    raise FileNotFoundError(
        "Unable to locate camelcrusher runtime bridge library. "
        f"Searched:\n{searched}"
    )


class BridgeLibrary:
    def __init__(self, path: Path) -> None:
        self.path = path
        self.lib = ctypes.CDLL(str(path))

        runtime_ptr = ctypes.POINTER(RuntimeHandle)

        self.lib.camelcrusher_runtime_bridge_public_parameter_count.restype = (
            ctypes.c_size_t
        )

        self.lib.camelcrusher_runtime_bridge_get_public_parameter_descriptor.argtypes = [
            ctypes.c_size_t,
            ctypes.POINTER(PublicParameterDescriptor),
        ]
        self.lib.camelcrusher_runtime_bridge_get_public_parameter_descriptor.restype = (
            ctypes.c_int
        )

        self.lib.camelcrusher_runtime_bridge_runtime_create.argtypes = []
        self.lib.camelcrusher_runtime_bridge_runtime_create.restype = runtime_ptr

        self.lib.camelcrusher_runtime_bridge_runtime_destroy.argtypes = [runtime_ptr]
        self.lib.camelcrusher_runtime_bridge_runtime_destroy.restype = None

        self.lib.camelcrusher_runtime_bridge_runtime_prepare.argtypes = [
            runtime_ptr,
            ProcessSpec,
        ]
        self.lib.camelcrusher_runtime_bridge_runtime_prepare.restype = None

        self.lib.camelcrusher_runtime_bridge_runtime_get_public_parameter.argtypes = [
            runtime_ptr,
            ctypes.c_size_t,
        ]
        self.lib.camelcrusher_runtime_bridge_runtime_get_public_parameter.restype = (
            ctypes.c_float
        )

        self.lib.camelcrusher_runtime_bridge_runtime_get_legacy_parameter.argtypes = [
            runtime_ptr,
            ctypes.c_size_t,
        ]
        self.lib.camelcrusher_runtime_bridge_runtime_get_legacy_parameter.restype = (
            ctypes.c_float
        )

        self.lib.camelcrusher_runtime_bridge_runtime_preset_count.argtypes = [runtime_ptr]
        self.lib.camelcrusher_runtime_bridge_runtime_preset_count.restype = (
            ctypes.c_size_t
        )

        self.lib.camelcrusher_runtime_bridge_runtime_mismatch_count.argtypes = [
            runtime_ptr
        ]
        self.lib.camelcrusher_runtime_bridge_runtime_mismatch_count.restype = (
            ctypes.c_size_t
        )

        self.lib.camelcrusher_runtime_bridge_runtime_get_selected_program_index.argtypes = [
            runtime_ptr,
            ctypes.POINTER(ctypes.c_size_t),
        ]
        self.lib.camelcrusher_runtime_bridge_runtime_get_selected_program_index.restype = (
            ctypes.c_int
        )

        self.lib.camelcrusher_runtime_bridge_runtime_get_selected_program_name.argtypes = [
            runtime_ptr,
            ctypes.POINTER(ctypes.c_char),
            ctypes.c_size_t,
        ]
        self.lib.camelcrusher_runtime_bridge_runtime_get_selected_program_name.restype = (
            ctypes.c_int
        )

        self.lib.camelcrusher_runtime_bridge_runtime_import_legacy_chunk.argtypes = [
            runtime_ptr,
            ctypes.POINTER(ctypes.c_uint8),
            ctypes.c_size_t,
            ctypes.c_int32,
            ctypes.POINTER(ctypes.c_float),
            ctypes.c_size_t,
            ctypes.POINTER(LegacyImportSummary),
            ctypes.POINTER(TextError),
        ]
        self.lib.camelcrusher_runtime_bridge_runtime_import_legacy_chunk.restype = (
            ctypes.c_int
        )

        self.lib.camelcrusher_runtime_bridge_runtime_state_size.argtypes = [runtime_ptr]
        self.lib.camelcrusher_runtime_bridge_runtime_state_size.restype = ctypes.c_size_t

        self.lib.camelcrusher_runtime_bridge_runtime_save_state.argtypes = [
            runtime_ptr,
            ctypes.POINTER(ctypes.c_uint8),
            ctypes.c_size_t,
            ctypes.POINTER(ctypes.c_size_t),
        ]
        self.lib.camelcrusher_runtime_bridge_runtime_save_state.restype = ctypes.c_int

        self.lib.camelcrusher_runtime_bridge_runtime_load_state.argtypes = [
            runtime_ptr,
            ctypes.POINTER(ctypes.c_uint8),
            ctypes.c_size_t,
            ctypes.POINTER(TextError),
        ]
        self.lib.camelcrusher_runtime_bridge_runtime_load_state.restype = ctypes.c_int

    def create_runtime(self) -> ctypes.POINTER(RuntimeHandle):
        runtime = self.lib.camelcrusher_runtime_bridge_runtime_create()
        if not runtime:
            raise RuntimeError("Unable to create runtime bridge instance")

        self.lib.camelcrusher_runtime_bridge_runtime_prepare(
            runtime,
            ProcessSpec(sample_rate=48000.0, max_block_size=512, channel_count=2),
        )
        return runtime

    def destroy_runtime(self, runtime: ctypes.POINTER(RuntimeHandle)) -> None:
        self.lib.camelcrusher_runtime_bridge_runtime_destroy(runtime)

    def public_parameter_descriptors(self) -> list[dict[str, Any]]:
        count = self.lib.camelcrusher_runtime_bridge_public_parameter_count()
        descriptors: list[dict[str, Any]] = []
        for index in range(count):
            raw = PublicParameterDescriptor()
            ok = self.lib.camelcrusher_runtime_bridge_get_public_parameter_descriptor(
                index, ctypes.byref(raw)
            )
            if not ok:
                raise RuntimeError(f"Unable to read descriptor for index {index}")
            descriptors.append(
                {
                    "public_index": raw.public_index,
                    "legacy_index": raw.legacy_index,
                    "stable_id": raw.stable_id.decode("utf-8"),
                    "display_name": raw.display_name.decode("utf-8"),
                    "value_kind": VALUE_KIND_NAMES.get(raw.value_kind, "unknown"),
                }
            )
        return descriptors

    def summarize_runtime(
        self, runtime: ctypes.POINTER(RuntimeHandle)
    ) -> dict[str, Any]:
        descriptors = self.public_parameter_descriptors()

        selected_program_index = ctypes.c_size_t()
        has_program = self.lib.camelcrusher_runtime_bridge_runtime_get_selected_program_index(
            runtime, ctypes.byref(selected_program_index)
        )

        selected_name_buffer = ctypes.create_string_buffer(MAX_PROGRAM_NAME)
        self.lib.camelcrusher_runtime_bridge_runtime_get_selected_program_name(
            runtime, selected_name_buffer, len(selected_name_buffer)
        )

        public_parameters = []
        for descriptor in descriptors:
            public_parameters.append(
                {
                    **descriptor,
                    "value": float(
                        self.lib.camelcrusher_runtime_bridge_runtime_get_public_parameter(
                            runtime, descriptor["public_index"]
                        )
                    ),
                }
            )

        legacy_parameters = []
        for legacy_index in range(17):
            legacy_parameters.append(
                {
                    "legacy_index": legacy_index,
                    "value": float(
                        self.lib.camelcrusher_runtime_bridge_runtime_get_legacy_parameter(
                            runtime, legacy_index
                        )
                    ),
                }
            )

        return {
            "selected_program_index": (
                int(selected_program_index.value) if has_program else None
            ),
            "selected_program_name": selected_name_buffer.value.decode("utf-8"),
            "preset_count": int(
                self.lib.camelcrusher_runtime_bridge_runtime_preset_count(runtime)
            ),
            "mismatch_count": int(
                self.lib.camelcrusher_runtime_bridge_runtime_mismatch_count(runtime)
            ),
            "public_parameters": public_parameters,
            "legacy_parameters": legacy_parameters,
        }

    def import_fixture_instance(
        self, runtime: ctypes.POINTER(RuntimeHandle), instance: dict[str, Any]
    ) -> dict[str, Any]:
        buffer_hex = instance.get("buffer_hex", "")
        if not isinstance(buffer_hex, str) or not buffer_hex:
            raise ValueError("Fixture instance is missing a valid buffer_hex string")

        parameters = instance.get("parameters", [])
        if not isinstance(parameters, list) or not parameters:
            raise ValueError("Fixture instance is missing parameter data")

        ordered_parameters = sorted(parameters, key=lambda item: item["index"])
        explicit_values = [float(item["manual_value"]) for item in ordered_parameters]

        chunk_bytes_value = bytes.fromhex(buffer_hex)
        chunk_bytes = (ctypes.c_uint8 * len(chunk_bytes_value)).from_buffer_copy(
            chunk_bytes_value
        )
        explicit_array = (ctypes.c_float * len(explicit_values))(*explicit_values)
        summary = LegacyImportSummary()
        error = TextError()
        program_number = int(instance.get("program_number", -1))

        ok = self.lib.camelcrusher_runtime_bridge_runtime_import_legacy_chunk(
            runtime,
            chunk_bytes,
            len(chunk_bytes_value),
            program_number,
            explicit_array,
            len(explicit_values),
            ctypes.byref(summary),
            ctypes.byref(error),
        )
        if not ok:
            raise RuntimeError(
                f"Legacy import failed at offset {error.offset}: "
                f"{_decode_c_string(error.message)}"
            )

        runtime_summary = self.summarize_runtime(runtime)
        runtime_summary.update(
            {
                "fixture_device_index": instance.get("device_index"),
                "fixture_plugin_name": instance.get("plugin_name"),
                "fixture_unique_id_fourcc": instance.get("unique_id_fourcc"),
                "import_selected_program_present": bool(
                    summary.selected_program_present
                ),
                "import_selected_program_index": int(summary.selected_program_index),
                "import_selected_program_name": _decode_c_string(
                    summary.selected_program_name
                ),
            }
        )
        return runtime_summary

    def save_runtime_state(
        self, runtime: ctypes.POINTER(RuntimeHandle), output_path: Path
    ) -> int:
        payload = self.serialized_runtime_state(runtime)
        output_path.write_bytes(payload)
        return len(payload)

    def serialized_runtime_state(
        self, runtime: ctypes.POINTER(RuntimeHandle)
    ) -> bytes:
        state_size = int(
            self.lib.camelcrusher_runtime_bridge_runtime_state_size(runtime)
        )
        if state_size <= 0:
            raise RuntimeError("Runtime returned an empty state blob")

        state_bytes = (ctypes.c_uint8 * state_size)()
        bytes_written = ctypes.c_size_t()
        ok = self.lib.camelcrusher_runtime_bridge_runtime_save_state(
            runtime,
            state_bytes,
            state_size,
            ctypes.byref(bytes_written),
        )
        if not ok:
            raise RuntimeError("Runtime failed to serialize state")

        return bytes(state_bytes[: bytes_written.value])

    def load_runtime_state(
        self, runtime: ctypes.POINTER(RuntimeHandle), bytes_value: bytes
    ) -> None:
        payload = (ctypes.c_uint8 * len(bytes_value)).from_buffer_copy(bytes_value)
        error = TextError()
        ok = self.lib.camelcrusher_runtime_bridge_runtime_load_state(
            runtime,
            payload,
            len(bytes_value),
            ctypes.byref(error),
        )
        if not ok:
            raise RuntimeError(
                f"State load failed at offset {error.offset}: "
                f"{_decode_c_string(error.message)}"
            )


def _load_fixture_instance(path: Path, instance_index: int) -> tuple[dict[str, Any], dict[str, Any]]:
    report = json.loads(path.read_text(encoding="utf-8"))
    instances = report.get("instances", [])
    if not isinstance(instances, list):
        raise ValueError("Fixture report does not contain an instances list")
    if instance_index < 0 or instance_index >= len(instances):
        raise IndexError(
            f"Fixture instance index {instance_index} is out of range for {len(instances)} instances"
        )
    return report, instances[instance_index]


def _sanitize_preset_name(name: str) -> str:
    collapsed = " ".join(name.split()).strip()
    return collapsed or "Imported Preset"


def _default_au_preset_output(preset_name: str) -> Path:
    sanitized_name = _sanitize_preset_name(preset_name)
    configured_dir = os.environ.get("CAMELCRUSHER_AU_USER_PRESET_DIR")
    if configured_dir:
        return Path(configured_dir) / f"{sanitized_name}.aupreset"
    standard_dir = (
        Path.home()
        / "Library"
        / "Audio"
        / "Presets"
        / AU_PRESET_MANUFACTURER_NAME
        / AU_PRESET_PLUGIN_NAME
    )
    try:
        standard_dir.mkdir(parents=True, exist_ok=True)
        return standard_dir / f"{sanitized_name}.aupreset"
    except PermissionError:
        fallback_dir = ROOT / "build" / "au-presets"
        fallback_dir.mkdir(parents=True, exist_ok=True)
        return fallback_dir / f"{sanitized_name}.aupreset"


def _au_preset_payload(
    state_bytes: bytes, preset_name: str, preset_number: int
) -> dict[str, Any]:
    normalized_name = _sanitize_preset_name(preset_name)
    return {
        "type": AU_PRESET_COMPONENT_TYPE,
        "subtype": AU_PRESET_COMPONENT_SUBTYPE,
        "manufacturer": AU_PRESET_COMPONENT_MANUFACTURER,
        "version": AU_PRESET_VERSION,
        "name": normalized_name,
        "preset-number": preset_number,
        "data": state_bytes,
        "camelcrusher_runtime_state": state_bytes,
    }


def _emit_json(data: dict[str, Any] | list[dict[str, Any]], pretty: bool) -> None:
    json.dump(data, sys.stdout, indent=2 if pretty else None)
    sys.stdout.write("\n")


def _build_legacy_state_summary(
    runtime_summary: dict[str, Any],
    *,
    source: str,
    plugin_name: str,
    source_file: str | None = None,
    instance_index: int | None = None,
) -> dict[str, Any]:
    legacy_parameters: list[dict[str, Any]] = []
    for raw_parameter in runtime_summary.get("legacy_parameters", []):
        legacy_index = int(raw_parameter["legacy_index"])
        name, value_kind = LEGACY_PARAMETER_LAYOUT[legacy_index]
        value = float(raw_parameter["value"])
        legacy_parameters.append(
            {
                "legacy_index": legacy_index,
                "name": name,
                "value": value,
                "display": _legacy_parameter_display(value_kind, value),
                "value_kind": value_kind,
            }
        )

    summary: dict[str, Any] = {
        "source": source,
        "plugin_name": plugin_name,
        "selected_program_index": runtime_summary.get("selected_program_index"),
        "selected_program_name": runtime_summary.get("selected_program_name"),
        "legacy_parameters": legacy_parameters,
    }
    if source_file is not None:
        summary["source_file"] = source_file
    if instance_index is not None:
        summary["instance_index"] = instance_index
    return summary


def _summary_parameters_by_index(summary: dict[str, Any]) -> dict[int, dict[str, Any]]:
    legacy_parameters = summary.get("legacy_parameters")
    if not isinstance(legacy_parameters, list):
        raise ValueError("State summary is missing a legacy_parameters list")

    by_index: dict[int, dict[str, Any]] = {}
    for raw_parameter in legacy_parameters:
        if not isinstance(raw_parameter, dict):
            raise ValueError("legacy_parameters entries must be objects")
        legacy_index = int(raw_parameter["legacy_index"])
        by_index[legacy_index] = raw_parameter
    return by_index


def command_describe_parameters(args: argparse.Namespace) -> int:
    bridge = BridgeLibrary(_resolve_bridge_path(args.bridge_lib))
    _emit_json(bridge.public_parameter_descriptors(), args.pretty)
    return 0


def command_import_fixture(args: argparse.Namespace) -> int:
    fixture_path = Path(args.fixture_json)
    output_path = Path(args.output)
    report, instance = _load_fixture_instance(fixture_path, args.instance_index)
    bridge = BridgeLibrary(_resolve_bridge_path(args.bridge_lib))
    runtime = bridge.create_runtime()
    try:
        summary = bridge.import_fixture_instance(runtime, instance)
        summary["source_file"] = report.get("source_file")
        summary["instance_index"] = args.instance_index
        summary["output_state_file"] = str(output_path)
        summary["output_state_size"] = bridge.save_runtime_state(runtime, output_path)
        _emit_json(summary, args.pretty)
        return 0
    finally:
        bridge.destroy_runtime(runtime)


def command_inspect_state(args: argparse.Namespace) -> int:
    state_path = Path(args.state_file)
    bridge = BridgeLibrary(_resolve_bridge_path(args.bridge_lib))
    runtime = bridge.create_runtime()
    try:
        bridge.load_runtime_state(runtime, state_path.read_bytes())
        summary = bridge.summarize_runtime(runtime)
        summary["state_file"] = str(state_path)
        summary["bridge_library"] = str(bridge.path)
        _emit_json(summary, args.pretty)
        return 0
    finally:
        bridge.destroy_runtime(runtime)


def command_summarize_fixture(args: argparse.Namespace) -> int:
    fixture_path = Path(args.fixture_json)
    report, instance = _load_fixture_instance(fixture_path, args.instance_index)
    bridge = BridgeLibrary(_resolve_bridge_path(args.bridge_lib))
    runtime = bridge.create_runtime()
    try:
        runtime_summary = bridge.import_fixture_instance(runtime, instance)
        summary = _build_legacy_state_summary(
            runtime_summary,
            source="fixture_json",
            plugin_name=str(instance.get("plugin_name") or AU_PRESET_PLUGIN_NAME),
            source_file=report.get("source_file"),
            instance_index=args.instance_index,
        )
        _emit_json(summary, args.pretty)
        return 0
    finally:
        bridge.destroy_runtime(runtime)


def command_compare_fixture_summary(args: argparse.Namespace) -> int:
    fixture_path = Path(args.fixture_json)
    captured_path = Path(args.summary_json)
    report, instance = _load_fixture_instance(fixture_path, args.instance_index)
    bridge = BridgeLibrary(_resolve_bridge_path(args.bridge_lib))
    runtime = bridge.create_runtime()
    try:
        runtime_summary = bridge.import_fixture_instance(runtime, instance)
        expected_summary = _build_legacy_state_summary(
            runtime_summary,
            source="fixture_json",
            plugin_name=str(instance.get("plugin_name") or AU_PRESET_PLUGIN_NAME),
            source_file=report.get("source_file"),
            instance_index=args.instance_index,
        )
    finally:
        bridge.destroy_runtime(runtime)

    actual_summary = json.loads(captured_path.read_text(encoding="utf-8"))
    expected_parameters = _summary_parameters_by_index(expected_summary)
    actual_parameters = _summary_parameters_by_index(actual_summary)

    mismatched_parameters: list[dict[str, Any]] = []
    max_abs_delta = 0.0
    matching_parameter_count = 0

    for legacy_index, expected_parameter in sorted(expected_parameters.items()):
        actual_parameter = actual_parameters.get(legacy_index)
        if actual_parameter is None:
            mismatched_parameters.append(
                {
                    "legacy_index": legacy_index,
                    "name": expected_parameter["name"],
                    "reason": "missing_from_actual_summary",
                }
            )
            continue

        expected_value = float(expected_parameter["value"])
        actual_value = float(actual_parameter["value"])
        delta = actual_value - expected_value
        max_abs_delta = max(max_abs_delta, abs(delta))
        if abs(delta) > args.tolerance:
            mismatched_parameters.append(
                {
                    "legacy_index": legacy_index,
                    "name": expected_parameter["name"],
                    "expected": expected_value,
                    "actual": actual_value,
                    "delta": delta,
                }
            )
        else:
            matching_parameter_count += 1

    expected_program_index = expected_summary.get("selected_program_index")
    actual_program_index = actual_summary.get("selected_program_index")
    expected_program_name = expected_summary.get("selected_program_name")
    actual_program_name = actual_summary.get("selected_program_name")

    program_index_matches = expected_program_index == actual_program_index
    program_name_matches = expected_program_name == actual_program_name
    plugin_name_matches = expected_summary.get("plugin_name") == actual_summary.get(
        "plugin_name"
    )

    comparison = {
        "fixture_source_file": report.get("source_file"),
        "fixture_instance_index": args.instance_index,
        "captured_summary_file": str(captured_path),
        "plugin_name_matches": plugin_name_matches,
        "program_index_matches": program_index_matches,
        "program_name_matches": program_name_matches,
        "matching_parameter_count": matching_parameter_count,
        "parameter_count": len(expected_parameters),
        "max_abs_delta": max_abs_delta,
        "tolerance": args.tolerance,
        "mismatched_parameters": mismatched_parameters,
        "expected_program_index": expected_program_index,
        "actual_program_index": actual_program_index,
        "expected_program_name": expected_program_name,
        "actual_program_name": actual_program_name,
    }
    comparison["exact_match"] = (
        plugin_name_matches
        and program_index_matches
        and program_name_matches
        and not mismatched_parameters
    )
    _emit_json(comparison, args.pretty)
    return 0


def command_export_au_preset(args: argparse.Namespace) -> int:
    state_path = Path(args.state_file)
    output_path = (
        Path(args.output)
        if args.output is not None
        else _default_au_preset_output(args.preset_name)
    )
    output_path.parent.mkdir(parents=True, exist_ok=True)
    state_bytes = state_path.read_bytes()
    payload = _au_preset_payload(state_bytes, args.preset_name, args.preset_number)
    with output_path.open("wb") as handle:
        plistlib.dump(payload, handle, fmt=plistlib.FMT_BINARY, sort_keys=False)

    summary = {
        "state_file": str(state_path),
        "output_au_preset": str(output_path),
        "preset_name": payload["name"],
        "preset_number": payload["preset-number"],
        "component_type": payload["type"],
        "component_subtype": payload["subtype"],
        "component_manufacturer": payload["manufacturer"],
        "state_size": len(state_bytes),
    }
    _emit_json(summary, args.pretty)
    return 0


def command_fixture_to_au_preset(args: argparse.Namespace) -> int:
    fixture_path = Path(args.fixture_json)
    report, instance = _load_fixture_instance(fixture_path, args.instance_index)
    bridge = BridgeLibrary(_resolve_bridge_path(args.bridge_lib))
    runtime = bridge.create_runtime()
    try:
        summary = bridge.import_fixture_instance(runtime, instance)
        state_bytes = bridge.serialized_runtime_state(runtime)
        default_name = summary.get("selected_program_name") or "Imported Preset"
        preset_name = (
            args.preset_name if args.preset_name is not None else str(default_name)
        )
        output_path = (
            Path(args.output)
            if args.output is not None
            else _default_au_preset_output(preset_name)
        )
        output_path.parent.mkdir(parents=True, exist_ok=True)
        payload = _au_preset_payload(state_bytes, preset_name, args.preset_number)
        with output_path.open("wb") as handle:
            plistlib.dump(payload, handle, fmt=plistlib.FMT_BINARY, sort_keys=False)

        summary.update(
            {
                "source_file": report.get("source_file"),
                "instance_index": args.instance_index,
                "output_au_preset": str(output_path),
                "preset_name": payload["name"],
                "preset_number": payload["preset-number"],
                "state_size": len(state_bytes),
            }
        )
        _emit_json(summary, args.pretty)
        return 0
    finally:
        bridge.destroy_runtime(runtime)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Inspect and convert CamelCrusher modern runtime state."
    )
    parser.add_argument(
        "--bridge-lib",
        help="Path to libcamelcrusher_runtime_bridge shared library",
    )
    parser.add_argument(
        "--pretty",
        action="store_true",
        help="Pretty-print JSON output",
    )

    subparsers = parser.add_subparsers(dest="command", required=True)

    describe_parser = subparsers.add_parser(
        "describe-parameters", help="Print the modern public parameter layout"
    )
    describe_parser.set_defaults(func=command_describe_parameters)

    import_parser = subparsers.add_parser(
        "import-fixture",
        help="Convert extracted fixture JSON into a serialized modern runtime state blob",
    )
    import_parser.add_argument("fixture_json", help="Path to extracted fixture JSON")
    import_parser.add_argument(
        "--instance-index",
        type=int,
        default=0,
        help="Fixture instance index to import",
    )
    import_parser.add_argument(
        "-o",
        "--output",
        required=True,
        help="Path to write the serialized modern state blob",
    )
    import_parser.set_defaults(func=command_import_fixture)

    inspect_parser = subparsers.add_parser(
        "inspect-state", help="Read and summarize a serialized modern runtime state blob"
    )
    inspect_parser.add_argument("state_file", help="Path to serialized modern state")
    inspect_parser.set_defaults(func=command_inspect_state)

    summarize_fixture_parser = subparsers.add_parser(
        "summarize-fixture",
        help="Summarize a fixture instance into the same 17-parameter shape the VST2 editor copies",
    )
    summarize_fixture_parser.add_argument(
        "fixture_json", help="Path to extracted fixture JSON"
    )
    summarize_fixture_parser.add_argument(
        "--instance-index",
        type=int,
        default=0,
        help="Fixture instance index to summarize",
    )
    summarize_fixture_parser.set_defaults(func=command_summarize_fixture)

    compare_fixture_summary_parser = subparsers.add_parser(
        "compare-fixture-summary",
        help="Compare a copied VST2 editor state summary JSON file against a fixture instance",
    )
    compare_fixture_summary_parser.add_argument(
        "fixture_json", help="Path to extracted fixture JSON"
    )
    compare_fixture_summary_parser.add_argument(
        "summary_json",
        help="Path to a captured state summary JSON file copied from the VST2 editor",
    )
    compare_fixture_summary_parser.add_argument(
        "--instance-index",
        type=int,
        default=0,
        help="Fixture instance index to compare",
    )
    compare_fixture_summary_parser.add_argument(
        "--tolerance",
        type=float,
        default=1e-4,
        help="Absolute tolerance for parameter-value comparisons",
    )
    compare_fixture_summary_parser.set_defaults(func=command_compare_fixture_summary)

    export_au_preset_parser = subparsers.add_parser(
        "export-au-preset",
        help="Wrap a serialized modern runtime state blob in an AU preset plist",
    )
    export_au_preset_parser.add_argument(
        "state_file", help="Path to serialized modern state"
    )
    export_au_preset_parser.add_argument(
        "--preset-name",
        required=True,
        help="User-visible AU preset name",
    )
    export_au_preset_parser.add_argument(
        "--preset-number",
        type=int,
        default=-1,
        help="Preset number to store in the AU preset payload; negative means user preset",
    )
    export_au_preset_parser.add_argument(
        "-o",
        "--output",
        help="Path to write the .aupreset file; defaults to the standard user preset folder",
    )
    export_au_preset_parser.set_defaults(func=command_export_au_preset)

    fixture_to_au_preset_parser = subparsers.add_parser(
        "fixture-to-au-preset",
        help="Import fixture JSON and write a host-loadable AU preset plist",
    )
    fixture_to_au_preset_parser.add_argument(
        "fixture_json", help="Path to extracted fixture JSON"
    )
    fixture_to_au_preset_parser.add_argument(
        "--instance-index",
        type=int,
        default=0,
        help="Fixture instance index to import",
    )
    fixture_to_au_preset_parser.add_argument(
        "--preset-name",
        help="User-visible AU preset name; defaults to the imported selected program name",
    )
    fixture_to_au_preset_parser.add_argument(
        "--preset-number",
        type=int,
        default=-1,
        help="Preset number to store in the AU preset payload; negative means user preset",
    )
    fixture_to_au_preset_parser.add_argument(
        "-o",
        "--output",
        help="Path to write the .aupreset file; defaults to the standard user preset folder",
    )
    fixture_to_au_preset_parser.set_defaults(func=command_fixture_to_au_preset)

    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        return args.func(args)
    except Exception as exc:
        parser.exit(1, f"{exc}\n")


if __name__ == "__main__":
    raise SystemExit(main())
