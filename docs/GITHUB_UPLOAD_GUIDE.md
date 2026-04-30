# GitHub Upload Guide

## Recommended Path

1. Prepare a clean local export:
   - `tools/prepare_github_export.sh`
2. Review the staged tree at:
   - `open-source-export/camelcrusher-rebuilt`
3. Read `docs/OPEN_SOURCE_NOTES.md` inside that export before publishing.
4. If you are publishing under `Lossless Visuals`, the clean default is:
   - org/account: `Lossless Visuals`
   - repo: `camelcrusher-rebuilt`
5. Initialize git in the export tree or copy it into a fresh git repo.
6. Make the first commit from the export tree, not from the live development workspace with generated build outputs.
7. Push that export repo to GitHub.

## Why This Path

- It keeps the public tree very close to the working local repo.
- It drops generated build output and machine-specific clutter.
- It intentionally excludes `reference/original-distribution/`, which contains the archived original CamelCrusher distribution and should not be republished blindly.

## Suggested First GitHub Structure

- Source repo: the export tree created above
- Release assets: attach the staged installer from `tools/build_macos_release.sh`
- README hero image: `docs/assets/camelcrusher-ui-preview.png`
- Docs to keep visible:
  - `README.md`
  - `docs/PUBLIC_COPY.md`
  - `docs/OPEN_SOURCE_NOTES.md`
  - `docs/RELEASE_GUIDE.md`

## Still Needs Human Review Before Publish

- License selection
- Trademark/name risk around `CamelCrusher`
- Whether the current UI art should remain exactly as-is in the public repo
- Whether the current vendored `third_party/vst2sdk` slice is the exact public SDK snapshot you want to ship to contributors
