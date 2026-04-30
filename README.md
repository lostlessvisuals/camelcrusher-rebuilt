# CamelCrusher Rebuilt

![CamelCrusher UI preview](docs/assets/camelcrusher-ui-preview.png)

`CamelCrusher Rebuilt` is a native Apple Silicon remake of CamelCrusher for modern macOS.

It is focused on two outcomes:

- a native plug-in family for `AU`, `VST2`, and `VST3`
- a `VST2` path made to reopen older projects that used CamelCrusher

For a lot of modern Mac users, especially on Apple Silicon, the original CamelCrusher workflow became broken, Intel-only, or effectively unavailable. This project exists to make those older projects open again while also shipping a usable modern plug-in family under the same musical identity.

The workspace codename is still `camelcrusher-recalled` for continuity, but the current modern product name is `CamelCrusher Rebuilt`. The likely shipping split is:

- `CamelCrusher Rebuilt`: project/repo identity for the remake effort
- `CamelCrusher`: current host-facing plug-in name so AU/VST integration stays aligned with the original naming surface

Current status: the shared core and all three wrapper formats are working locally on this machine, including verified legacy `VST2` session recall in Ableton Live.

- shared C++ core scaffold with the frozen 17-parameter legacy schema
- wrapper-agnostic parameter metadata for the legacy 17-slot surface
- named legacy state containers and preset-bank abstractions
- legacy chunk decoder/encoder for the saved CamelCrusher preset buffer
- wrapper-agnostic legacy-state importer that resolves current values from chunk data plus Ableton's explicit parameter list
- wrapper-neutral modern plugin model with a 12-parameter public host view that preserves the 5 hidden compatibility slots
- wrapper-neutral modern processor with a more deliberate distortion/filter/compression path for real stereo sample processing
- wrapper-neutral modern runtime with save/load support for a public serialized state blob
- shared-library bridge API for thin custom wrappers and other tooling
- Python `modern_state_tool.py` CLI for describing parameters, importing fixture JSON into a modern state blob, inspecting saved state, and exporting host-loadable `.aupreset` files
- in-process macOS `AUAudioUnit` scaffold backed by the shared runtime bridge
- compiled-in 20-program CamelCrusher factory preset bank seeded from checked-in legacy fixture data, so a fresh AU instance already exposes the original program list
- same-identity `VST2` compatibility wrapper backed by the same shared core, with the legacy 17-parameter, 20-program, chunk-based host surface, native macOS preview editor, original raw `AEffect` flags/category mirrored from the legacy binary, and a faceplate-driven CamelCrusher-inspired aesthetic pass
- official Steinberg-SDK-backed `VST3` wrapper backed by the same shared runtime, public 12-parameter surface, 20 factory presets, and a native macOS preview editor
- assembled UI-bearing AU extension bundle under `build/CamelCrusherAUUI.appex`
- assembled containing app bundle under `build/CamelCrusherHost.app` with the AU extension embedded at `Contents/PlugIns/CamelCrusherAUUI.appex`
- assembled `VST2` bundle under `build/CamelCrusher.vst`
- assembled `VST3` bundle under `build/VST3/Debug/CamelCrusher.vst3`
- developer install helper plus a low-noise AU component probe for local registration checks
- guarded developer install helper and reversible swap helper for `VST2` testing
- developer install helper for local `VST3` copy into `~/Library/Audio/Plug-Ins/VST3`
- sandbox entitlements for the AU-containing app and embedded extension so PlugInKit will consider the bundle a valid AUv3-style install candidate
- the shared runtime bridge dylib embedded inside the `.appex`, so the registered extension can launch away from the build tree
- Ableton `.als` fixture extractor for CamelCrusher `VST2` state
- generated local fixture JSONs under `tests/fixtures/`, with public-safe source labels
- CMake and smoke-test wiring for the shared core, AU/VST3 wrappers, bridge, and tooling surface

Current commands:

