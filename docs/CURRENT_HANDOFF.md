# Current Handoff

## Active Objective
- Productize the now-working `AU`, `VST2`, and `VST3` builds into a release-ready macOS install story without changing the public export into a visibly different repo from local development.

## Current Repo Reality
- The package now contains a minimal implementation scaffold:
  - `CMakeLists.txt` for a small shared-core build and test baseline
  - `include/camelcrusher_recalled/LegacySchema.h` and `src/core/LegacySchema.cpp`
  - `include/camelcrusher_recalled/LegacyParameters.h` and `src/core/LegacyParameters.cpp`
  - `include/camelcrusher_recalled/LegacyState.h` and `src/core/LegacyState.cpp`
  - `include/camelcrusher_recalled/LegacyChunk.h` and `src/core/LegacyChunk.cpp`
  - `include/camelcrusher_recalled/LegacyPresetBank.h` and `src/core/LegacyPresetBank.cpp`
  - `include/camelcrusher_recalled/LegacyImport.h` and `src/core/LegacyImport.cpp`
  - `include/camelcrusher_recalled/ModernPluginModel.h` and `src/core/ModernPluginModel.cpp`
  - `include/camelcrusher_recalled/ModernProcessor.h` and `src/core/ModernProcessor.cpp`
  - `include/camelcrusher_recalled/ModernRuntime.h` and `src/core/ModernRuntime.cpp`
  - `include/camelcrusher_recalled/ModernRuntimeBridge.h` and `src/core/ModernRuntimeBridge.cpp`
  - `include/camelcrusher_recalled/CamelCrusherRecalledAudioUnit.h` and `src/modern_au/CamelCrusherRecalledAudioUnit.mm`
  - `src/modern_vst3/CamelCrusherVst3IDs.h`, `CamelCrusherVst3Version.h`, `CamelCrusherVst3State.h`, `CamelCrusherVst3State.cpp`, `CamelCrusherVst3Processor.h`, `CamelCrusherVst3Processor.cpp`, `CamelCrusherVst3Controller.h`, `CamelCrusherVst3Controller.cpp`, and `CamelCrusherVst3Entry.cpp`
  - `src/compat_vst2/CamelCrusherVst2Effect.h`, `src/compat_vst2/CamelCrusherVst2Effect.cpp`, `src/compat_vst2/CamelCrusherVst2Editor.h`, `src/compat_vst2/CamelCrusherVst2Editor.mm`, `src/compat_vst2/CamelCrusherVst2Entry.cpp`, and `src/compat_vst2/sdk/pluginterfaces/vst2.x/aeffect.h`
  - `src/modern_au/CamelCrusherRecalledExtensionMain.mm`
  - `src/modern_au/CamelCrusherRecalledAudioUnitFactory.m`
  - `src/modern_au/CamelCrusherRecalledHostMain.mm`
  - `src/modern_au/Info.plist.in`
  - `src/modern_au/HostInfo.plist.in`
  - vendored Steinberg SDK under `third_party/vst3sdk`
  - `tools/extract_camelcrusher_fixture.py` for Ableton `.als` extraction
  - `tools/install_dev_au_host_app.sh` for local app-extension installation and registration checks
  - `tools/install_dev_vst3.sh` for local `VST3` installation
  - `tools/install_dev_vst2.sh` for guarded `VST2` bundle installation
  - `tools/swap_installed_vst2.sh` for reversible “back up original / activate current repo-built bundle / restore original” local testing
  - `tools/modern_au_component_probe.mm` for low-noise local discovery validation
  - `tools/modern_au_view_probe.mm` for a tiny AppKit host that requests the real registered AU custom view
  - `tools/modern_state_tool.py` for bridge-backed fixture import, state inspection, and `.aupreset` export
  - `tests/legacy_schema_smoke.cpp`, `tests/legacy_chunk_smoke.cpp`, `tests/legacy_model_smoke.cpp`, `tests/modern_plugin_model_smoke.cpp`, `tests/modern_processor_smoke.cpp`, `tests/modern_runtime_smoke.cpp`, `tests/modern_runtime_bridge_smoke.cpp`, `tests/vst2_compat_smoke.cpp`, `tests/vst2_bundle_smoke.cpp`, `tests/vst2_editor_smoke.mm`, `tests/modern_vst3_smoke.cpp`, `tests/modern_vst3_bundle_smoke.cpp`, `tests/modern_au_smoke.mm`, `tests/modern_au_bundle_smoke.mm`, `tests/modern_au_container_smoke.mm`, `tests/test_fixture_extractor.py`, and `tests/test_modern_state_tool.py`
  - extracted local fixture artifacts under `tests/fixtures/`
