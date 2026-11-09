#!/usr/bin/env bash
set -euo pipefail

for root in /Library "$HOME/Library"; do
  for format in VST VST2 VST3; do
    suffix=vst
    [[ "$format" == VST3 ]] && suffix=vst3
    bundle="$root/Audio/Plug-Ins/$format/CamelCrusher.$suffix"
    [[ -e "$bundle" || -L "$bundle" ]] || continue
    printf '\n%s\n' "$bundle"
    if [[ -L "$bundle" ]]; then
      printf 'Alias target: %s\n' "$(readlink "$bundle")"
    fi
    if [[ ! -d "$bundle" ]]; then
      echo 'WARNING: broken alias'
      continue
    fi
    plist="$bundle/Contents/Info.plist"
    for key in CFBundleIdentifier CFBundleVersion CamelCrusherRebuiltReleaseVersion CamelCrusherRebuiltReleaseBuild; do
      value="$(/usr/libexec/PlistBuddy -c "Print :$key" "$plist" 2>/dev/null || true)"
      [[ -z "$value" ]] || printf '%s: %s\n' "$key" "$value"
    done
    shasum -a 256 "$bundle/Contents/MacOS/CamelCrusher"
  done
done
printf '\nMultiple real bundles can shadow an update. Restart the host after installing.\n'