- Build all three plug-in formats from the repo, including the vendored `VST2` SDK slice: `cmake -S . -B build && cmake --build build`
- Build against an alternate `VST2` SDK root if needed: `cmake -S . -B build -DCAMELCRUSHER_VST2_SDK_ROOT=/path/to/vst2sdk && cmake --build build`
- Test: `ctest --test-dir build --output-on-failure`
- Extract fixture: `python3 tools/extract_camelcrusher_fixture.py <fixture.als> --pretty -o tests/fixtures/<name>.json`
- Extract fixture for a public repo artifact: `python3 tools/extract_camelcrusher_fixture.py <fixture.als> --source-label <public-label> --pretty -o tests/fixtures/<name>.json`
- Describe the public parameter surface: `CAMELCRUSHER_BRIDGE_LIB=build/libcamelcrusher_runtime_bridge.dylib python3 tools/modern_state_tool.py describe-parameters --pretty`
- Import a checked-in fixture into a serialized modern state blob: `CAMELCRUSHER_BRIDGE_LIB=build/libcamelcrusher_runtime_bridge.dylib python3 tools/modern_state_tool.py import-fixture tests/fixtures/catalyst_patron_camelcrusher.json --instance-index 0 --output build/catalyst_instance_0.ccrstate --pretty`
- Inspect a serialized modern state blob: `CAMELCRUSHER_BRIDGE_LIB=build/libcamelcrusher_runtime_bridge.dylib python3 tools/modern_state_tool.py inspect-state build/catalyst_instance_0.ccrstate --pretty`
- Summarize a fixture into the same compare-friendly 17-parameter JSON shape used during legacy-state validation: `CAMELCRUSHER_BRIDGE_LIB=build/libcamelcrusher_runtime_bridge.dylib python3 tools/modern_state_tool.py summarize-fixture tests/fixtures/catalyst_patron_camelcrusher.json --instance-index 0 --pretty`
- Compare a captured VST2 state-summary JSON file against a fixture instance: `CAMELCRUSHER_BRIDGE_LIB=build/libcamelcrusher_runtime_bridge.dylib python3 tools/modern_state_tool.py compare-fixture-summary tests/fixtures/catalyst_patron_camelcrusher.json /tmp/camelcrusher-captured-state.json --instance-index 0 --pretty`
- Export a serialized modern state blob as an AU preset file: `CAMELCRUSHER_BRIDGE_LIB=build/libcamelcrusher_runtime_bridge.dylib python3 tools/modern_state_tool.py export-au-preset build/catalyst_instance_0.ccrstate --preset-name "Catalyst British Clean" --pretty`
- Convert a checked-in fixture directly into an AU preset file for manual host loading: `CAMELCRUSHER_BRIDGE_LIB=build/libcamelcrusher_runtime_bridge.dylib python3 tools/modern_state_tool.py fixture-to-au-preset tests/fixtures/catalyst_patron_camelcrusher.json --instance-index 0 --preset-name "Catalyst British Clean" --pretty`
- Build the AU scaffold smoke test: `cmake --build build --target modern_au_smoke`
- Build the extension-style AU bundle smoke test: `cmake --build build --target modern_au_bundle_smoke`
- Build the containing-app AU bundle smoke test: `cmake --build build --target modern_au_container_smoke`
- Build the in-process VST3 smoke test: `cmake --build build --target modern_vst3_smoke`
- Build the bundle-level VST3 smoke test: `cmake --build build --target modern_vst3_bundle_smoke`
- Build the VST2 identity/chunk smoke test: `cmake --build build --target vst2_compat_smoke`
- Build the VST2 bundle-entry smoke test: `cmake --build build --target vst2_bundle_smoke`
- Build the VST2 editor smoke test: `cmake --build build --target vst2_editor_smoke`
- Probe whether macOS currently sees the AU component and whether it advertises a custom view: `cmake --build build --target modern_au_component_probe && build/modern_au_component_probe`
- Try host-style instantiation of the registered AU component: `cmake --build build --target modern_au_component_instantiate && build/modern_au_component_instantiate`
- Open the registered AU in a tiny local AppKit host and request the actual custom view: `cmake --build build --target modern_au_view_probe && build/modern_au_view_probe`
- Install the dev containing app into `~/Applications`, register it with PlugInKit, and launch the background host once so the AU becomes visible: `tools/install_dev_au_host_app.sh install`
- Remove the dev containing app from `~/Applications`: `tools/install_dev_au_host_app.sh uninstall`
- Install the dev `VST3` bundle into `~/Library/Audio/Plug-Ins/VST3`: `tools/install_dev_vst3.sh install`
- Remove the dev `VST3` bundle from `~/Library/Audio/Plug-Ins/VST3`: `tools/install_dev_vst3.sh uninstall`
- Install the dev `VST2` bundle to a safe alternate path: `CAMELCRUSHER_VST2_TARGET_BUNDLE=/tmp/CamelCrusher-test.vst tools/install_dev_vst2.sh install`
- Back up the currently installed `CamelCrusher.vst` and activate the current repo-built bundle for real host testing: `tools/swap_installed_vst2.sh activate`
- Restore the previous `CamelCrusher.vst` after testing: `tools/swap_installed_vst2.sh restore`
- Build the release installer bundle with all three formats: `tools/build_macos_release.sh`
- Prepare a GitHub-ready export tree that stays close to the local repo while stripping generated distribution clutter: `tools/prepare_github_export.sh`

