# CamelCrusher Rebuilt

![CamelCrusher UI preview](docs/assets/camelcrusher-ui-preview.png)

`CamelCrusher Rebuilt` is a native Apple Silicon remake of CamelCrusher for modern macOS.

It is built around a simple goal: old sessions should open again. If you have older projects that used CamelCrusher, `CamelCrusher Rebuilt` is meant to let those sessions reopen on Apple Silicon Macs while also giving modern Mac users a native way to keep using the plug-in without depending on the original abandoned setup.

It currently targets:

- `AU`
- `VST2`
- `VST3`

The current host-facing plug-in name is still `CamelCrusher`.

## Download

Download the current macOS installer directly from the repo:

- [CamelCrusher-Rebuilt-0.7.0.pkg](downloads/CamelCrusher-Rebuilt-0.7.0.pkg)

Versioned announcement posts can also live on [GitHub Releases](https://github.com/lostlessvisuals/camelcrusher-rebuilt/releases).

## Why

CamelCrusher was a staple plug-in for a lot of Mac users, but on modern macOS, and especially on Apple Silicon, the original workflow became increasingly fragile or unavailable. `CamelCrusher Rebuilt` exists to bring that workflow back in a native form, with a clear emphasis on getting older CamelCrusher sessions open and usable again on modern Macs.

## Current Status

The project is working locally on Apple Silicon macOS across all three formats, with verified `VST2` session recall and matching custom UI across the current wrappers.

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## More

- Engineering details: [docs/ENGINEERING_GUIDE.md](docs/ENGINEERING_GUIDE.md)
- Release/install notes: [docs/RELEASE_GUIDE.md](docs/RELEASE_GUIDE.md)
- Current project continuity: [docs/CURRENT_HANDOFF.md](docs/CURRENT_HANDOFF.md)

## License

Licensed under the [Apache License 2.0](LICENSE).
