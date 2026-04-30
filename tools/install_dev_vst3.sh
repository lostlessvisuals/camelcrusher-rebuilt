#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "$0")/.." && pwd)"
source "$repo_root/tools/camelcrusher_metadata.sh"
build_dir="${CAMELCRUSHER_BUILD_DIR:-$repo_root/build}"
action="${1:-install}"
target_dir="${HOME}/Library/Audio/Plug-Ins/VST3"
plugin_name="$(camelcrusher_metadata_value CAMELCRUSHER_PLUGIN_NAME)"
target_bundle="${target_dir}/${plugin_name}.vst3"

find_bundle() {
  find "$build_dir" -type d -name "${plugin_name}.vst3" | head -n 1
}

case "$action" in
  install)
    source_bundle="$(find_bundle)"
    if [[ -z "${source_bundle:-}" ]]; then
      echo "Could not find ${plugin_name}.vst3 under $build_dir" >&2
      echo "Build the VST3 target first." >&2
      exit 1
    fi

    mkdir -p "$target_dir"
    rm -rf "$target_bundle"
    cp -R "$source_bundle" "$target_bundle"

    if command -v codesign >/dev/null 2>&1; then
      codesign --force --sign - --deep --timestamp=none "$target_bundle" >/dev/null 2>&1 || true
    fi

    printf 'Installed %s\n' "$target_bundle"
    ;;
  uninstall)
    rm -rf "$target_bundle"
    printf 'Removed %s\n' "$target_bundle"
    ;;
  *)
    echo "Usage: $0 [install|uninstall]" >&2
    exit 1
    ;;
esac
