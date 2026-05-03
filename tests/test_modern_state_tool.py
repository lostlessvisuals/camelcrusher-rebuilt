#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import plistlib
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOL_PATH = ROOT / "tools" / "modern_state_tool.py"
FIXTURE_PATH = ROOT / "tests" / "fixtures" / "catalyst_patron_camelcrusher.json"


class ModernStateToolTests(unittest.TestCase):
    def _run_tool(self, *args: str) -> dict:
        env = os.environ.copy()
        result = subprocess.run(
            [sys.executable, str(TOOL_PATH), "--pretty", *args],
            check=True,
            capture_output=True,
            text=True,
            cwd=ROOT,
            env=env,
        )
        return json.loads(result.stdout)

    def test_describe_parameters_reports_public_surface(self) -> None:
        description = self._run_tool("describe-parameters")

        self.assertEqual(len(description), 12)
        self.assertEqual(description[0]["stable_id"], "dist_on")
        self.assertEqual(description[0]["legacy_index"], 0)
        self.assertEqual(description[0]["value_kind"], "switch_like")

    def test_can_import_fixture_and_inspect_saved_state(self) -> None:
        with tempfile.TemporaryDirectory() as tempdir:
            output_path = Path(tempdir) / "fixture.ccrstate"

            imported = self._run_tool(
                "import-fixture",
                str(FIXTURE_PATH),
                "--instance-index",
                "0",
                "--output",
                str(output_path),
            )

            self.assertTrue(output_path.exists())
            self.assertEqual(imported["source_file"], "altare/catalyst_patron.als")
            self.assertEqual(imported["selected_program_index"], 3)
            self.assertEqual(imported["selected_program_name"], "British Clean")
            self.assertEqual(imported["preset_count"], 20)
            self.assertEqual(imported["mismatch_count"], 0)
            self.assertEqual(len(imported["public_parameters"]), 12)

            inspected = self._run_tool("inspect-state", str(output_path))

            self.assertEqual(inspected["selected_program_index"], 3)
            self.assertEqual(inspected["selected_program_name"], "British Clean")
            self.assertEqual(inspected["preset_count"], 20)
            self.assertEqual(inspected["mismatch_count"], 0)
            self.assertEqual(len(inspected["public_parameters"]), 12)
            self.assertAlmostEqual(
                inspected["public_parameters"][10]["value"], 0.6150000095, places=6
            )

    def test_can_summarize_fixture_for_vst2_verification(self) -> None:
        summary = self._run_tool(
            "summarize-fixture",
            str(FIXTURE_PATH),
            "--instance-index",
            "0",
        )

        self.assertEqual(summary["source"], "fixture_json")
        self.assertEqual(summary["plugin_name"], "CamelCrusher")
        self.assertEqual(summary["selected_program_index"], 3)
        self.assertEqual(summary["selected_program_name"], "British Clean")
        self.assertEqual(len(summary["legacy_parameters"]), 17)
        self.assertEqual(summary["legacy_parameters"][0]["name"], "DistOn")
        self.assertEqual(summary["legacy_parameters"][0]["display"], "On")

    def test_can_compare_fixture_summary_against_matching_capture(self) -> None:
        with tempfile.TemporaryDirectory() as tempdir:
            summary_path = Path(tempdir) / "captured-state.json"
            fixture_summary = self._run_tool(
                "summarize-fixture",
                str(FIXTURE_PATH),
                "--instance-index",
                "0",
            )
            summary_path.write_text(json.dumps(fixture_summary), encoding="utf-8")

            comparison = self._run_tool(
                "compare-fixture-summary",
                str(FIXTURE_PATH),
                str(summary_path),
                "--instance-index",
                "0",
            )

            self.assertTrue(comparison["exact_match"])
            self.assertEqual(comparison["matching_parameter_count"], 17)
            self.assertEqual(comparison["mismatched_parameters"], [])

    def test_compare_fixture_summary_reports_drift(self) -> None:
        with tempfile.TemporaryDirectory() as tempdir:
            summary_path = Path(tempdir) / "captured-state.json"
            fixture_summary = self._run_tool(
                "summarize-fixture",
                str(FIXTURE_PATH),
                "--instance-index",
                "0",
            )
            fixture_summary["legacy_parameters"][1]["value"] = 0.0
            summary_path.write_text(json.dumps(fixture_summary), encoding="utf-8")

            comparison = self._run_tool(
                "compare-fixture-summary",
                str(FIXTURE_PATH),
                str(summary_path),
                "--instance-index",
                "0",
            )

            self.assertFalse(comparison["exact_match"])
            self.assertEqual(comparison["matching_parameter_count"], 16)
            self.assertEqual(comparison["mismatched_parameters"][0]["legacy_index"], 1)

    def test_can_export_au_preset_from_state_blob(self) -> None:
        with tempfile.TemporaryDirectory() as tempdir:
            state_path = Path(tempdir) / "fixture.ccrstate"
            preset_path = Path(tempdir) / "British Clean Import.aupreset"

            self._run_tool(
                "import-fixture",
                str(FIXTURE_PATH),
                "--instance-index",
                "0",
                "--output",
                str(state_path),
            )
            exported = self._run_tool(
                "export-au-preset",
                str(state_path),
                "--preset-name",
                "British Clean Import",
                "--preset-number",
                "-77",
                "--output",
                str(preset_path),
            )

            self.assertTrue(preset_path.exists())
            self.assertEqual(exported["preset_name"], "British Clean Import")
            self.assertEqual(exported["preset_number"], -77)

            payload = plistlib.loads(preset_path.read_bytes())
            self.assertEqual(payload["type"], int.from_bytes(b"aufx", "big"))
            self.assertEqual(payload["subtype"], int.from_bytes(b"CcrR", "big"))
            self.assertEqual(payload["manufacturer"], int.from_bytes(b"CmAu", "big"))
            self.assertEqual(payload["name"], "British Clean Import")
            self.assertEqual(payload["preset-number"], -77)
            self.assertEqual(payload["data"], state_path.read_bytes())
            self.assertEqual(payload["camelcrusher_runtime_state"], payload["data"])

    def test_can_write_au_preset_directly_from_fixture(self) -> None:
        with tempfile.TemporaryDirectory() as tempdir:
            preset_path = Path(tempdir) / "fixture.aupreset"

            exported = self._run_tool(
                "fixture-to-au-preset",
                str(FIXTURE_PATH),
                "--instance-index",
                "0",
                "--output",
                str(preset_path),
            )

            self.assertTrue(preset_path.exists())
            self.assertEqual(exported["source_file"], "altare/catalyst_patron.als")
            self.assertEqual(exported["preset_name"], "British Clean")
            self.assertEqual(exported["preset_number"], -1)

            payload = plistlib.loads(preset_path.read_bytes())
            self.assertEqual(payload["name"], "British Clean")
            self.assertEqual(payload["preset-number"], -1)
            self.assertGreater(len(payload["data"]), 0)


if __name__ == "__main__":
    unittest.main()
