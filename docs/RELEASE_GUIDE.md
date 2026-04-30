# Release Guide

## Goal

Build one macOS installer that can install:

- `AU` via the containing host app
- same-identity `VST2`
- `VST3`

## Commands

- Release build and installer staging:
  - `tools/build_macos_release.sh`
- Optional signing for AU bundles during the build:
  - `CODESIGN_IDENTITY="Apple Development: ..." tools/build_macos_release.sh`
- Optional installer signing:
  - `CODESIGN_IDENTITY="Apple Development: ..." INSTALLER_SIGN_IDENTITY="Developer ID Installer: ..." tools/build_macos_release.sh`

## Output Shape

The staged release bundle lands under:

- `release/CamelCrusher-Rebuilt-<version>-macOS/`

That folder contains:

- `Install CamelCrusher.pkg`
- `Uninstall CamelCrusher.command`
- `README.txt`
- `SHA256SUMS.txt`
- intermediate component packages and payload folders

## Installer Choices

The installer exposes separate choices for:

- `Audio Unit (AU)`
- `VST2`
- `VST3`

The AU choice installs `/Applications/CamelCrusher Host.app` and runs a postinstall hook that launches the host app once so PlugInKit can register the embedded AU.

## Notes

- The release builder now uses the vendored `VST2` SDK slice in `third_party/vst2sdk` by default.
- The `VST2` option is meant for legacy-session compatibility on Apple Silicon Macs and now ships in the same installer family as `AU` and `VST3`.
- The staged release is intentionally close to the local build layout. This pass does not create a separate “marketing” product tree with different assets or plugin identities.
- The AU subpackage is built with an explicit component property list so PackageKit does not treat same-ID backups of `CamelCrusher Host.app` outside `/Applications` as relocated installs and skip the AU choice on reinstall.
- Do not deep re-sign the staged AU app bundle inside the release builder. The build step already signs `CamelCrusherHost.app` and `CamelCrusherAUUI.appex` with the sandbox entitlements, and re-signing the copied app with `codesign --deep` strips those entitlements and breaks PlugInKit registration.
- The `VST2` subpackage now also runs a postinstall step that creates compatibility aliases for the current console user under `~/Library/Audio/Plug-Ins/VST/` and `~/Library/Audio/Plug-Ins/VST2/`, plus a legacy local-system alias under `/Library/Audio/Plug-Ins/VST2/`. That is deliberate: older Live projects on this machine restored CamelCrusher from the user VST path, while extracted fixtures also reference the historical `VST2` directory name.
- Expanded package BOMs may still show AppleDouble-style `._*` receipt paths. That appears to come from macOS `com.apple.provenance` metadata rather than literal hidden files in the repo or build outputs.
