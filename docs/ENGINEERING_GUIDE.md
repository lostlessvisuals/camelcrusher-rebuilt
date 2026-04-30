# Engineering Guide

## Goal

Build the `camelcrusher-recalled` workspace into `CamelCrusher Rebuilt`, a native Apple Silicon remake of CamelCrusher with:

- modern `AU`, `VST2`, and `VST3` targets for native Apple Silicon use
- a same-identity `VST2` path that can reopen old Ableton Live sets without Rosetta
- a shared DSP/state core so all formats behave consistently

The compatibility goal is not merely "sound like CamelCrusher." It is "match enough of CamelCrusher's host identity and saved state model that old sets can reopen with the right settings."

## Assumptions

- This repo now carries a public-source-shaped release path, not only a local lab build.
- The highest-value host is Ableton Live on macOS Apple Silicon.
- The best fixture evidence currently lives in `/Volumes/T7/Dropbox/Music/RIPPED/altare/Projects`.
- Legacy Live sessions may be recoverable from both the explicit float parameter list and the saved binary VST preset buffer.
- `VST2` support is now built from the vendored `third_party/vst2sdk` slice, but it still should not drive the architecture of the whole project.
- `camelcrusher-recalled` is the workspace codename, while `CamelCrusher Rebuilt` is the remake/project name. The current host-facing plug-in identity is back to `CamelCrusher` so integration behavior stays aligned with the original naming surface.

## Scope

- User-visible behavior:
  - A native Apple Silicon AU/VST3 plug-in that reproduces CamelCrusher behavior closely enough for manual migration.
  - A legacy-compatibility path for old Live sets that currently expect the old VST2 CamelCrusher.
  - A compatibility target that, if successful, lets old sets open without manual remapping.
- State/data implications:
  - Preserve the legacy `VST2` identity and 17-parameter surface where old Ableton sets depend on it.
  - Parse and round-trip the legacy preset/state chunk format stored in `.als` files.
  - Provide a migration/import path from legacy chunks into modern AU/VST3 state.
- Persistence implications:
  - Old sessions must restore from host-saved state.
  - New sessions in AU/VST3 must save/load reliably using their native persistence models.
- Platform-specific implications:
  - Modern targets should run natively on Apple Silicon.
  - The AU target helps Logic and other AU-capable hosts, but it does not solve legacy Live VST2 recall by itself.
  - The VST2 compatibility target is stricter and higher risk than AU/VST3 because it must match a legacy host contract exactly.
- Release implications:
  - The shipping macOS installer should carry AU, VST2, and VST3 from the same build tree.

## Non-goals

- Windows support in the first planning pass.
- Exact UI skin cloning in phase 1.
- Perfect DSP parity before state recall works.
- Assuming AU or VST3 can automatically replace a missing VST2 in old Live sets.

## Affected areas

- Package workspace:
  - `README.md`
  - `AGENTS.md`
  - `docs/CURRENT_HANDOFF.md`
  - `docs/LEARNINGS.md`
  - `docs/ENGINEERING_GUIDE.md`
- Planned code areas:
  - `src/core/`
    - DSP blocks
    - parameter model
    - preset/chunk serializer-deserializer
  - `src/compat_vst2/`
    - same-identity legacy compatibility wrapper
    - legacy identity constants such as `CaCr`
  - `src/modern_vst3/`
    - native VST3 wrapper and migration importer
  - `src/modern_au/`
    - native AU wrapper and migration importer
  - `tests/fixtures/`
    - extracted state/chunk fixtures from Altare `.als` files
  - `tests/`
    - chunk parsing tests
    - parameter-map tests
    - migration tests
- Confirmed legacy host evidence from inspected Altare sets:
  - VST2 path: `/Library/Audio/Plug-Ins/VST2/CamelCrusher.vst`
  - Plug-in name: `CamelCrusher`
  - Unique ID: `1130447730` / `CaCr`
  - Parameter count: `17`
  - Program count: `20`
  - Live stores a binary preset buffer plus explicit float parameters
- Sample fixture paths:
  - `/Volumes/T7/Dropbox/Music/RIPPED/altare/Projects/Catalyst - Patreon/Catalyst - Patreon/Catalyst - Patreon.als`
  - `/Volumes/T7/Dropbox/Music/RIPPED/altare/Projects/Bye Rmx - Patreon (als + stems)/Bye Rmx - Patreon (als + stems)/Bye Rmx - Patreon/Bye Rmx - Patreon.als`

## Acceptance criteria

- Phase 1:
  - A native Apple Silicon AU/VST3 plug-in exists with a shared parameter model and a manual preset importer fed from legacy fixture data.
  - Given a captured legacy fixture, the modern target restores the expected settings for the 17 legacy VST2 parameters.
