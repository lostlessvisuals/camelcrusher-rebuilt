## CamelCrusher Windows Default Skin

This folder is a direct reference copy of the installed Windows CamelCrusher skin payload from:

`C:\ProgramData\Camel Audio\CamelCrusherData\Skins\default`

The original plug-in already stores its UI as external skin assets, so the clean extraction path was to copy the skin directory rather than trying to carve bitmaps out of `CamelCrusher.dll`.

### Included assets

- `Background.png` - 345x373 faceplate/background
- `Knob.png` - 48x2640 knob filmstrip used by the rotary controls
- `OnButton.png` - 32x48 on/off switch sprite
- `PhatButton.png` - 22x40 compressor mode button sprite
- `ButtonRandom.png` - 93x56 randomize button sprite
- `SelectorPreset.png` - 196x90 preset selector sprite
- `FontDisplay.png` - 414x12 bitmap font atlas
- `FontDisplay-Old.png` - 725x20 older bitmap font atlas variant
- `FontSelector.png` - 418x12 selector/version text atlas
- `FontVersion.png` - 435x9 version text atlas
- `SkinParameters.txt` - original bitmap/font/control mapping and UI coordinates

### Useful layout notes

`SkinParameters.txt` identifies the original control names and positions:

- `DistOn`, `DistMech`, `DistTube`
- `MmFilterOn`, `MmFilterCutoff`, `MmFilterRes`
- `CompressOn`, `CompressAmount`, `CompressMode`
- `MasterOn`, `MasterMix`, `MasterVolume`
- `Randomize`
- `PresetSelector`
- `VersionDisplay`
- `InfoLink`

For rebuilding work, `Background.png` plus `SkinParameters.txt` is the fastest way to reconstruct the original panel geometry, and `Knob.png` is the key sprite sheet for the rotary controls.