- Public export guidance now lives in `docs/OPEN_SOURCE_NOTES.md`, including fixture labeling and the vendored `VST2` build story.
- Release metadata is now centralized in `cmake/CamelCrusherMetadata.cmake`, and the local install helpers plus release scripts read from that shared source instead of carrying separate hard-coded product names and bundle IDs.
- The source tree no longer carries the older `private_vst2` naming. `VST2` is now treated as a first-class source/build/export format, and the reversible dev helper is `tools/swap_installed_vst2.sh`.
- The repo now has a first real macOS release packager in `tools/build_macos_release.sh`, which stages:
  - `/Applications/CamelCrusher Host.app` for the AU path
  - `/Library/Audio/Plug-Ins/VST3/CamelCrusher.vst3`
  - `/Library/Audio/Plug-Ins/VST/CamelCrusher.vst`
  - a customizable installer with per-format choices
  - an `Uninstall CamelCrusher.command` helper
- The repo now also has a GitHub-export helper in `tools/prepare_github_export.sh` that keeps the public export close to the local source tree while excluding generated outputs and the archived original vendor distribution.
- The GitHub export is no longer just staged locally: `open-source-export/camelcrusher-rebuilt` was initialized as a clean repo and published at `https://github.com/lostlessvisuals/camelcrusher-rebuilt`.
  - The published repo currently includes the UI preview image at `docs/assets/camelcrusher-ui-preview.png` in the top-level README.
  - The public repo description is: `Native Apple Silicon remake of CamelCrusher for macOS, with AU, VST2, and VST3 builds, plus a VST2 compatibility path for reopening older projects.`
- The public repo landing page was tightened after publish:
  - `README.md` is now a short high-level landing page instead of an engineering dump.
  - `docs/assets/camelcrusher-ui-preview.png` is now generated from the live shared macOS UI surface via `tools/render_mac_editor_preview.mm`, so the published screenshot includes the corrected button layout and visible `Randomize` button.
  - The repo now exposes a direct downloadable installer at `downloads/CamelCrusher-Rebuilt-0.7.1.pkg`.
  - The first public release page is still the older `v0.7.0` page; a cleaner rerelease path is to tag and ship `v0.7.1` so the installer filename stops drifting under the old version.
  - The public repo no longer includes the `reference/` tree; `tools/prepare_github_export.sh` now excludes it entirely, and `main` removed the previously published reference assets at commit `e794d3c` (`Remove reference folder from public export`).
- The project is grounded in inspected Ableton `.als` fixtures from `/Volumes/T7/Dropbox/Music/RIPPED/altare/Projects`.
- Verified legacy Live references use `VST2` CamelCrusher at `/Library/Audio/Plug-Ins/VST2/CamelCrusher.vst`.
- Verified legacy `VST2` identity in inspected sets:
  - `UniqueId = 1130447730` which decodes to `CaCr`
  - `ParameterCount = 17`
  - `ProgramCount = 20`
- Verified installed macOS `AU` exists separately and exposes a different parameter surface than the legacy Live `VST2` data, so AU is useful for modern targets but not a substitute for VST2 session recall.
- Verified a same-name, same-UID `VST2` bundle now builds at `build/CamelCrusher.vst` and exports both `VSTPluginMain` and `main_macho`.
- Verified the `VST2` wrapper currently reports:
  - `uniqueID = CaCr`
  - `17` parameters
  - `20` programs
  - `cEffect.version = 1`
  - `getVstVersion() = 2400`
  - `AEffect.flags = 57`, matching the original legacy binary
  - `getPlugCategory() = 0`, matching the original legacy binary
  - chunk-based state via `effFlagsProgramChunks`