- Phase 2:
  - The project contains repeatable tests that decode at least one real CamelCrusher preset buffer from an Altare `.als` file.
  - The decoded state can be re-encoded without losing the tested values needed for recall.
- Phase 3:
  - A same-identity VST2 target loads natively on Apple Silicon without Rosetta.
  - At least one old Altare Live set opens and resolves a formerly missing CamelCrusher instance automatically.
  - The resolved instance restores the expected settings without manual user intervention.
- Phase 4:
  - Rendered audio from the compatibility target is close enough to the legacy plug-in on reference material that the migration is musically acceptable.

## Risks and edge cases

- `VST2` risk:
  - Steinberg officially discontinued VST2 and says Apple Silicon Mac users can continue to use VST2 under Rosetta 2 in Steinberg hosts, which is exactly the gap this project is trying to escape. This makes `VST2` the hardest and riskiest target, not the baseline. [Steinberg](https://helpcenter.steinberg.de/hc/en-us/articles/4409561018258-VST-2-Discontinued)
- Host identity risk:
  - Hosts do not match only on visible plug-in name. The compatibility target likely needs the old VST2 UID and matching parameter/state behavior.
- State-format risk:
  - Live stores both a float parameter list and a binary preset buffer. The host may prefer one over the other during restore.
- Parameter-surface risk:
  - The legacy Live VST2 data shows 17 parameters, while the installed AU exposes a different surface. Wrapper parity cannot be assumed.
- Coexistence risk:
  - A same-identity compatibility target may conflict with the old plug-in if both are installed together.
- DSP risk:
  - State recall can succeed while the sound is still wrong.
- Verification risk:
  - A prototype can appear to work in isolation yet fail to remap automatically in old sets because the host-matching contract is incomplete.

## Recommended implementation order

1. Freeze the compatibility facts.
   - Build a tiny fixture-extraction tool that reads `.als` files and emits structured CamelCrusher evidence: path, plug-in name, UID, parameter list, program number, and raw preset buffer.
2. Scaffold the repo around a shared core.
   - Create a shared C++ state/DSP library with no wrapper assumptions.
   - Define the legacy 17-parameter compatibility surface as a stable schema.
3. Build the safe modern target first.
   - Implement AU and VST3 targets under the original host-facing `CamelCrusher` name while keeping the remake project itself separate from that plug-in identity.
   - Add a manual importer from extracted legacy fixture state into the shared core.
4. Reverse-engineer the legacy chunk format.
   - Decode the binary preset buffer.
   - Verify how much of the real state comes from the chunk versus the explicit float parameters.
5. Add migration tests and audio references.
   - Save a handful of fixture states.
   - Render legacy reference audio under Rosetta and compare against the new core.
6. Decision gate: same-identity VST2 release target.
   - Only after chunk parsing and parameter parity are working should the project attempt a same-identity VST2 target.
   - If viable, implement the VST2 target with the legacy `CaCr` identity and tested parameter order.
7. Host validation in real sets.
   - Test old Altare projects in native Apple Silicon Live.
   - Confirm whether the compatibility target is discovered and recalled automatically.

## Suggested tests

- Fixture extraction tests:
  - Parse known Altare `.als` files and assert presence of CamelCrusher instances with UID `CaCr`.
- State-schema tests:
  - Assert the 17-parameter legacy VST2 surface remains stable.
  - Verify importer/exporter behavior for each parameter.
- Chunk tests:
  - Decode real legacy preset buffers from fixtures.
  - Re-encode and compare stable portions where appropriate.
- Host persistence tests:
  - Save/load AU state.
  - Save/load VST3 state.
  - Save/load compatibility target state against legacy fixture expectations.
- Audio tests:
  - Compare rendered reference clips between the legacy Rosetta-hosted plug-in and the new implementation.
- Integration tests:
  - Open at least one old Altare Live set that previously referenced CamelCrusher and confirm recall behavior.

## Appendix: Relevant Source Evidence

- Ableton documents that on Apple Silicon, native Live only recognizes VST2/VST3 plug-ins built natively for Apple Silicon, while some Intel AUs can load through compatibility services. This is why AU helps for modern hosts but does not solve legacy VST2 set recall on its own. [Ableton](https://help.ableton.com/hc/en-us/articles/4410323149074-Plug-ins-on-Mac-in-Live-11-1-and-later)
- Apple documents AU preset/state persistence separately from VST persistence, reinforcing that AU is a different host contract from the VST2 identity stored in legacy Live sets. [Apple](https://developer.apple.com/library/archive/technotes/tn2157/_index.html)
- Steinberg documents VST3 persistence as host-saved component/controller state, which is the model the modern VST3 target should use once legacy import exists. [Steinberg](https://steinbergmedia.github.io/vst3_dev_portal/pages/FAQ/Persistence.html)
