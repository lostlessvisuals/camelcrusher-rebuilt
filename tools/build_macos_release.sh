#!/usr/bin/env bash
set -euo pipefail
export COPYFILE_DISABLE=1

script_dir="$(cd "$(dirname "$0")" && pwd)"
repo_root="$(cd "$script_dir/.." && pwd)"
source "$repo_root/tools/camelcrusher_metadata.sh"

build_dir="${BUILD_DIR:-$repo_root/build-release}"
release_root="${RELEASE_ROOT:-$repo_root/release}"
vst2_sdk_root="${CAMELCRUSHER_VST2_SDK_ROOT:-$repo_root/third_party/vst2sdk}"
codesign_identity="${CODESIGN_IDENTITY:--}"
installer_sign_identity="${INSTALLER_SIGN_IDENTITY:-}"
plugin_name="$(camelcrusher_metadata_value CAMELCRUSHER_PLUGIN_NAME)"
project_title="$(camelcrusher_metadata_value CAMELCRUSHER_PROJECT_TITLE)"
release_version="$(camelcrusher_metadata_value CAMELCRUSHER_RELEASE_VERSION)"
host_app_exec="$(camelcrusher_metadata_value CAMELCRUSHER_AU_HOST_APP_EXECUTABLE)"
host_app_install_name="$(camelcrusher_metadata_value CAMELCRUSHER_AU_HOST_APP_INSTALL_NAME)"
vst2_bundle_name="$(camelcrusher_metadata_value CAMELCRUSHER_VST2_BUNDLE_NAME)"
final_stage_dir="$release_root/${project_title// /-}-${release_version}-macOS"
stage_dir="$(mktemp -d /tmp/camelcrusher-release-stage.XXXXXX)"
packages_dir="$stage_dir/packages"
scripts_dir="$stage_dir/package-scripts"
payload_dir="$stage_dir/payloads"
resources_dir="$stage_dir/resources"
installer_path="$stage_dir/Install ${plugin_name}.pkg"
unsigned_installer_path="$stage_dir/Install ${plugin_name} unsigned.pkg"
distribution_path="$stage_dir/Distribution.xml"
au_component_plist="$stage_dir/au-components.plist"

cleanup() {
  rm -rf "$stage_dir"
}

trap cleanup EXIT

metadata_value() {
  camelcrusher_metadata_value "$1"
}

sign_bundle_if_possible() {
  local target="$1"
  if command -v codesign >/dev/null 2>&1; then
    codesign --force --sign "$codesign_identity" --deep --timestamp=none "$target" >/dev/null 2>&1 || true
  fi
}

configure_au_component_plist() {
  pkgbuild --analyze --root "$payload_dir/au" "$au_component_plist" >/dev/null

  /usr/libexec/PlistBuddy -c "Set :0:BundleIsRelocatable false" "$au_component_plist"
  /usr/libexec/PlistBuddy -c "Set :0:BundleIsVersionChecked false" "$au_component_plist"
  /usr/libexec/PlistBuddy -c "Set :0:BundleHasStrictIdentifier false" "$au_component_plist"
  /usr/libexec/PlistBuddy -c "Set :0:BundleOverwriteAction upgrade" "$au_component_plist"

  /usr/libexec/PlistBuddy -c "Add :0:ChildBundles:0:BundleIsRelocatable bool false" "$au_component_plist" 2>/dev/null || \
    /usr/libexec/PlistBuddy -c "Set :0:ChildBundles:0:BundleIsRelocatable false" "$au_component_plist"
  /usr/libexec/PlistBuddy -c "Add :0:ChildBundles:0:BundleIsVersionChecked bool false" "$au_component_plist" 2>/dev/null || \
    /usr/libexec/PlistBuddy -c "Set :0:ChildBundles:0:BundleIsVersionChecked false" "$au_component_plist"
  /usr/libexec/PlistBuddy -c "Add :0:ChildBundles:0:BundleHasStrictIdentifier bool false" "$au_component_plist" 2>/dev/null || \
    /usr/libexec/PlistBuddy -c "Set :0:ChildBundles:0:BundleHasStrictIdentifier false" "$au_component_plist"
  /usr/libexec/PlistBuddy -c "Set :0:ChildBundles:0:BundleOverwriteAction upgrade" "$au_component_plist"
}