- Verified the `VST2` path now has a real native macOS preview editor, so `effFlagsHasEditor` is backed by an actual openable host window.
- Verified the first Catalyst old-set check now lands exactly on the expected saved state for fixture instance `0`: program `3` / `British Clean`, with `17/17` legacy parameters matching at `max_abs_delta = 0.0`.
- Verified the `VST2` editor is no longer a generic debug panel. The current pass presents a CamelCrusher-inspired four-module metal layout with custom knobs, lamp toggles, and de-emphasized debug utilities.
- Verified the visual system is no longer `VST2`-only. `VST3` now uses the same CamelCrusher-style AppKit surface, and the installed out-of-process `AU` now advertises `providesUserInterface = 1` and returns an `AUAudioUnitRemoteViewController`.
- Verified the preset-strip typography pass is now shared across the macOS surfaces: `VST2`, `VST3`, and the in-process `AU` view all use the smaller preset text sizing.
- Verified the `VST2` live editor now includes the original-style `Randomize` button instead of only exposing it in the probe or `VST3`.
- Verified the corrected control-placement model is shared: knobs, `On` buttons, the `Phat` button, and the `Randomize` button all align from center-anchored coordinates against the skin geometry.
- Verified the sprite-frame mapping is now corrected for the original skin assets: `On` and `Phat` use the top frame for the active/blue state, while `Randomize` uses the top frame as its idle gold state and the lower frame as the pressed accent.
- Verified the shared processor now matches two more important CamelCrusher behaviors:
  - `Master On` off now bypasses the whole plug-in instead of only disabling the output controls
  - the compressor `P` control now behaves like a switch-style `Phat` mode with a more aggressive voice, not like a generic continuous “mode” blend
- Verified the recent “left-lane mono” suspicion was not reproduced in the shared DSP core. The real hardening work landed in the wrappers and tests:
  - `ModernProcessor`, `VST2`, `VST3`, and `AU` now all have asymmetric stereo regression coverage
  - the `VST3` processor now safely handles mono-input/stereo-output host layouts instead of assuming channel 2 always exists
  - the `AU` render block now handles both deinterleaved stereo buffers and one-buffer interleaved stereo buffers
  - the first failing AU regression was a false alarm caused by Objective-C block capture semantics in the test, not by the runtime path itself
- A user-side manual check has now confirmed that the current AU opens in Ableton Live on this machine.
- The remake/project name is still `CamelCrusher Rebuilt`, while the current host-facing plug-in name has been moved back to `CamelCrusher`. The workspace path and bundle identifiers still retain `camelcrusher-recalled` for continuity during active development.
- Ableton rename lesson from April 29, 2026: Live was sensitive to AUv3 host-facing rename changes on this machine. After moving away from `CamelCrusher Rebuilt` again, the safe path is to keep the AU component-facing identity vendor-prefixed and bump the bundle/component version each time that host-facing identity changes.
- AU custom-view lesson from April 29, 2026: the working install shape on this machine is a single embedded `com.apple.AudioUnit-UI` extension, not a split installed `AudioUnit` plus companion `AudioUnit-UI` pair. Once the host app embedded only the UI-bearing extension, stopped swallowing `beginRequestWithExtensionContext:` in the view controller, and centered the editor on its natural `345x373` geometry, the registered out-of-process proxy flipped to `providesUserInterface = 1` and returned a remote AU view controller cleanly.

## Decisions That Matter Right Now
- Decision: package codename is `camelcrusher-recalled`.
  Why it matters: this is the stable workspace for future engineering and handoff.
- Decision: keep the remake/project name separate from the actual plug-in name.
  Why it matters: the project can still be discussed as `CamelCrusher Rebuilt`, while the installed AU/VST name stays `CamelCrusher` to minimize host-integration surprises.
