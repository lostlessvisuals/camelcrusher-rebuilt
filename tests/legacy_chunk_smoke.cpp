#include "camelcrusher_recalled/LegacyChunk.h"
#include "camelcrusher_recalled/LegacyImport.h"
#include "camelcrusher_recalled/LegacyPresetBank.h"
#include "camelcrusher_recalled/LegacySchema.h"
#include "camelcrusher_recalled/LegacyState.h"

#include <algorithm>
#include <array>
#include "TestSupport.h"
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace {

using camelcrusher_recalled::LegacyChunk;
using camelcrusher_recalled::LegacyProgram;
using camelcrusher_recalled::decodeLegacyChunk;
using camelcrusher_recalled::decodeLegacyChunkHex;
using camelcrusher_recalled::encodeLegacyChunk;
using camelcrusher_recalled::importLegacyState;
using camelcrusher_recalled::importedStateToArray;
using camelcrusher_recalled::isLikelyLegacyChunk;
using camelcrusher_recalled::LegacyCurrentStateSource;
using camelcrusher_recalled::legacyPresetAt;
using camelcrusher_recalled::legacyStateToArray;
using camelcrusher_recalled::kLegacyParameterCount;

LegacyProgram makeProgram(const std::string& name, float seed) {
  LegacyProgram program;
  program.record_marker = 1.0F;
  program.name = name;
  for (std::size_t index = 0; index < program.parameter_values.size(); ++index) {
    program.parameter_values[index] = seed + static_cast<float>(index) * 0.03125F;
  }
  return program;
}

std::string readFile(const std::filesystem::path& path) {
  std::ifstream stream(path);
  CC_CHECK(stream.good());
  return std::string(std::istreambuf_iterator<char>(stream),
                     std::istreambuf_iterator<char>());
}

std::string extractFirstQuotedValue(const std::string& content,
                                    const std::string& key) {
  const auto marker = "\"" + key + "\": \"";
  const auto start = content.find(marker);
  CC_CHECK(start != std::string::npos);
  const auto value_begin = start + marker.size();
  const auto value_end = content.find('"', value_begin);
  CC_CHECK(value_end != std::string::npos);
  return content.substr(value_begin, value_end - value_begin);
}

int extractFirstIntegerValue(const std::string& content, const std::string& key) {
  const auto marker = "\"" + key + "\": ";
  const auto start = content.find(marker);
  CC_CHECK(start != std::string::npos);
  const auto value_begin = start + marker.size();
  const auto value_end = content.find_first_not_of("-0123456789", value_begin);
  CC_CHECK(value_end != std::string::npos);
  return std::stoi(content.substr(value_begin, value_end - value_begin));
}

std::vector<float> extractManualValues(const std::string& content,
                                       std::size_t expected_count) {
  std::regex pattern(R"("manual_value"\s*:\s*([-+]?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?))");
  std::vector<float> values;

  for (std::sregex_iterator it(content.begin(), content.end(), pattern), end;
       it != end && values.size() < expected_count;
       ++it) {
    values.push_back(std::stof((*it)[1].str()));
  }

  CC_CHECK(values.size() == expected_count);
  return values;
}

bool nearlyEqual(float left, float right) {
  return std::fabs(left - right) < 0.000001F;
}

void assertRealFixtureRoundTrip(const std::filesystem::path& fixture_path) {
  const auto content = readFile(fixture_path);
  const auto buffer_hex = extractFirstQuotedValue(content, "buffer_hex");
  const auto program_number = extractFirstIntegerValue(content, "program_number");
  const auto parameter_values =
      extractManualValues(content, kLegacyParameterCount);

  const auto decode_result = decodeLegacyChunkHex(buffer_hex);
  CC_CHECK(decode_result.ok());
  CC_CHECK(decode_result.chunk.has_value());

  const auto& chunk = decode_result.chunk.value();
  CC_CHECK(isLikelyLegacyChunk(chunk));
  CC_CHECK(chunk.programs.size() == 20U);
  CC_CHECK(program_number >= 0);
  CC_CHECK(static_cast<std::size_t>(program_number) < chunk.programs.size());

  const auto& selected_program =
      chunk.programs[static_cast<std::size_t>(program_number)];
  for (std::size_t index = 0; index < parameter_values.size(); ++index) {
    CC_CHECK(nearlyEqual(selected_program.parameter_values[index],
                       parameter_values[index]));
  }

  const auto imported_state =
      importLegacyState(chunk, program_number, parameter_values);
  CC_CHECK(imported_state.current_state_source ==
         LegacyCurrentStateSource::kExplicitParameters);
  CC_CHECK(imported_state.selected_program_index.has_value());
  CC_CHECK(imported_state.selected_program_name == selected_program.name);
  CC_CHECK(imported_state.selected_program_matches_explicit_parameters);
  CC_CHECK(imported_state.parameter_mismatches.empty());
  CC_CHECK(imported_state.preset_bank.presets.size() == 20U);
  const auto* selected_preset = legacyPresetAt(
      imported_state.preset_bank, static_cast<std::size_t>(program_number));
  CC_CHECK(selected_preset != nullptr);
  CC_CHECK(selected_preset->name == selected_program.name);

  const auto imported_values = importedStateToArray(imported_state);
  const auto selected_preset_values = legacyStateToArray(selected_preset->state);
  for (std::size_t index = 0; index < parameter_values.size(); ++index) {
    CC_CHECK(nearlyEqual(imported_values[index], parameter_values[index]));
    CC_CHECK(nearlyEqual(selected_preset_values[index], parameter_values[index]));
  }

  const auto reencoded = encodeLegacyChunk(chunk);
  const auto redecode_result = decodeLegacyChunk(reencoded);
  CC_CHECK(redecode_result.ok());
  CC_CHECK(redecode_result.chunk.has_value());
  CC_CHECK(redecode_result.chunk->programs.size() == chunk.programs.size());
  CC_CHECK(redecode_result.chunk->programs.front().name == chunk.programs.front().name);
  CC_CHECK(redecode_result.chunk->programs.back().name == chunk.programs.back().name);
}

}  // namespace