Notes:

- `tools/install_dev_au_host_app.sh` accepts `CODESIGN_IDENTITY=...` for real Apple development signing. The full local registration and instantiation path on this machine was verified with an Apple Development identity rather than ad hoc signing.
- The install helper now launches the dev host app once after copying it into `~/Applications`, because that first launch is what made the AU visible to PlugInKit on this machine.
- After that first registration launch, the dev host app does not need to stay running for component discovery or instantiation.
- The currently installed AU now registers with the host-visible name `CamelCrusher` and exposes 20 factory presets from `Annihilate` through `Turn it to 11`.
- The installed AU is now packaged as a single `com.apple.AudioUnit-UI` extension inside the dev host app, and the registered out-of-process component now reports `providesUserInterface = 1` and returns an `AUAudioUnitRemoteViewController`.
- `auvaltool -v aufx CcrR RvFx` now succeeds locally, so the AU currently clears Apple’s validation pass as well as the repo smoke tests.
- The dev host app now builds as `build/CamelCrusherHost.app` and installs into `~/Applications/CamelCrusher Host.app`, while the release installer stages the same AU payload into `/Applications/CamelCrusher Host.app`.
- The current no-UI `VST3` bundle also exposes `CamelCrusher` with the same 12 public parameters and 20 factory presets, and the repo now validates both in-process state/render behavior and bundle-level factory loading through Steinberg hosting helpers.
- The current `VST3` path now also exposes a native macOS editor window, so Ableton’s plug-in open button no longer dead-ends on a `createView == nullptr` wrapper.
- The dev `VST3` install helper currently copies the built bundle into `~/Library/Audio/Plug-Ins/VST3/CamelCrusher.vst3`, which is the path Ableton should rescan.
- `VST2` is now treated as a first-class source and release format alongside `AU` and `VST3`.
- The repo now vendors the required `VST2` SDK slice under `third_party/vst2sdk`, so the default macOS build compiles all three formats from the repo itself.
- The guarded `tools/install_dev_vst2.sh` helper refuses to overwrite a pre-existing non-compatibility `CamelCrusher.vst` by default.
- The reversible `tools/swap_installed_vst2.sh` helper is the intended path for real old-set tests, because it records a backup before replacing the scanned bundle path.
- The new release builder produces a customizable macOS installer with AU, VST2, and VST3 choices plus a matching uninstall helper inside `release/<product-version>/`.
- The `VST2` wrapper now has a real preview editor window, so hosts have an honest `effFlagsHasEditor` path again, and the current editor now sits on top of a bundled faceplate resource derived from the rebuilt CamelCrusher artwork instead of a purely code-painted panel.
- The current `VST2` surface now follows the original 2x2 control geometry more closely and uses sprite-based `On` / `Phat` buttons with borderless patch selection controls on top of the rebuilt background art.
- The `VST2` aesthetic pass is intentionally focused on the original 12 visible controls. The 5 reserved compatibility slots remain preserved in state, but they are no longer foregrounded in the main UI.
- `modern_state_tool.py` now prefers the standard AU preset directory, but if `~/Library/Audio/Presets/Rivet/CamelCrusher` is not writable on the current machine it falls back to `build/au-presets/` so manual host testing still has a real `.aupreset` artifact to load.
- The AU preset path is now hardened against unknown negative-number user preset requests, which fixed a real out-of-process extension crash during `auvaltool` validation.

Docs:

- `docs/ENGINEERING_GUIDE.md` for the implementation plan
- `docs/CURRENT_HANDOFF.md` for the active objective and next steps
- `docs/LEARNINGS.md` for durable compatibility findings
- `docs/OPEN_SOURCE_NOTES.md` for public-repo constraints and fixture hygiene
- `docs/RELEASE_GUIDE.md` for installer, signing, and staging details
- `docs/GITHUB_UPLOAD_GUIDE.md` for the recommended open-source export and upload path