- Decision: keep the AUv3 component-facing name vendor-prefixed as `Camel Audio: CamelCrusher`.
  Why it matters: on this machine Ableton resumed indexing the AU reliably only after the component-facing name kept the vendor prefix and the AU version was bumped.
- Decision: install the AU as a single `com.apple.AudioUnit-UI` extension inside the host app.
  Why it matters: that is the packaging shape that now produces a real out-of-process custom view on this machine instead of a generic host surface.
- Decision: pursue a dual-track design.
  Why it matters: modern native targets and legacy drop-in compatibility have different constraints and should not be conflated.
- Decision: use Altare `.als` projects as fixture truth.
  Why it matters: host persistence details are more important than generic plug-in folklore here.
- Decision: keep the legacy 17-name surface centralized in code and validate it against extracted fixtures.
  Why it matters: parameter identity/order drift is a compatibility bug, not a cosmetic detail.
- Decision: vendor the `VST2` SDK slice and build `VST2` by default on macOS.
  Why it matters: the repo, installer, and eventual public source export should all compile the same three plug-in formats without a machine-local Downloads dependency.
- Decision: model the future modern host surface as 12 public parameters plus 5 preserved hidden compatibility slots.
  Why it matters: modern wrappers can present a cleaner public surface without losing legacy state needed for compatibility and migration.
- Decision: keep the current processor explicitly framework-neutral and clearly non-parity.
  Why it matters: the repo can keep moving toward a usable open-source core without pretending the current DSP is a finished CamelCrusher recreation.
- Decision: use a shared-library bridge plus tooling as the first thin custom wrapper seam.
  Why it matters: future AU/VST3 shims can stay small while the open-source-safe core keeps owning legacy import, parameter layout, state persistence, and processing.
- Decision: the first real wrapper target is a macOS `AUAudioUnit` subclass built against the system Apple SDK, not a third-party wrapper framework.
  Why it matters: the repo can prove a host-facing wrapper path now, keep the public build dependency-light, and postpone VST3 SDK decisions until after the AU bundle is real.
- Decision: the first `VST3` target is a thin no-UI wrapper built directly against the official Steinberg SDK vendored into `third_party/vst3sdk`.
  Why it matters: the repo now has a second modern host format without adding a larger wrapper framework or contaminating the shared core with wrapper-specific legacy SDK assumptions.
- Decision: spend the next core passes on sound behavior inside the shared processor before chasing more wrapper proliferation.
  Why it matters: once the AU loads in Ableton, the biggest user-visible gap is no longer discovery, it is whether the effect feels plausibly musical while the import and compatibility work matures.
- Decision: package the first AU artifact as a no-UI extension-style bundle with an `AUAudioUnitFactory` principal class and `AudioComponents` plist metadata.
  Why it matters: it matches Apple’s documented AUv3 extension model closely enough to test bundle loading and metadata now, while keeping actual host-install/discovery work as the next separate slice.
- Decision: ship the AU extension inside a tiny containing macOS app for real discovery checks, instead of treating a loose `.appex` as installable.
  Why it matters: Apple’s AUv3 model is an app-extension model, and the containing app is the correct dev-install shape to validate before considering host-specific issues.
- Decision: embed the runtime bridge dylib inside the `.appex`, and use a real Apple Development signing identity for local registration checks when available.
  Why it matters: the extension must launch outside the build directory, and on this machine real PlugInKit discovery/instantiation only became reliable once the installed app-extension package was development-signed and self-contained.

