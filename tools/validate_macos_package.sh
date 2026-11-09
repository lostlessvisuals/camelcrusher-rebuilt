#!/usr/bin/env bash
set -euo pipefail

package="${1:?Usage: validate_macos_package.sh package.pkg build-directory}"
build_dir="${2:?Missing build directory}"
work="$(mktemp -d /tmp/camelcrusher-package-check.XXXXXX)"
trap 'rm -rf "$work"' EXIT
pkgutil --expand-full "$package" "$work/expanded"

vst2="$work/expanded/camelcrusher-vst2.pkg/Payload/Library/Audio/Plug-Ins/VST/CamelCrusher.vst"
vst3="$work/expanded/camelcrusher-vst3.pkg/Payload/Library/Audio/Plug-Ins/VST3/CamelCrusher.vst3"
au="$work/expanded/camelcrusher-au.pkg/Payload/Applications/CamelCrusher Host.app"
for bundle in "$vst2" "$vst3" "$au"; do
  test -d "$bundle"
  codesign --verify --deep --strict "$bundle"
done

"$build_dir/vst2_host_contract" "$vst2/Contents/MacOS/CamelCrusher"
"$build_dir/modern_vst3_bundle_smoke" "$vst3"
"$build_dir/au_stereo_contract"

# The AU extension is an executable, not a dlopen-able module. Check that the
# staged AU code and its runtime bridge are exactly the binaries just tested.
extension="Contents/PlugIns/CamelCrusherAUUI.appex/Contents"
cmp "$au/$extension/MacOS/CamelCrusherAU" \
    "$build_dir/CamelCrusherHost.app/$extension/MacOS/CamelCrusherAU"
cmp "$au/$extension/Frameworks/libcamelcrusher_runtime_bridge.dylib" \
    "$build_dir/CamelCrusherHost.app/$extension/Frameworks/libcamelcrusher_runtime_bridge.dylib"
echo "Packaged VST2/VST3 processing, AU source routing, signatures and AU payload checks passed."
