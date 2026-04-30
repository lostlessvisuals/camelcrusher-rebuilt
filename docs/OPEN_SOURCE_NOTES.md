# Open Source Notes

## Goal

Keep the shared core and all three wrapper paths publishable from one repo.

## Current Public-Safe Direction

- Keep `src/core/` and `include/camelcrusher_recalled/` free of wrapper-specific SDK assumptions.
- Keep the default public seam centered on `camelcrusher_runtime_bridge` and `tools/modern_state_tool.py`, so wrappers and tooling can reuse the same import/state contract.
- Treat the legacy `VST2` same-identity target as a first-class product and source format.
- Keep the repo-local `src/compat_vst2/sdk/pluginterfaces/vst2.x` compatibility headers scoped to the `VST2` path.
- Keep trademark-sensitive names, legacy UID constants, and compatibility-specific wrapper behavior centralized instead of scattering them through the repo.
- Keep the public export very close to the local working tree. The current plan is to strip generated artifacts and the archived original vendor distribution, not to fork the repo into a visually different public variant.

## Fixture Hygiene

- Do not commit extracted fixtures that expose private local source paths.
- Use `tools/extract_camelcrusher_fixture.py --source-label ...` when generating repo fixture JSONs intended for publication.
- Current checked-in fixture JSONs use public-safe labels instead of raw Dropbox paths.

## Wrapper Choice Constraints

- Prefer a wrapper/toolchain path that can live in a public repo without requiring proprietary runtime code in the shared core.
- The first AU scaffold now builds only against Apple system SDK frameworks, which is a good default public baseline.
- The current VST3 scaffold vendors the official Steinberg SDK under `third_party/vst3sdk` and stays no-UI, which keeps the public wrapper story inspectable and relatively thin.
- The assembled AU bundle metadata is templated from repo files rather than hidden in an Xcode project, which keeps the public build inspectable and portable.
- The repo now also assembles a tiny containing app for AU discovery experiments, which keeps the public AUv3 packaging story aligned with Apple’s app-extension model instead of depending on a local Xcode project.
- The repo now vendors the `VST2` SDK slice it depends on under `third_party/vst2sdk`, so the default macOS build can compile `VST2` without a machine-local Downloads path.
- Real local AU registration on this machine was validated with a real Apple Development signing identity and sandboxed app-extension packaging. Keep that as a development/deployment note, not a shared-core dependency.
- Keep AU/VST3 scaffolding separable from the `VST2` implementation details even though all three now build from the same repo.
- The public export should preserve the vendored `VST2` SDK slice and `VST2` wrapper code so contributors can compile the same format set locally.

## What Still Needs Deliberate Review

- License choice for the public repo.
- Whether the package name, product name, and compatibility target naming need trademark review before publication.
- Whether any real fixture artifacts need further sanitization beyond source-path redaction.
- Whether the current bundled faceplate/artwork can ship publicly as-is, or needs a later art-source replacement.

## Current Export Path

- Use `tools/prepare_github_export.sh` to create a repo-shaped export tree under `open-source-export/camelcrusher-rebuilt`.
- The export intentionally excludes:
  - generated build products
  - release bundles
  - Python caches and macOS Finder clutter
  - `reference/original-distribution/`, which contains the original vendor distribution used for reverse-engineering
- The export intentionally keeps the current source layout, wrapper code, shared UI assets, and checked-in fixtures so the public tree stays close to local development.
