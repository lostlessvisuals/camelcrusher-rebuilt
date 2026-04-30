#!/usr/bin/env python3

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Tuple

from PIL import Image, ImageDraw, ImageFont


ROOT = Path("/Volumes/T7/Dropbox/Codex/packages/camelcrusher-recalled")
DEFAULT_SKIN_DIR = ROOT / "reference/original-ui/windows-default-skin"
DEFAULT_MANUAL = (
    ROOT
    / "reference/original-distribution/windows-example-distribution/installed/ProgramData/Camel Audio/CamelCrusherData/CamelCrusherManual.pdf"
)
DEFAULT_MANUAL_THUMB = Path("/tmp/CamelCrusherManual.pdf.png")


@dataclass(frozen=True)
class FontAtlas:
    image_name: str
    characters: str
    widths: List[int]


@dataclass(frozen=True)
class Control:
    name: str
    control_type: str
    image_name: str
    page: str
    coords: Tuple[int, ...]


@dataclass(frozen=True)
class RenderState:
    preset_name: str = "DRM BigBadBeat"
    version_text: str = "v0.10"
    dist_on: bool = True
    filter_on: bool = True
    compress_on: bool = True
    master_on: bool = True
    phat_on: bool = True
    randomize_on: bool = False
    dist_tube: float = 0.55
    dist_mech: float = 0.40
    filter_cutoff: float = 0.25
    filter_res: float = 0.18
    compress_amount: float = 0.66
    master_volume: float = 0.52
    master_mix: float = 0.38


def parse_skin_parameters(path: Path) -> Tuple[Dict[str, FontAtlas], Dict[str, Control]]:
    lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
    fonts: Dict[str, FontAtlas] = {}
    controls: Dict[str, Control] = {}
    index = 0

    while index < len(lines):
        line = lines[index].strip()

        if line == "FONTS":
            index += 1
            while index < len(lines):
                name = lines[index].strip()
                if not name:
                    index += 1
                    continue
                if name.startswith("-") or name == "PARAMETERS":
                    break
                declared_count = int(lines[index + 1].strip())
                characters = lines[index + 2]
                widths = [int(part) for part in lines[index + 3].split()]
                if len(characters) != len(widths):
                    raise ValueError(f"Unexpected font atlas metadata for {name}")
                fonts[name] = FontAtlas(
                    image_name=f"{name}.png",
                    characters=characters,
                    widths=widths,
                )
                if declared_count != len(characters):
                    # The shipped CamelCrusher skin file reports 75 chars for FontDisplay,
                    # but the actual atlas row contains 74. Use the real atlas length.
                    pass
                index += 4
            continue

        if line == "CONTROLS":
            index += 1
            while index < len(lines):
                row = lines[index].strip()
                if not row:
                    index += 1
                    continue
                parts = row.split()
                if len(parts) < 5:
                    break
                name, control_type, image_name, page, *coord_text = parts
                controls[name] = Control(
                    name=name,
                    control_type=control_type,
                    image_name=image_name,
                    page=page,
                    coords=tuple(int(value) for value in coord_text),
                )
                index += 1
            continue

        index += 1

    return fonts, controls


def extract_vertical_frame(image: Image.Image, frame_index: int, frame_count: int) -> Image.Image:
    frame_height = image.height // frame_count
    top = frame_index * frame_height
    return image.crop((0, top, image.width, top + frame_height))


def draw_bitmap_text(
    canvas: Image.Image,
    position: Tuple[int, int],
    text: str,
    atlas: FontAtlas,
    atlas_image: Image.Image,
) -> None:
    x, y = position
    mapping = {char: idx for idx, char in enumerate(atlas.characters)}
    offsets: List[int] = []
    running_x = 0
    for width in atlas.widths:
        offsets.append(running_x)
        running_x += width
    for char in text:
        idx = mapping.get(char)
        if idx is None:
            x += 6
            continue
        width = atlas.widths[idx]
        glyph_x = offsets[idx]
        glyph = atlas_image.crop((glyph_x, 0, glyph_x + width, atlas_image.height))
        canvas.alpha_composite(glyph, (x, y))
        x += width


