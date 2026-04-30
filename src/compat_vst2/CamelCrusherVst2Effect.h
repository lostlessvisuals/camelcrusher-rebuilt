#pragma once

#include "camelcrusher_recalled/LegacyPresetBank.h"
#include "camelcrusher_recalled/LegacyState.h"
#include "camelcrusher_recalled/ModernPluginModel.h"
#include "camelcrusher_recalled/ModernProcessor.h"

#include "audioeffectx.h"

#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

namespace camelcrusher_recalled {

class CamelCrusherVst2Effect final : public AudioEffectX {
 public:
  explicit CamelCrusherVst2Effect(audioMasterCallback audio_master);
  ~CamelCrusherVst2Effect() override = default;

  void setSampleRate(float sample_rate) override;
  void setBlockSize(VstInt32 block_size) override;
  void resume() override;
  void suspend() override;

  void processReplacing(float** inputs,
                        float** outputs,
                        VstInt32 sample_frames) override;

  void setParameter(VstInt32 index, float value) override;
  float getParameter(VstInt32 index) override;

  void setProgram(VstInt32 program) override;
  void setProgramName(char* name) override;
  void getProgramName(char* name) override;
  bool getProgramNameIndexed(VstInt32 category,
                             VstInt32 index,
                             char* text) override;

  void getParameterLabel(VstInt32 index, char* label) override;
  void getParameterDisplay(VstInt32 index, char* text) override;
  void getParameterName(VstInt32 index, char* text) override;

  VstInt32 getChunk(void** data, bool is_preset) override;
  VstInt32 setChunk(void* data, VstInt32 byte_size, bool is_preset) override;

  bool getEffectName(char* name) override;
  bool getVendorString(char* text) override;
  bool getProductString(char* text) override;
  VstInt32 getVendorVersion() override;
  VstInt32 getVstVersion() override;
  VstPlugCategory getPlugCategory() override;
  VstInt32 canDo(char* text) override;
  bool getInputProperties(VstInt32 index, VstPinProperties* properties) override;
  bool getOutputProperties(VstInt32 index, VstPinProperties* properties) override;

 private:
  void prepareProcessor();
  void adoptProgram(VstInt32 program);
  void syncCurrentProgramState();
  void refreshProcessingState();

  static LegacyPresetBank normalizedBank(LegacyPresetBank bank);
  static LegacyPresetBank defaultProgramBank();
  static std::vector<std::byte> encodeBankChunk(const LegacyPresetBank& bank);
  static std::vector<std::byte> encodePresetChunk(const LegacyPreset& preset);
  static bool isValidProgramIndex(VstInt32 program);
  static void copyVstString(char* destination,
                            VstInt32 max_length,
                            std::string_view value);
  static float clampNormalized(float value);

  LegacyPresetBank preset_bank_;
  LegacyState current_state_{};
  ModernPluginState processing_state_{};
  ModernProcessor processor_;
  std::vector<std::byte> chunk_storage_;
};

AudioEffect* createCamelCrusherVst2Effect(audioMasterCallback audio_master);

}  // namespace camelcrusher_recalled
