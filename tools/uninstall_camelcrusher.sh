#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "$0")/.." && pwd)"
source "$repo_root/tools/camelcrusher_metadata.sh"

plugin_name="$(camelcrusher_metadata_value CAMELCRUSHER_PLUGIN_NAME)"
vst2_name="$(camelcrusher_metadata_value CAMELCRUSHER_VST2_BUNDLE_NAME)"
app_name="$(camelcrusher_metadata_value CAMELCRUSHER_AU_HOST_APP_INSTALL_NAME)"
au_bundle_id="$(camelcrusher_metadata_value CAMELCRUSHER_AU_BUNDLE_IDENTIFIER)"
vst2_bundle_id="$(camelcrusher_metadata_value CAMELCRUSHER_VST2_BUNDLE_IDENTIFIER)"
host_bundle_id="$(camelcrusher_metadata_value CAMELCRUSHER_AU_HOST_APP_IDENTIFIER)"

safe_remove_bundle() {
  local target="$1"
  local expected_key="$2"
  local expected_value="$3"

  if [[ ! -e "$target" ]]; then
    return 0
  fi

  local info_plist="$target/Contents/Info.plist"
  if [[ ! -f "$info_plist" ]]; then
    echo "Skipping $target because Info.plist was missing."
    return 0
  fi

  local actual_value
  actual_value="$(/usr/libexec/PlistBuddy -c "Print :$expected_key" "$info_plist" 2>/dev/null || true)"
  if [[ "$actual_value" != "$expected_value" ]]; then
    echo "Skipping $target because $expected_key did not match."
    return 0
  fi

  rm -rf "$target"
  echo "Removed $target"
}

safe_remove_symlink() {
  local target="$1"

  if [[ -L "$target" ]]; then
    rm -f "$target"
    echo "Removed symlink $target"
  fi
}

safe_remove_bundle "/Applications/$app_name" "CFBundleIdentifier" "$host_bundle_id"
safe_remove_bundle "$HOME/Applications/$app_name" "CFBundleIdentifier" "$host_bundle_id"
safe_remove_bundle "/Library/Audio/Plug-Ins/VST3/${plugin_name}.vst3" "CFBundleIdentifier" "$(camelcrusher_metadata_value CAMELCRUSHER_VST3_BUNDLE_IDENTIFIER)"
safe_remove_bundle "$HOME/Library/Audio/Plug-Ins/VST3/${plugin_name}.vst3" "CFBundleIdentifier" "$(camelcrusher_metadata_value CAMELCRUSHER_VST3_BUNDLE_IDENTIFIER)"
safe_remove_symlink "/Library/Audio/Plug-Ins/VST2/${vst2_name}.vst"
safe_remove_symlink "$HOME/Library/Audio/Plug-Ins/VST2/${vst2_name}.vst"
safe_remove_symlink "$HOME/Library/Audio/Plug-Ins/VST/${vst2_name}.vst"
safe_remove_bundle "/Library/Audio/Plug-Ins/VST/${vst2_name}.vst" "CFBundleIdentifier" "$vst2_bundle_id"
safe_remove_bundle "$HOME/Library/Audio/Plug-Ins/VST/${vst2_name}.vst" "CFBundleIdentifier" "$vst2_bundle_id"

if command -v pluginkit >/dev/null 2>&1; then
  pluginkit -r "/Applications/$app_name" >/dev/null 2>&1 || true
  pluginkit -r "$HOME/Applications/$app_name" >/dev/null 2>&1 || true
  pluginkit -m -i "$au_bundle_id" >/dev/null 2>&1 || true
fi
