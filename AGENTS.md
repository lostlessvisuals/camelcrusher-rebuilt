# CamelCrusher Rebuilt

Compatibility-focused native remake workspace for keeping legacy CamelCrusher sessions alive while building modern AU/VST2/VST3 targets.

## Workspace Role
- This is the implementation workspace for the CamelCrusher compatibility/remake project.
- Inherit shared preflight plus the package-workspace skill inventory from `/Volumes/T7/Dropbox/Codex/packages/AGENTS.md` unless this project deliberately narrows it.

## Constraints
- Primary goal: preserve old Ableton Live sets that reference the legacy `VST2` CamelCrusher on macOS.
- Treat the legacy `VST2` identity as compatibility-sensitive data. Do not casually rename, reorder, or remap legacy parameters once captured.
- Keep the DSP core independent from AU/VST3/VST2 wrapper code.
- Treat the legacy `VST2` identity, SDK snapshot, and shipping metadata as deliberate compatibility surfaces. Do not let them drift accidentally.
- Do not modify source `.als` files under `/Volumes/T7/Dropbox/Music/RIPPED`.

## How To Work
- Read `docs/CURRENT_HANDOFF.md` before deeper repo docs.
- Read `docs/LEARNINGS.md` before substantial compatibility work.
- Treat inspected Live-set data as ground truth before relying on memory or generic plug-in assumptions.
- Keep package-specific decisions in repo docs, not in this file.
- Prefer reversible inspection tools and fixture extraction over destructive edits.
- Keep the package loadable/testable after each coherent slice.

## Build / Run / Test
- Build: `cmake -S . -B build && cmake --build build`
- Run: `python3 tools/extract_camelcrusher_fixture.py <fixture.als> --pretty -o tests/fixtures/<name>.json`
- Test: `ctest --test-dir build --output-on-failure`

## Coding Conventions
- Use a shared C++ DSP/state core with thin format wrappers.
- Centralize compatibility constants such as legacy UID, parameter order, and chunk parsing.
- Keep compatibility-mode behavior explicit and opt-in at the target level.
