# CamelCrusher Rebuilt 0.7.3

## Fixes

- Corrected VST2 channel metadata and host callbacks so hosts receive valid stereo information.
- Removed the workaround that copied left over a silent right channel. Hard-panned and delayed right-channel audio now stay intact, including when bypassed.
- Corrected the legacy accumulating audio callback and removed audio-thread file logging.
- Added proper VST3 mono-input/stereo-output negotiation and preserved filter tails when inputs become silent.
- Hardened AU stereo buffers, including host-provided null output buffers. AU supports noninterleaved Float32 stereo; unsupported formats are explicitly rejected rather than silently misprocessed.
- Made test checks run in Release builds and added independent packaged-binary tests, channel-layout tests, and wet-preset checks.
- Synchronized the public VST2 source/header and pinned the complete VST2 interface definitions with their license notice.

## Installing

Quit your audio host, install the new package, then reopen the host. The installer offers AU, VST2, and VST3. Plug-in names, IDs, parameter ordering, and legacy chunk formats remain unchanged.

Older real VST2 bundles at compatibility-alias paths are moved out of scan folders into `Library/Application Support/CamelCrusherRebuilt/Backups` before creating aliases to the updated system bundle. Originals are preserved, not deleted. This handles the current console user; other macOS accounts may still have their own copies.

For a diagnostic listing of user/system VST2 and VST3 copies, run `bash tools/diagnose_installation.sh`. The VST2 bundle keeps its legacy version fields for compatibility and adds separate Rebuilt release/build metadata.

The package retains ad hoc plug-in signing and is not Developer ID notarized. These fixes do not claim exact sonic parity with the original CamelCrusher or confirmation in every host. The original reporter's host and format remain unknown.

## Validation

The release builder runs the complete test suite with checks enabled, expands the final installer, and tests its actual VST2 and VST3 payloads. The AU routing tests run in-process; package validation also checks AU payload equality and signatures. Installed/out-of-process AU and the reporter's actual host still need a listening check.
