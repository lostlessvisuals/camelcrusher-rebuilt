#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "$0")/.." && pwd)"
source "$repo_root/tools/camelcrusher_metadata.sh"
build_dir="${CAMELCRUSHER_BUILD_DIR:-$repo_root/build}"
action="${1:-install}"
plugin_name="$(camelcrusher_metadata_value CAMELCRUSHER_VST2_BUNDLE_NAME)"
compatibility_marker_key="$(camelcrusher_metadata_value CAMELCRUSHER_VST2_COMPATIBILITY_MARKER_KEY)"
target_bundle="${CAMELCRUSHER_VST2_TARGET_BUNDLE:-$HOME/Library/Audio/Plug-Ins/VST/${plugin_name}.vst}"

find_bundle() {
  find "$build_dir" -type d -name "${plugin_name}.vst" | head -n 1
}

is_compatibility_bundle() {
  local bundle="$1"
  /usr/libexec/PlistBuddy -c "Print :${compatibility_marker_key}" \
    "$bundle/Contents/Info.plist" >/dev/null 2>&1
}

case "$action" in
  install)
    source_bundle="$(find_bundle)"
    if [[ -z "${source_bundle:-}" ]]; then
      echo "Could not find ${plugin_name}.vst under $build_dir" >&2
      echo "Build the VST2 target first." >&2
      exit 1
    fi

    mkdir -p "$(dirname "$target_bundle")"
    if [[ -e "$target_bundle" ]] && ! is_compatibility_bundle "$target_bundle"; then
      echo "Refusing to overwrite a CamelCrusher bundle at $target_bundle that was not built by this repo." >&2
      echo "Back up the existing bundle or set CAMELCRUSHER_VST2_TARGET_BUNDLE to an alternate path." >&2
      exit 1
    fi

    rm -rf "$target_bundle"
    cp -R "$source_bundle" "$target_bundle"

    if command -v codesign >/dev/null 2>&1; then
      codesign --force --sign - --deep --timestamp=none "$target_bundle" >/dev/null 2>&1 || true
    fi

    printf 'Installed %s\n' "$target_bundle"
    ;;
  uninstall)
    if [[ -e "$target_bundle" ]] && ! is_compatibility_bundle "$target_bundle"; then
      echo "Refusing to remove a CamelCrusher bundle at $target_bundle that was not built by this repo." >&2
      exit 1
    fi
    rm -rf "$target_bundle"
    printf 'Removed %s\n' "$target_bundle"
    ;;
  *)
    echo "Usage: $0 [install|uninstall]" >&2
    exit 1
    ;;
esac