## Verification State
- Passed:
  - Located multiple Altare Live sets containing CamelCrusher VST2 references.
  - Built a shared-core scaffold and CMake test baseline.
  - Added a fixture extractor that emits structured CamelCrusher JSON from real `.als` files.
  - Generated fixture artifacts for the confirmed Catalyst and Bye Rmx sets.
  - Verified the real fixtures match the frozen 17 named parameters once Ableton's blank placeholder slots are ignored.
  - Decoded the real 1767-byte CamelCrusher preset buffer into 20 program records.
  - Verified the saved `ProgramNumber` points at the same 17 values stored in the explicit Ableton parameter list.
  - Verified the decoded chunk can be re-encoded and decoded again without losing program names or parameter values.
  - Added wrapper-agnostic parameter metadata, named legacy state containers, preset-bank helpers, and a richer importer that preserves selected-preset context plus mismatch diagnostics.
  - Added a wrapper-neutral modern plugin model that maps imported legacy state into a public 12-parameter host view while preserving the 5 hidden compatibility slots.
  - Added a wrapper-neutral stereo processor that derives controls from the modern plugin state and applies placeholder distortion, filter, compression, wet/dry mix, and output gain.
  - Added a wrapper-neutral runtime layer that owns current state, preset-selection diagnostics, save/load of a serialized modern state blob, and end-to-end processing.
  - Added `camelcrusher_runtime_bridge` as a public-safe shared-library seam around the runtime for thin custom wrappers and non-host tooling.
  - Added `tools/modern_state_tool.py`, which can turn checked-in fixture JSON into a serialized modern state blob and inspect that blob through the bridge.
  - Added a thin `VST3` processor/controller/factory wrapper around the shared runtime, with the same 12 public parameters and a 21-slot program surface made of `Current State` plus the 20 decoded legacy factory presets.
  - Added a native macOS preview editor for the `VST3` wrapper, so the host now has a real `createView(ViewType::kEditor)` surface instead of a silent no-window path.
  - Added `modern_vst3_smoke`, which instantiates the concrete VST3 processor/controller classes in process, loads wrapped state, checks program names and parameter mapping, and renders audio through the shared processor.
- Added `modern_vst3_editor_smoke`, which attaches the editor view to a dummy `NSView` parent and verifies the window path does not regress back to `nullptr`.
- Added `src/shared_ui/CamelCrusherMacEditorSurface.h` and `src/shared_ui/CamelCrusherMacEditorSurface.mm`, which now hold the shared macOS CamelCrusher chrome used by `VST3` and `AU`.
- Added `modern_vst3_bundle_smoke`, which loads the built `.vst3` bundle through Steinberg hosting helpers, validates bundle structure, enumerates the factory classes, and instantiates both processor and controller from the built package.
  - Added `tools/install_dev_vst3.sh`, which installs the built `CamelCrusher.vst3` into `~/Library/Audio/Plug-Ins/VST3/` for Ableton rescans on this machine.
  - Added a same-identity `VST2` target that reuses the shared core but restores the legacy host-facing surface directly: name `CamelCrusher`, UID `CaCr`, `17` parameters, `20` programs, `2400` VST version, and legacy chunk save/load.
  - Added a small local `pluginterfaces/vst2.x` compatibility layer because the available `VST2_SDK/public.sdk/source/vst2.x` checkout on this machine contained `audioeffect*.h/.cpp` but not the old `aeffect*.h` interface headers.
  - Added `vst2_compat_smoke`, which instantiates the wrapper in process, verifies the legacy identity surface, and checks full-bank plus single-program chunk round-trips.
  - Added `vst2_bundle_smoke`, which dlopens the built `CamelCrusher.vst` bundle, resolves `VSTPluginMain`, instantiates the effect through the exported entrypoint, and validates the packaged identity.
  - Added `CamelCrusherVst2Editor` plus `vst2_editor_smoke`, so the wrapper now mirrors the original `HasEditor` flag honestly and gives Ableton a real native macOS window to open.
  - Probed the original Intel-only CamelCrusher binary under Rosetta and used that raw `AEffect` ground truth to align the wrapper to `flags = 57` and `plugCategory = 0`.
  - Added a `Copy State JSON` action to the VST2 preview editor plus `modern_state_tool.py summarize-fixture` / `compare-fixture-summary`, so Live-loaded state can be compared directly against checked-in fixture expectations instead of eyeballing the controls.
  - Rebuilt the `VST2` editor around a CamelCrusher-inspired visual system: bronze/charcoal metal paneling, four module bays, user-facing knob labels (`Mech`, `Tube`, `Cutoff`, `Resonance`, `Amount`, `Phat`, `Mix`, `Volume`), custom knobs, and lamp toggles.
  - Moved the 5 reserved compatibility slots out of the main visual surface so the visible editor now focuses on the original 12 user-facing controls instead of exposing internal compatibility state as first-class UI.
  - Added `tools/install_dev_vst2.sh`, which safely installs the built bundle only when it is targeting a repo-built compatibility-marked bundle path.
  - Added `tools/swap_installed_vst2.sh`, which can back up a pre-existing `CamelCrusher.vst`, activate the current repo-built bundle for Live testing, and restore the previous bundle later.
  - Verified the packaged `VST2` bundle is a real macOS `.vst` bundle with `BNDLstCA` `PkgInfo`, bundle ID `com.camelaudio.vst.CamelCrusher`, and an arm64 Mach-O bundle binary.
  - Verified the repo now builds a no-UI `VST3` bundle at `build/VST3/Debug/CamelCrusher.vst3`.
  - Verified the dev `VST3` install helper copies the current build into `~/Library/Audio/Plug-Ins/VST3/CamelCrusher.vst3`.
