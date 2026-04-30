#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "$0")/.." && pwd)"
source "$repo_root/tools/camelcrusher_metadata.sh"
build_dir="${CAMELCRUSHER_BUILD_DIR:-$repo_root/build}"
plugin_name="$(camelcrusher_metadata_value CAMELCRUSHER_VST2_BUNDLE_NAME)"
project_title="$(camelcrusher_metadata_value CAMELCRUSHER_PROJECT_TITLE)"
compatibility_marker_key="$(camelcrusher_metadata_value CAMELCRUSHER_VST2_COMPATIBILITY_MARKER_KEY)"
target_bundle="${CAMELCRUSHER_VST2_TARGET_BUNDLE:-$HOME/Library/Audio/Plug-Ins/VST/${plugin_name}.vst}"
backup_root="${CAMELCRUSHER_VST2_BACKUP_ROOT:-$HOME/Library/Application Support/${project_title}/VST2-backups}"
manifest_path="${backup_root}/active-backup-path.txt"
action="${1:-status}"

find_bundle() {
  find "$build_dir" -type d -name "${plugin_name}.vst" | head -n 1
}

is_repo_built_bundle() {
  local bundle="$1"
  /usr/libexec/PlistBuddy -c "Print :${compatibility_marker_key}" \
    "$bundle/Contents/Info.plist" >/dev/null 2>&1
}

ensure_target_parent() {
  mkdir -p "$(dirname "$target_bundle")"
  mkdir -p "$backup_root"
}

print_status() {
  printf 'Target: %s\n' "$target_bundle"
  if [[ -e "$target_bundle" ]]; then
    if is_repo_built_bundle "$target_bundle"; then
      echo "State: repo-built VST2 bundle installed"
    else
      echo "State: another CamelCrusher bundle is currently installed"
    fi
  else
    echo "State: target bundle missing"
  fi

  if [[ -f "$manifest_path" ]]; then
    printf 'Recorded backup: %s\n' "$(cat "$manifest_path")"
  else
    echo "Recorded backup: none"
  fi
}

activate_repo_bundle() {
  ensure_target_parent
  local source_bundle
  source_bundle="$(find_bundle)"
  if [[ -z "${source_bundle:-}" ]]; then
      echo "Could not find ${plugin_name}.vst under $build_dir" >&2
      echo "Build the VST2 target first." >&2
      exit 1
    fi

  local backup_path=""
  if [[ -e "$target_bundle" ]]; then
    if is_repo_built_bundle "$target_bundle"; then
      rm -rf "$target_bundle"
    else
      backup_path="${backup_root}/${plugin_name}.$(date +%Y%m%d-%H%M%S).vst"
      mv "$target_bundle" "$backup_path"
      printf '%s\n' "$backup_path" >"$manifest_path"
    fi
  fi

  cp -R "$source_bundle" "$target_bundle"
  if command -v codesign >/dev/null 2>&1; then
    codesign --force --sign - --deep --timestamp=none "$target_bundle" >/dev/null 2>&1 || true
  fi

  printf 'Installed repo-built VST2 bundle at %s\n' "$target_bundle"
  if [[ -n "$backup_path" ]]; then
    printf 'Backed up previous bundle to %s\n' "$backup_path"
  fi
}

restore_previous_bundle() {
  ensure_target_parent

  if [[ -e "$target_bundle" ]]; then
    if is_repo_built_bundle "$target_bundle"; then
      rm -rf "$target_bundle"
    else
      echo "Refusing to remove a CamelCrusher bundle at $target_bundle that was not built by this repo." >&2
      exit 1
    fi
  fi

  if [[ ! -f "$manifest_path" ]]; then
    echo "No recorded backup to restore." >&2
    exit 1
  fi

  local backup_path
  backup_path="$(cat "$manifest_path")"
  if [[ ! -e "$backup_path" ]]; then
    echo "Recorded backup path no longer exists: $backup_path" >&2
    exit 1
  fi

  mv "$backup_path" "$target_bundle"
  rm -f "$manifest_path"
  printf 'Restored previous bundle to %s\n' "$target_bundle"
}

case "$action" in
  activate)
    activate_repo_bundle
    ;;
  restore)
    restore_previous_bundle
    ;;
  status)
    print_status
    ;;
  *)
    echo "Usage: $0 [activate|restore|status]" >&2
    exit 1
    ;;
esac
