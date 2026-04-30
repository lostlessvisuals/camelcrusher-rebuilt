#include "camelcrusher_recalled/LegacyChunk.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <string>

namespace camelcrusher_recalled {
namespace {

template <typename T>
bool readLittleEndian(std::span<const std::byte> bytes,
                      std::size_t offset,
                      T& destination) {
  if (offset + sizeof(T) > bytes.size()) {
    return false;
  }

  std::memcpy(&destination, bytes.data() + offset, sizeof(T));
  return true;
}

template <typename T>
void writeLittleEndian(std::vector<std::byte>& output, const T& value) {
  const auto* raw_bytes = reinterpret_cast<const std::byte*>(&value);
  output.insert(output.end(), raw_bytes, raw_bytes + sizeof(T));
}

std::optional<std::uint8_t> hexNibble(const char character) {
  if (character >= '0' && character <= '9') {
    return static_cast<std::uint8_t>(character - '0');
  }
  if (character >= 'a' && character <= 'f') {
    return static_cast<std::uint8_t>(character - 'a' + 10);
  }
  if (character >= 'A' && character <= 'F') {
    return static_cast<std::uint8_t>(character - 'A' + 10);
  }
  return std::nullopt;
}

}  // namespace

LegacyChunkDecodeResult decodeLegacyChunk(std::span<const std::byte> bytes) {
  LegacyChunk chunk;
  std::size_t offset = 0;

  while (offset < bytes.size()) {
    bool is_padding = true;
    for (std::size_t index = offset; index < bytes.size(); ++index) {
      if (bytes[index] != std::byte{0}) {
        is_padding = false;
        break;
      }
    }
    if (is_padding) {
      break;
    }

    LegacyProgram program;
    if (!readLittleEndian(bytes, offset, program.record_marker)) {
      return {.error = LegacyChunkDecodeError{
                  .offset = offset, .message = "Missing record marker"}}; 
    }
    offset += sizeof(float);

    const auto name_begin = offset;
    while (offset < bytes.size() && bytes[offset] != std::byte{0}) {
      ++offset;
    }
    if (offset >= bytes.size()) {
      return {.error = LegacyChunkDecodeError{
                  .offset = name_begin, .message = "Unterminated program name"}}; 
    }

    program.name.assign(reinterpret_cast<const char*>(bytes.data() + name_begin),
                        offset - name_begin);
    ++offset;  // trailing NUL

    std::uint32_t parameter_count = 0;
    if (!readLittleEndian(bytes, offset, parameter_count)) {
      return {.error = LegacyChunkDecodeError{
                  .offset = offset, .message = "Missing parameter count"}}; 
    }
    offset += sizeof(std::uint32_t);

    if (parameter_count != kLegacyParameterCount) {
      return {.error = LegacyChunkDecodeError{
                  .offset = offset - sizeof(std::uint32_t),
                  .message = "Unexpected parameter count"}}; 
    }

    for (std::size_t parameter_index = 0;
         parameter_index < kLegacyParameterSpecs.size();
         ++parameter_index) {
      if (!readLittleEndian(bytes, offset,
                            program.parameter_values[parameter_index])) {
        return {.error = LegacyChunkDecodeError{
                    .offset = offset, .message = "Truncated parameter block"}}; 
      }
      offset += sizeof(float);
    }

    chunk.programs.push_back(std::move(program));
  }

  return {.chunk = std::move(chunk)};
}

LegacyChunkDecodeResult decodeLegacyChunkHex(const std::string_view hex) {
  std::string compact;
  compact.reserve(hex.size());
  for (const char character : hex) {
    if (!std::isspace(static_cast<unsigned char>(character))) {
      compact.push_back(character);
    }
  }

  if ((compact.size() % 2U) != 0U) {
    return {.error = LegacyChunkDecodeError{
                .offset = compact.size(), .message = "Odd-length hex string"}}; 
  }

  std::vector<std::byte> bytes;
  bytes.reserve(compact.size() / 2U);

  for (std::size_t index = 0; index < compact.size(); index += 2) {
    const auto high = hexNibble(compact[index]);
    const auto low = hexNibble(compact[index + 1]);
    if (!high.has_value() || !low.has_value()) {
      return {.error = LegacyChunkDecodeError{
                  .offset = index, .message = "Invalid hex digit"}}; 
    }

    const auto value =
        static_cast<std::uint8_t>((high.value() << 4U) | low.value());
    bytes.push_back(static_cast<std::byte>(value));
  }

  return decodeLegacyChunk(bytes);
}

std::vector<std::byte> encodeLegacyChunk(const LegacyChunk& chunk) {
  std::vector<std::byte> bytes;

  for (const auto& program : chunk.programs) {
    writeLittleEndian(bytes, program.record_marker);
    bytes.insert(bytes.end(),
                 reinterpret_cast<const std::byte*>(program.name.data()),
                 reinterpret_cast<const std::byte*>(program.name.data()) +
                     program.name.size());
    bytes.push_back(std::byte{0});

    const auto parameter_count =
        static_cast<std::uint32_t>(program.parameter_values.size());
    writeLittleEndian(bytes, parameter_count);

    for (const auto value : program.parameter_values) {
      writeLittleEndian(bytes, value);
    }
  }

  return bytes;
}

bool isLikelyLegacyChunk(const LegacyChunk& chunk) {
  if (chunk.programs.empty()) {
    return false;
  }

  return std::all_of(
      chunk.programs.begin(),
      chunk.programs.end(),
      [](const LegacyProgram& program) { return !program.name.empty(); });
}

}  // namespace camelcrusher_recalled
