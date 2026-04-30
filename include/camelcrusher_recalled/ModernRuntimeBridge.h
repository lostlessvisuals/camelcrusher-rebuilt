#pragma once

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
#if defined(CAMELCRUSHER_RUNTIME_BRIDGE_BUILD)
#define CAMELCRUSHER_RUNTIME_BRIDGE_API __declspec(dllexport)
#else
#define CAMELCRUSHER_RUNTIME_BRIDGE_API __declspec(dllimport)
#endif
#else
#define CAMELCRUSHER_RUNTIME_BRIDGE_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum {
  CAMELCRUSHER_RUNTIME_BRIDGE_MAX_ERROR_MESSAGE = 128,
  CAMELCRUSHER_RUNTIME_BRIDGE_MAX_PROGRAM_NAME = 64,
};

typedef enum CamelCrusherRuntimeBridgeValueKind {
  CAMELCRUSHER_RUNTIME_BRIDGE_VALUE_CONTINUOUS_NORMALIZED = 0,
  CAMELCRUSHER_RUNTIME_BRIDGE_VALUE_SWITCH_LIKE = 1,
} CamelCrusherRuntimeBridgeValueKind;

typedef struct CamelCrusherRuntimeBridgePublicParameterDescriptor {
  size_t public_index;
  size_t legacy_index;
  const char* stable_id;
  const char* display_name;
  uint32_t value_kind;
} CamelCrusherRuntimeBridgePublicParameterDescriptor;

typedef struct CamelCrusherRuntimeBridgeTextError {
  size_t offset;
  char message[CAMELCRUSHER_RUNTIME_BRIDGE_MAX_ERROR_MESSAGE];
} CamelCrusherRuntimeBridgeTextError;

typedef struct CamelCrusherRuntimeBridgeLegacyImportSummary {
  int selected_program_present;
  size_t selected_program_index;
  char selected_program_name[CAMELCRUSHER_RUNTIME_BRIDGE_MAX_PROGRAM_NAME];
  size_t preset_count;
  size_t mismatch_count;
} CamelCrusherRuntimeBridgeLegacyImportSummary;

typedef struct CamelCrusherRuntimeBridgeProcessSpec {
  double sample_rate;
  size_t max_block_size;
  size_t channel_count;
} CamelCrusherRuntimeBridgeProcessSpec;

typedef struct CamelCrusherRuntimeBridgeRuntime
    CamelCrusherRuntimeBridgeRuntime;

CAMELCRUSHER_RUNTIME_BRIDGE_API size_t
camelcrusher_runtime_bridge_public_parameter_count(void);

CAMELCRUSHER_RUNTIME_BRIDGE_API int
camelcrusher_runtime_bridge_get_public_parameter_descriptor(
    size_t public_index,
    CamelCrusherRuntimeBridgePublicParameterDescriptor* out_descriptor);

CAMELCRUSHER_RUNTIME_BRIDGE_API CamelCrusherRuntimeBridgeRuntime*
camelcrusher_runtime_bridge_runtime_create(void);

CAMELCRUSHER_RUNTIME_BRIDGE_API void
camelcrusher_runtime_bridge_runtime_destroy(
    CamelCrusherRuntimeBridgeRuntime* runtime);

CAMELCRUSHER_RUNTIME_BRIDGE_API void
camelcrusher_runtime_bridge_runtime_prepare(
    CamelCrusherRuntimeBridgeRuntime* runtime,
    CamelCrusherRuntimeBridgeProcessSpec spec);

CAMELCRUSHER_RUNTIME_BRIDGE_API void
camelcrusher_runtime_bridge_runtime_reset(
    CamelCrusherRuntimeBridgeRuntime* runtime);

CAMELCRUSHER_RUNTIME_BRIDGE_API float
camelcrusher_runtime_bridge_runtime_get_public_parameter(
    const CamelCrusherRuntimeBridgeRuntime* runtime,
    size_t public_index);

CAMELCRUSHER_RUNTIME_BRIDGE_API void
camelcrusher_runtime_bridge_runtime_set_public_parameter(
    CamelCrusherRuntimeBridgeRuntime* runtime,
    size_t public_index,
    float normalized_value);

CAMELCRUSHER_RUNTIME_BRIDGE_API float
camelcrusher_runtime_bridge_runtime_get_legacy_parameter(
    const CamelCrusherRuntimeBridgeRuntime* runtime,
    size_t legacy_index);

CAMELCRUSHER_RUNTIME_BRIDGE_API size_t
camelcrusher_runtime_bridge_runtime_preset_count(
    const CamelCrusherRuntimeBridgeRuntime* runtime);

CAMELCRUSHER_RUNTIME_BRIDGE_API size_t
camelcrusher_runtime_bridge_runtime_mismatch_count(
    const CamelCrusherRuntimeBridgeRuntime* runtime);

CAMELCRUSHER_RUNTIME_BRIDGE_API int
camelcrusher_runtime_bridge_runtime_get_selected_program_index(
    const CamelCrusherRuntimeBridgeRuntime* runtime,
    size_t* out_program_index);

CAMELCRUSHER_RUNTIME_BRIDGE_API int
camelcrusher_runtime_bridge_runtime_get_selected_program_name(
    const CamelCrusherRuntimeBridgeRuntime* runtime,
    char* out_name,
    size_t out_name_capacity);

CAMELCRUSHER_RUNTIME_BRIDGE_API int
camelcrusher_runtime_bridge_runtime_get_legacy_preset_name(
    const CamelCrusherRuntimeBridgeRuntime* runtime,
    size_t program_index,
    char* out_name,
    size_t out_name_capacity);

CAMELCRUSHER_RUNTIME_BRIDGE_API int
camelcrusher_runtime_bridge_runtime_select_legacy_preset(
    CamelCrusherRuntimeBridgeRuntime* runtime,
    size_t program_index,
    int adopt_preset_state);

CAMELCRUSHER_RUNTIME_BRIDGE_API int
camelcrusher_runtime_bridge_runtime_import_legacy_chunk(
    CamelCrusherRuntimeBridgeRuntime* runtime,
    const uint8_t* chunk_bytes,
    size_t chunk_size,
    int32_t program_number,
    const float* explicit_parameter_values,
    size_t explicit_parameter_count,
    CamelCrusherRuntimeBridgeLegacyImportSummary* out_summary,
    CamelCrusherRuntimeBridgeTextError* out_error);

CAMELCRUSHER_RUNTIME_BRIDGE_API size_t
camelcrusher_runtime_bridge_runtime_state_size(
    const CamelCrusherRuntimeBridgeRuntime* runtime);

CAMELCRUSHER_RUNTIME_BRIDGE_API int
camelcrusher_runtime_bridge_runtime_save_state(
    const CamelCrusherRuntimeBridgeRuntime* runtime,
    uint8_t* out_bytes,
    size_t out_bytes_capacity,
    size_t* out_bytes_written);

CAMELCRUSHER_RUNTIME_BRIDGE_API int
camelcrusher_runtime_bridge_runtime_load_state(
    CamelCrusherRuntimeBridgeRuntime* runtime,
    const uint8_t* bytes,
    size_t byte_count,
    CamelCrusherRuntimeBridgeTextError* out_error);

CAMELCRUSHER_RUNTIME_BRIDGE_API void
camelcrusher_runtime_bridge_runtime_process_stereo(
    CamelCrusherRuntimeBridgeRuntime* runtime,
    float* left,
    float* right,
    size_t frame_count);

#ifdef __cplusplus
}
#endif