rm -rf "$final_stage_dir"
mkdir -p "$final_stage_dir" "$packages_dir" "$scripts_dir/au" "$payload_dir" "$resources_dir"
mkdir -p "$scripts_dir/vst2"

if [[ "$codesign_identity" == "-" ]]; then
  echo "Warning: building the release bundle with ad hoc code signing. Use CODESIGN_IDENTITY for a redistributable AU install." >&2
fi
if [[ -z "$installer_sign_identity" ]]; then
  echo "Warning: installer package will be unsigned. Use INSTALLER_SIGN_IDENTITY for a Developer ID Installer signature." >&2
fi

cmake -S "$repo_root" -B "$build_dir" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCAMELCRUSHER_ENABLE_VST2_COMPATIBILITY=ON \
  -DCAMELCRUSHER_VST2_SDK_ROOT="$vst2_sdk_root" \
  -DCAMELCRUSHER_AU_CODESIGN_IDENTITY="$codesign_identity"

cmake --build "$build_dir" -- -j4

cmake --build "$build_dir" --target \
  camelcrusher_modern_au_container_app \
  camelcrusher_modern_vst3 \
  modern_au_bundle_smoke \
  modern_au_container_smoke \
  modern_vst3_bundle_smoke \
  vst2_bundle_smoke -- -j4

ctest --test-dir "$build_dir" --output-on-failure

au_build_app="$build_dir/${host_app_exec}.app"
vst3_build_bundle="$(find "$build_dir" -type d -name "${plugin_name}.vst3" | head -n 1)"
vst2_build_bundle="$(find "$build_dir" -type d -name "${vst2_bundle_name}.vst" | head -n 1)"

if [[ ! -d "$au_build_app" || -z "${vst3_build_bundle:-}" || -z "${vst2_build_bundle:-}" ]]; then
  echo "Release artifacts were missing after build." >&2
  exit 1
fi

au_payload_root="$payload_dir/au/Applications"
vst3_payload_root="$payload_dir/vst3/Library/Audio/Plug-Ins/VST3"
vst2_payload_root="$payload_dir/vst2/Library/Audio/Plug-Ins/VST"
mkdir -p "$au_payload_root" "$vst3_payload_root" "$vst2_payload_root"

cp -R "$au_build_app" "$au_payload_root/$host_app_install_name"
cp -R "$vst3_build_bundle" "$vst3_payload_root/${plugin_name}.vst3"
cp -R "$vst2_build_bundle" "$vst2_payload_root/${vst2_bundle_name}.vst"

if command -v dot_clean >/dev/null 2>&1; then
  dot_clean -m "$payload_dir" >/dev/null 2>&1 || true
  dot_clean -m "$resources_dir" >/dev/null 2>&1 || true
fi

configure_au_component_plist

sign_bundle_if_possible "$vst3_payload_root/${plugin_name}.vst3"
sign_bundle_if_possible "$vst2_payload_root/${vst2_bundle_name}.vst"

if command -v xattr >/dev/null 2>&1; then
  xattr -cr "$payload_dir" >/dev/null 2>&1 || true
  xattr -cr "$resources_dir" >/dev/null 2>&1 || true
fi

if command -v dot_clean >/dev/null 2>&1; then
  dot_clean -m "$payload_dir" >/dev/null 2>&1 || true
  dot_clean -m "$resources_dir" >/dev/null 2>&1 || true
fi

sed \
  -e "s|@CAMELCRUSHER_AU_HOST_APP_INSTALL_NAME@|$host_app_install_name|g" \
  "$repo_root/packaging/macos/postinstall-au.sh.in" >"$scripts_dir/au/postinstall"
chmod +x "$scripts_dir/au/postinstall"