int main() {
  LegacyChunk synthetic_chunk;
  synthetic_chunk.programs.push_back(makeProgram("First Program", 0.10F));
  synthetic_chunk.programs.push_back(makeProgram("Second Program", 0.20F));

  const auto encoded = encodeLegacyChunk(synthetic_chunk);
  const auto decoded = decodeLegacyChunk(encoded);
  CC_CHECK(decoded.ok());
  CC_CHECK(decoded.chunk.has_value());
  CC_CHECK(decoded.chunk->programs.size() == 2U);
  CC_CHECK(decoded.chunk->programs[0].name == "First Program");
  CC_CHECK(nearlyEqual(decoded.chunk->programs[1].parameter_values[3], 0.29375F));

  const auto imported_synthetic =
      importLegacyState(decoded.chunk.value(), 1, decoded.chunk->programs[1].parameter_values);
  CC_CHECK(imported_synthetic.current_state_source ==
         LegacyCurrentStateSource::kExplicitParameters);
  CC_CHECK(imported_synthetic.selected_program_index == 1U);
  CC_CHECK(imported_synthetic.selected_program_matches_explicit_parameters);
  CC_CHECK(imported_synthetic.parameter_mismatches.empty());

  const auto root = std::filesystem::path(CAMELCRUSHER_RECALLED_SOURCE_DIR);
  assertRealFixtureRoundTrip(
      root / "tests/fixtures/catalyst_patron_camelcrusher.json");
  assertRealFixtureRoundTrip(
      root / "tests/fixtures/bye_rmx_patron_camelcrusher.json");

  std::cout << "Legacy chunk smoke test passed\n";
  return 0;
}