- Added `CamelCrusherRecalledAudioUnit`, an in-process macOS `AUAudioUnit` scaffold with a runtime-backed parameter tree, `fullState` persistence via the serialized modern state blob, preset exposure from imported legacy banks, and offline render plumbing.
- Added `CamelCrusherRecalledViewController`, so the AU now answers `requestViewControllerWithCompletionHandler:` with a custom CamelCrusher-style `NSViewController`.
- Added `modern_au_smoke`, which instantiates the AU wrapper in process, loads imported preset-bank state, exercises preset switching, and renders audio through the shared processor.
  - Added `CamelCrusherRecalledAudioUnitFactory`, `Info.plist` templating, and AU bundle assembly.
  - Added `modern_au_bundle_smoke`, which now validates the active UI-bearing AU bundle shape rather than the older split-install experiment.
  - Added `camelcrusher_modern_au_container_app`, which now embeds only the `AudioUnit-UI` extension inside `build/CamelCrusherHost.app`.
  - Added `modern_au_container_smoke`, which validates the containing app metadata and the single embedded AU UI extension.
  - Added `modern_au_component_probe`, `modern_au_component_instantiate`, `modern_au_view_probe`, and `tools/install_dev_au_host_app.sh` so local developer installs can check discovery, out-of-process instantiation, and the real custom-view handoff without relying on noisy `auval -a` output.
- Verified the dev containing app can be installed into `~/Applications/CamelCrusher Host.app`, surfaced by `pluginkit`, found by `AudioComponentFindNext`, and instantiated through `AUAudioUnit instantiateWithComponentDescription:` on this machine once the app is launched once after install.
  - Verified the registered out-of-process AU now reports `providesUserInterface = 1`, returns `AUAudioUnitRemoteViewController`, and captures a real custom-view path through `modern_au_view_probe`.
  - Verified `auvaltool -v aufx CcrR CmAu` now succeeds end to end: class info, factory presets, parameter retention, format negotiation, render tests, and parameter scheduling all pass locally.
  - Added `.aupreset` export to `tools/modern_state_tool.py`, so checked-in fixture JSON can now become a host-loadable AU preset artifact without any extra machine-local SDK code.
  - Added in-process user-preset coverage to `CamelCrusherRecalledAudioUnit`, including normalized AU preset dictionaries, explicit user-preset directory handling, and negative-number preset round-trips in `modern_au_smoke`.
  - Generated a real manual-load preset artifact at `build/au-presets/Catalyst British Clean.aupreset` from the Catalyst fixture because the standard user preset folder is not writable on this machine.
  - Seeded the default runtime with the original 20-program CamelCrusher factory bank from checked-in fixture data, so fresh AU instances now expose the legacy preset list without needing an imported state blob first.
  - Verified the registered AU now surfaces to the host as `CamelCrusher`.
  - Verified the rename-regression fix inside Ableton: after reinstall plus rescan, Live now surfaces `CamelCrusher` again in the Plug-Ins browser on this machine.
  - Verified the generic out-of-process AU proxy reports 20 factory presets, from `Annihilate` through `Turn it to 11`.
  - Fixed a packaging dependency bug where the containing app could embed a stale `.appex` even after the AU bundle metadata changed; `modern_au_container_smoke` now guards against that drift.
  - Reworked the shared processor into a more deliberate non-parity signal path: richer mechanical/tube-style distortion shaping, a proper resonant lowpass core, stereo-linked compression, and a more sensible master output mapping.
  - Confirmed the updated processor still passes the focused runtime and AU smoke coverage after the DSP rewrite.
  - Hardened the AU negative user-preset path so unknown preset requests fail cleanly instead of crashing the out-of-process extension during validation.
  - Added public-safe fixture source labels so the checked-in JSON artifacts do not expose local Dropbox paths.
  - Fixed a repo-wide AU packaging race where `cmake --build build -j` could try to rebuild the AU bundle stamp twice; the host-container packaging path now depends on the AU bundle utility target instead of racing the stamp file directly.
