#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
repo_root="$(cd "$script_dir/.." && pwd)"
source "$repo_root/tools/camelcrusher_metadata.sh"
build_dir=${BUILD_DIR:-"$repo_root/build"}
app_name="$(camelcrusher_metadata_value CAMELCRUSHER_AU_HOST_APP_EXECUTABLE).app"
installed_app_name="$(camelcrusher_metadata_value CAMELCRUSHER_AU_HOST_APP_INSTALL_NAME)"
build_app="$build_dir/$app_name"
install_root=${INSTALL_ROOT:-"$HOME/Applications"}
installed_app="$install_root/$installed_app_name"
bundle_id="$(camelcrusher_metadata_value CAMELCRUSHER_AU_BUNDLE_IDENTIFIER)"
probe_tool="$build_dir/modern_au_component_probe"
view_probe_tool="$build_dir/modern_au_view_probe"
codesign_identity=${CODESIGN_IDENTITY:-}
host_process_name="$(camelcrusher_metadata_value CAMELCRUSHER_AU_HOST_APP_EXECUTABLE)"
extension_process_name="$(camelcrusher_metadata_value CAMELCRUSHER_AU_EXTENSION_EXECUTABLE)"

usage() {
  cat <<EOF
Usage:
  $0 install
  $0 uninstall
  $0 status

Environment:
  BUILD_DIR     Override the build directory. Default: $build_dir
  INSTALL_ROOT  Override the install root. Default: $install_root
  CODESIGN_IDENTITY  Optional Apple signing identity. Defaults to ad hoc signing.
EOF
}

run_probe() {
  if [[ -x "$probe_tool" ]]; then
    "$probe_tool" || true
  else
    echo "Probe tool not found at $probe_tool"
  fi
}

command=${1:-}
if [[ -z "$command" ]]; then
  usage
  exit 1
fi

case "$command" in
  install)
    if [[ -n "$codesign_identity" ]]; then
      cmake -S "$repo_root" -B "$build_dir" \
        -DCAMELCRUSHER_AU_CODESIGN_IDENTITY="$codesign_identity"
    fi
    cmake --build "$build_dir" --target camelcrusher_modern_au_container_app modern_au_component_probe modern_au_view_probe
    pkill -x "$host_process_name" || true
    pkill -x "$extension_process_name" || true
    mkdir -p "$install_root"
    if [[ -e "$installed_app" ]]; then
      pluginkit -r "$installed_app" || true
    fi
    rm -rf "$installed_app"
    cp -R "$build_app" "$installed_app"
    pluginkit -a "$installed_app"
    open -g "$installed_app"
    sleep 2
    echo "Installed $installed_app"
    pluginkit -m -i "$bundle_id" || true
    run_probe
    if [[ -x "$view_probe_tool" ]]; then
      echo "GUI AU probe available at $view_probe_tool"
    fi
    ;;
  uninstall)
    pkill -x "$host_process_name" || true
    pkill -x "$extension_process_name" || true
    if [[ -e "$installed_app" ]]; then
      pluginkit -r "$installed_app" || true
      rm -rf "$installed_app"
      echo "Removed $installed_app"
    else
      echo "No installed app at $installed_app"
    fi
    ;;
  status)
    echo "Build app: $build_app"
    echo "Installed app: $installed_app"
    if [[ -e "$installed_app" ]]; then
      echo "Installed: yes"
    else
      echo "Installed: no"
    fi
    pluginkit -m -i "$bundle_id" || true
    run_probe
    if [[ -x "$view_probe_tool" ]]; then
      echo "GUI AU probe available at $view_probe_tool"
    fi
    ;;
  *)
    usage
    exit 1
    ;;
esac