def draw_fallback_text(
    canvas: Image.Image,
    position: Tuple[int, int],
    text: str,
    fill: Tuple[int, int, int, int],
) -> None:
    draw = ImageDraw.Draw(canvas)
    font = ImageFont.load_default()
    draw.text(position, text, fill=fill, font=font)


def make_render(
    skin_dir: Path,
    background_path: Path,
    knob_mode: str,
    module_button_mode: str,
    phat_button_mode: str,
    random_button_mode: str,
    output_path: Path,
    state: RenderState,
) -> Path:
    fonts, controls = parse_skin_parameters(skin_dir / "SkinParameters.txt")

    background = Image.open(background_path).convert("RGBA")
    knob_sheet = Image.open(skin_dir / "Knob.png").convert("RGBA")
    on_button = Image.open(skin_dir / "OnButton.png").convert("RGBA")
    phat_button = Image.open(skin_dir / "PhatButton.png").convert("RGBA")
    selector = Image.open(skin_dir / "SelectorPreset.png").convert("RGBA")
    randomize = Image.open(skin_dir / "ButtonRandom.png").convert("RGBA")
    font_selector = Image.open(skin_dir / "FontSelector.png").convert("RGBA")
    font_version = Image.open(skin_dir / "FontVersion.png").convert("RGBA")

    canvas = background.copy()

    def resolve_position(
        control: Control,
        width: int,
        height: int,
        mode: str,
    ) -> Tuple[int, int]:
        x, y = control.coords[:2]
        if mode == "literal":
            return x, y
        if mode == "center-anchor":
            return x - (width // 2), y - (height // 2)
        raise ValueError(f"Unknown control mode: {mode}")

    def paste_control(
        control_name: str,
        image: Image.Image,
        frame_count: int,
        on: bool,
        mode: str,
    ) -> None:
        control = controls[control_name]
        frame = extract_vertical_frame(image, 0 if on else 1, frame_count)
        x, y = resolve_position(control, frame.width, frame.height, mode)
        canvas.alpha_composite(frame, (x, y))

    def paste_knob(control_name: str, value: float) -> None:
        control = controls[control_name]
        frame_count = knob_sheet.height // 48
        frame_index = round(max(0.0, min(1.0, value)) * (frame_count - 1))
        frame = extract_vertical_frame(knob_sheet, frame_index, frame_count)
        x, y = control.coords[:2]
        if knob_mode == "literal":
            paste_x, paste_y = x, y
        elif knob_mode == "center-anchor":
            paste_x, paste_y = x - (frame.width // 2), y - (frame.height // 2)
        else:
            raise ValueError(f"Unknown knob mode: {knob_mode}")
        canvas.alpha_composite(frame, (paste_x, paste_y))

    preset = controls["PresetSelector"]
    preset_x1, preset_y1, preset_x2, preset_y2 = preset.coords
    selector_frame = extract_vertical_frame(selector, 0, 3)
    canvas.alpha_composite(selector_frame, (preset_x1, preset_y1))

    random_control = controls["Randomize"]
    random_frame = extract_vertical_frame(randomize, 0 if state.randomize_on else 1, 2)
    random_x, random_y = resolve_position(
        random_control, random_frame.width, random_frame.height, random_button_mode
    )
    canvas.alpha_composite(random_frame, (random_x, random_y))

    paste_control("DistOn", on_button, 2, state.dist_on, module_button_mode)
    paste_control("MmFilterOn", on_button, 2, state.filter_on, module_button_mode)
    paste_control("CompressOn", on_button, 2, state.compress_on, module_button_mode)
    paste_control("MasterOn", on_button, 2, state.master_on, module_button_mode)
    paste_control("CompressMode", phat_button, 2, state.phat_on, phat_button_mode)

    paste_knob("DistTube", state.dist_tube)
    paste_knob("DistMech", state.dist_mech)
    paste_knob("MmFilterCutoff", state.filter_cutoff)
    paste_knob("MmFilterRes", state.filter_res)
    paste_knob("CompressAmount", state.compress_amount)
    paste_knob("MasterVolume", state.master_volume)
    paste_knob("MasterMix", state.master_mix)

    if "FontSelector" in fonts:
        draw_bitmap_text(
            canvas,
            (49, 121),
            state.preset_name,
            fonts["FontSelector"],
            font_selector,
        )
    else:
        draw_fallback_text(canvas, (49, 121), state.preset_name, (214, 220, 255, 255))

    if "FontVersion" in fonts:
        draw_bitmap_text(
            canvas,
            (202, 66),
            state.version_text,
            fonts["FontVersion"],
            font_version,
        )
    else:
        draw_fallback_text(canvas, (202, 66), state.version_text, (220, 200, 120, 255))

    output_path.parent.mkdir(parents=True, exist_ok=True)
    canvas.save(output_path)
    return output_path


def crop_manual_plugin(manual_thumb: Path, output_path: Path) -> Path:
    image = Image.open(manual_thumb).convert("RGBA")
    crop = image.crop((223, 275, 749, 842))
    output_path.parent.mkdir(parents=True, exist_ok=True)
    crop.save(output_path)
    return output_path


def build_comparison_sheet(
    labeled_images: Iterable[Tuple[str, Path]],
    output_path: Path,
) -> Path:
    rendered: List[Tuple[str, Image.Image]] = []
    for label, path in labeled_images:
        rendered.append((label, Image.open(path).convert("RGBA")))

    label_height = 28
    padding = 18
    width = sum(image.width for _, image in rendered) + padding * (len(rendered) + 1)
    height = max(image.height for _, image in rendered) + label_height + padding * 2
    sheet = Image.new("RGBA", (width, height), (24, 24, 27, 255))
    draw = ImageDraw.Draw(sheet)

    x = padding
    for label, image in rendered:
        draw.text((x, padding - 2), label, fill=(230, 230, 235, 255))
        sheet.alpha_composite(image, (x, padding + label_height))
        x += image.width + padding

    output_path.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(output_path)
    return output_path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Render CamelCrusher layout probes.")
    parser.add_argument(
        "--skin-dir",
        type=Path,
        default=DEFAULT_SKIN_DIR,
        help="Skin directory containing Background.png, Knob.png, and SkinParameters.txt.",
    )
    parser.add_argument(
        "--background",
        type=Path,
        default=DEFAULT_SKIN_DIR / "Background.png",
        help="Background image to composite against.",
    )
    parser.add_argument(
        "--manual-thumb",
        type=Path,
        default=DEFAULT_MANUAL_THUMB,
        help="Quick Look thumbnail for the original manual.",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("/tmp/camelcrusher-layout-probe"),
        help="Directory for rendered probe images.",
    )
    parser.add_argument(
        "--module-button-mode",
        choices=("literal", "center-anchor"),
        default="literal",
        help="Placement model for the four On buttons.",
    )
    parser.add_argument(
        "--phat-button-mode",
        choices=("literal", "center-anchor"),
        default="literal",
        help="Placement model for the Phat button.",
    )
    parser.add_argument(
        "--random-button-mode",
        choices=("literal", "center-anchor"),
        default="literal",
        help="Placement model for the Randomize button.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    output_dir = args.output_dir

    original_manual_crop = crop_manual_plugin(
        args.manual_thumb,
        output_dir / "manual-plugin-crop.png",
    )

    original_literal = make_render(
        args.skin_dir,
        args.skin_dir / "Background.png",
        "literal",
        args.module_button_mode,
        args.phat_button_mode,
        args.random_button_mode,
        output_dir / "original-literal.png",
        RenderState(),
    )
    original_center = make_render(
        args.skin_dir,
        args.skin_dir / "Background.png",
        "center-anchor",
        args.module_button_mode,
        args.phat_button_mode,
        args.random_button_mode,
        output_dir / "original-center-anchor.png",
        RenderState(),
    )
    edited_center = make_render(
        args.skin_dir,
        args.background,
        "center-anchor",
        args.module_button_mode,
        args.phat_button_mode,
        args.random_button_mode,
        output_dir / "edited-center-anchor.png",
        RenderState(),
    )

    sheet = build_comparison_sheet(
        [
            ("Manual Crop", original_manual_crop),
            ("Original Skin / literal", original_literal),
            ("Original Skin / center-anchor", original_center),
            ("Edited Background / center-anchor", edited_center),
        ],
        output_dir / "comparison-sheet.png",
    )

    print(original_manual_crop)
    print(original_literal)
    print(original_center)
    print(edited_center)
    print(sheet)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