- `ctest --test-dir build --output-on-failure` passes locally with `18/18`.
- `tools/build_macos_release.sh` now passes end to end again after the `VST2` naming cleanup, regenerating `release/CamelCrusher-Rebuilt-0.7.1-macOS/Install CamelCrusher.pkg` with AU/VST2/VST3 choices and refreshed staged payloads.
- `tools/prepare_github_export.sh` now regenerates `open-source-export/camelcrusher-rebuilt` without any remaining `private_vst2` source/docs references.
- The GitHub repo `lostlessvisuals/camelcrusher-rebuilt` is now live and currently matches the exported tree head at commit `421373e` (`Vendor VST3 SDK contents`).
- The published repo has moved beyond the initial publish:
  - `main` now includes the shorter README, regenerated screenshot, preview-render helper, and direct installer download at commit `c890b12` (`Add direct installer download`).
  - The public release page `v0.7.0` exists and points people at the direct installer URL even though the release assets themselves still only show GitHub’s auto-generated source archives.
  - On May 10, 2026, the repo download drift was corrected and then rolled into a real `0.7.1` rerelease so the public raw GitHub URL can point at a correctly versioned installer instead of silently replacing the old `0.7.0` bits.
- A fresh scratch build at `/tmp/camelcrusher-recalled-fresh-build` now proves the default repo configure path uses `third_party/vst2sdk` instead of a machine-local Downloads checkout, and that scratch tree also passes `18/18` tests.
- Installer validation now has a clearer status:
  - `installer -showChoicesXML -pkg ... -target /` reports the expected AU/VST2/VST3 selectable choices
  - a real GUI installer run on this machine did install `VST2` and `VST3` into `/Library/Audio/Plug-Ins/...`
  - that same first GUI run did not actually reinstall the AU host app, because PackageKit treated a backed-up `CamelCrusher Host.app` in `~/Library/Application Support/CamelCrusherRebuild/...` as a relocated copy of the same bundle and skipped the AU component
  - `tools/build_macos_release.sh` now generates an explicit AU component property list with relocatable/version-check/strict-identifier behavior disabled, so the rebuilt installer should stop skipping the AU when same-ID backups exist outside `/Applications`
  - a second installer verification pass found that the packaged AU app on disk had lost its sandbox entitlements after install; the release builder was re-signing the already-signed AU app bundle with `codesign --deep`, which stripped the entitlements that PlugInKit needs
  - `tools/build_macos_release.sh` no longer re-signs the staged AU app after the build step, so the release-staged `/Applications/CamelCrusher Host.app` now preserves the original `com.apple.security.app-sandbox` entitlement on both the host app and the embedded `CamelCrusherAUUI.appex`
  - a later Live restore regression on this machine turned out to be `VST2` path-sensitive, not DSP-sensitive: older successful restores used `/Users/porter/Library/Audio/Plug-Ins/VST/CamelCrusher.vst`, while the product installer only left `/Library/Audio/Plug-Ins/VST/CamelCrusher.vst`
  - the release builder now adds a `VST2` postinstall script that creates compatibility aliases for the current console user at `~/Library/Audio/Plug-Ins/VST/CamelCrusher.vst` and `~/Library/Audio/Plug-Ins/VST2/CamelCrusher.vst`, plus a local-system alias at `/Library/Audio/Plug-Ins/VST2/CamelCrusher.vst`
  - `installer -pkg ... -target /` is still blocked from this Codex session because macOS requires root for the real system install step
  - expanded installer BOMs still contain AppleDouble `._*` paths; moving the staging area outside Dropbox fixed the PackageKit reinstall bug, but macOS still leaves `com.apple.provenance` on copied payload files and `pkgbuild` encodes that as receipt noise even when no literal `._*` files exist in the repo or build outputs
