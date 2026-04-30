#!/usr/bin/env python3
from __future__ import annotations

import gzip
import importlib.util
import tempfile
import textwrap
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOL_PATH = ROOT / "tools" / "extract_camelcrusher_fixture.py"

SPEC = importlib.util.spec_from_file_location("fixture_extractor", TOOL_PATH)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"Unable to load fixture extractor from {TOOL_PATH}")

fixture_extractor = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(fixture_extractor)


def _parameter_xml(names: list[str]) -> str:
    blocks = []
    for index, name in enumerate(names):
        blocks.append(
            f"""
            <PluginFloatParameter Id="{index}">
              <ParameterName Value="{name}" />
              <ParameterId Value="{index}" />
              <VisualIndex Value="{index}" />
              <ParameterValue>
                <Manual Value="{index / 16:.8f}" />
              </ParameterValue>
            </PluginFloatParameter>
            """
        )
    return "\n".join(blocks)


def _placeholder_parameter_xml(count: int) -> str:
    blocks = []
    for index in range(count):
        blocks.append(
            f"""
            <PluginFloatParameter Id="{1000 + index}">
              <ParameterName Value="" />
              <ParameterId Value="{1000 + index}" />
              <VisualIndex Value="1073741823" />
              <ParameterValue>
                <Manual Value="0.1234567687" />
              </ParameterValue>
            </PluginFloatParameter>
            """
        )
    return "\n".join(blocks)


def _als_xml(
    plugin_name: str,
    unique_id: int,
    parameter_names: list[str],
    placeholder_count: int = 0,
) -> str:
    return textwrap.dedent(
        f"""\
        <?xml version="1.0" encoding="UTF-8"?>
        <Ableton>
          <LiveSet>
            <Tracks>
              <AudioTrack Id="1">
                <DeviceChain>
                  <Devices>
                    <PluginDevice Id="1">
                      <PluginDesc>
                        <VstPluginInfo Id="0">
                          <Path Value="/Library/Audio/Plug-Ins/VST2/{plugin_name}.vst" />
                          <PlugName Value="{plugin_name}" />
                          <UniqueId Value="{unique_id}" />
                          <NumberOfParameters Value="{len(parameter_names)}" />
                          <NumberOfPrograms Value="20" />
                          <Preset>
                            <VstPreset Id="1">
                              <ProgramNumber Value="3" />
                              <ProgramCount Value="20" />
                              <ParameterCount Value="{len(parameter_names)}" />
                              <Buffer>
                                41424344
                              </Buffer>
                              <PluginVersion Value="1" />
                              <ByteOrder Value="2" />
                            </VstPreset>
                          </Preset>
                          <VstVersion Value="2400" />
                        </VstPluginInfo>
                      </PluginDesc>
                      <ParameterList>
                        {_parameter_xml(parameter_names)}
                        {_placeholder_parameter_xml(placeholder_count)}
                      </ParameterList>
                    </PluginDevice>
                  </Devices>
                </DeviceChain>
              </AudioTrack>
            </Tracks>
          </LiveSet>
        </Ableton>
        """
    )


class FixtureExtractorTests(unittest.TestCase):
    def _write_als(self, xml_text: str) -> Path:
        tempdir = tempfile.TemporaryDirectory()
        self.addCleanup(tempdir.cleanup)
        path = Path(tempdir.name) / "fixture.als"
        with gzip.open(path, "wt", encoding="utf-8") as handle:
            handle.write(xml_text)
        return path

    def test_extracts_camelcrusher_fixture(self) -> None:
        source = self._write_als(
            _als_xml(
                "CamelCrusher",
                fixture_extractor.LEGACY_UNIQUE_ID,
                fixture_extractor.LEGACY_PARAMETER_NAMES,
            )
        )

        report = fixture_extractor.extract_camelcrusher_instances(source)

        self.assertEqual(report["instance_count"], 1)
        instance = report["instances"][0]
        self.assertEqual(instance["plugin_name"], "CamelCrusher")
        self.assertEqual(instance["unique_id_fourcc"], "CaCr")
        self.assertEqual(instance["number_of_parameters"], 17)
        self.assertEqual(instance["program_number"], 3)
        self.assertEqual(instance["buffer_hex"], "41424344")
        self.assertTrue(instance["schema_match"])

    def test_can_override_source_label_for_public_fixtures(self) -> None:
        source = self._write_als(
            _als_xml(
                "CamelCrusher",
                fixture_extractor.LEGACY_UNIQUE_ID,
                fixture_extractor.LEGACY_PARAMETER_NAMES,
            )
        )

        report = fixture_extractor.extract_camelcrusher_instances(
            source, source_label="public/fixture.als"
        )

        self.assertEqual(report["source_file"], "public/fixture.als")

    def test_reports_parameter_order_mismatch(self) -> None:
        reordered_names = fixture_extractor.LEGACY_PARAMETER_NAMES.copy()
        reordered_names[0], reordered_names[1] = (
            reordered_names[1],
            reordered_names[0],
        )
        source = self._write_als(
            _als_xml("CamelCrusher", fixture_extractor.LEGACY_UNIQUE_ID, reordered_names)
        )

        report = fixture_extractor.extract_camelcrusher_instances(source)

        self.assertEqual(report["instance_count"], 1)
        instance = report["instances"][0]
        self.assertFalse(instance["schema_match"])
        self.assertGreaterEqual(len(instance["schema_mismatches"]), 1)

    def test_ignores_blank_placeholder_parameters(self) -> None:
        source = self._write_als(
            _als_xml(
                "CamelCrusher",
                fixture_extractor.LEGACY_UNIQUE_ID,
                fixture_extractor.LEGACY_PARAMETER_NAMES,
                placeholder_count=111,
            )
        )

        report = fixture_extractor.extract_camelcrusher_instances(source)

        self.assertEqual(report["instance_count"], 1)
        instance = report["instances"][0]
        self.assertEqual(instance["raw_parameter_node_count"], 128)
        self.assertEqual(instance["named_parameter_count"], 17)
        self.assertTrue(instance["schema_match"])

    def test_ignores_non_camelcrusher_plugins(self) -> None:
        source = self._write_als(
            _als_xml("SomethingElse", 123456789, fixture_extractor.LEGACY_PARAMETER_NAMES)
        )

        report = fixture_extractor.extract_camelcrusher_instances(source)

        self.assertEqual(report["instance_count"], 0)
        self.assertEqual(report["instances"], [])


if __name__ == "__main__":
    unittest.main()