sed \
  -e "s|@CAMELCRUSHER_VST2_BUNDLE_NAME@|$vst2_bundle_name|g" \
  "$repo_root/packaging/macos/postinstall-vst2.sh.in" >"$scripts_dir/vst2/postinstall"
chmod +x "$scripts_dir/vst2/postinstall"

pkgbuild \
  --root "$payload_dir/au" \
  --component-plist "$au_component_plist" \
  --identifier "$(metadata_value CAMELCRUSHER_INSTALLER_AU_PKG_IDENTIFIER)" \
  --version "$release_version" \
  --install-location "/" \
  --scripts "$scripts_dir/au" \
  "$packages_dir/camelcrusher-au.pkg"

pkgbuild \
  --root "$payload_dir/vst3" \
  --identifier "$(metadata_value CAMELCRUSHER_INSTALLER_VST3_PKG_IDENTIFIER)" \
  --version "$release_version" \
  --install-location "/" \
  "$packages_dir/camelcrusher-vst3.pkg"

pkgbuild \
  --root "$payload_dir/vst2" \
  --identifier "$(metadata_value CAMELCRUSHER_INSTALLER_VST2_PKG_IDENTIFIER)" \
  --version "$(metadata_value CAMELCRUSHER_VST2_LEGACY_VERSION_STRING)" \
  --install-location "/" \
  --scripts "$scripts_dir/vst2" \
  "$packages_dir/camelcrusher-vst2.pkg"

cat >"$resources_dir/README.txt" <<EOF
${project_title} ${release_version}

This release installs the same local plug-in family currently used in development:
- Audio Unit via ${host_app_install_name}
- VST3 as ${plugin_name}.vst3
- VST2 as ${vst2_bundle_name}.vst

The VST2 option is intended for legacy-session compatibility on Apple Silicon Macs.
EOF

sed \
  -e "s|@CAMELCRUSHER_PLUGIN_NAME@|$plugin_name|g" \
  -e "s|@CAMELCRUSHER_INSTALLER_PRODUCT_IDENTIFIER@|$(metadata_value CAMELCRUSHER_INSTALLER_PRODUCT_IDENTIFIER)|g" \
  -e "s|@CAMELCRUSHER_INSTALLER_AU_PKG_IDENTIFIER@|$(metadata_value CAMELCRUSHER_INSTALLER_AU_PKG_IDENTIFIER)|g" \
  -e "s|@CAMELCRUSHER_INSTALLER_VST3_PKG_IDENTIFIER@|$(metadata_value CAMELCRUSHER_INSTALLER_VST3_PKG_IDENTIFIER)|g" \
  -e "s|@CAMELCRUSHER_INSTALLER_VST2_PKG_IDENTIFIER@|$(metadata_value CAMELCRUSHER_INSTALLER_VST2_PKG_IDENTIFIER)|g" \
  -e "s|@CAMELCRUSHER_RELEASE_VERSION@|$release_version|g" \
  -e "s|@CAMELCRUSHER_VST2_LEGACY_VERSION_STRING@|$(metadata_value CAMELCRUSHER_VST2_LEGACY_VERSION_STRING)|g" \
  "$repo_root/packaging/macos/Distribution.xml.in" >"$distribution_path"

cp "$repo_root/tools/uninstall_camelcrusher.sh" "$stage_dir/Uninstall CamelCrusher.command"
chmod +x "$stage_dir/Uninstall CamelCrusher.command"

productbuild \
  --distribution "$distribution_path" \
  --package-path "$packages_dir" \
  --resources "$resources_dir" \
  "$unsigned_installer_path"

if [[ -n "$installer_sign_identity" ]]; then
  productsign --sign "$installer_sign_identity" "$unsigned_installer_path" "$installer_path"
  rm -f "$unsigned_installer_path"
else
  mv "$unsigned_installer_path" "$installer_path"
fi

(cd "$stage_dir" && shasum -a 256 "$(basename "$installer_path")" >SHA256SUMS.txt)

rsync -a --delete "$stage_dir"/ "$final_stage_dir"/

echo "Release bundle ready at $final_stage_dir"