- Still needed:
  - Extract and catalog more chunk/parameter fixtures from additional sets.
  - Validate the exact-recall path across the remaining CamelCrusher instances in `Catalyst` and at least one `Bye Rmx` instance, not just the first confirmed Catalyst device.
  - Push the editor closer to the original CamelCrusher finish: title treatment, knob cap details, module engraving, and any AU/VST3-specific fit issues that show up in real hosts.
  - Render old-vs-new audio references once a prototype exists.

## Next Steps
1. Keep shifting the main implementation effort toward deeper DSP feel and audio-reference comparisons.
2. Keep expanding real old-set coverage beyond the first confirmed Catalyst instance so `VST2` recall confidence stops depending on a single project.
3. Decide whether the repo-backed installer download is sufficient for now or whether a future pass should upload the `.pkg` as a first-class GitHub Release asset instead of linking to `downloads/`.
4. Decide whether the current public GitHub state should stay unsigned/dev-friendly or whether a signed/notarized public release is worth the extra macOS distribution work.

## Risks Or Blockers
- Public VST2 support still needs wider install and host validation beyond this machine.
- Old Live session recall may depend on the binary chunk format, not only the exposed parameter list.
- The `VST2` bundle now clears local identity/chunk smokes, but more old Live sets still need coverage.
- The current `VST2` path now has a much stronger first aesthetic pass, but it is still not a pixel-faithful CamelCrusher clone yet.
- The shared macOS CamelCrusher surface is now in `VST3`, and the AU custom-view handoff is working locally, but Ableton still needs a fresh visual check after the UI-extension packaging change.
- The new AU wrapper now has a working local install/discovery path on this machine, and the user has confirmed it opens in Ableton, but preset-browser and imported-preset behavior inside Live still need more coverage.
- The new `VST3` wrapper is now built, smoke-tested, installed locally, and has a native preview editor, but it still needs a fresh real Ableton open-window validation on this machine after reinstall.
- The local install workflow currently depends on launching the containing app once after install so PlugInKit exposes the AU consistently on this machine.
- The rebuilt installer should stop skipping the AU host app when a same-bundle-ID backup exists elsewhere, but that still needs one fresh post-fix GUI install confirmation.
- On this machine, `~/Library/Audio/Presets` is root-owned and not writable for new vendor directories, so the default manual-import artifact path currently falls back to `build/au-presets/` instead of `~/Library/Audio/Presets/Camel Audio/CamelCrusher`.
- Installer BOMs still carry AppleDouble-style `._*` receipt entries because `pkgbuild` is encoding surviving `com.apple.provenance` metadata from copied payload files, even though the actual repo/build trees do not contain literal `._*` files.
- Exact sound parity may take much longer than session-state parity.

## Pointers
- Stable spec / architecture doc: `docs/ENGINEERING_GUIDE.md`
- Open-source readiness / publication constraints: `docs/OPEN_SOURCE_NOTES.md`
- Supporting work log / status doc: none yet
- Related artifact or handoff: `docs/LEARNINGS.md`
