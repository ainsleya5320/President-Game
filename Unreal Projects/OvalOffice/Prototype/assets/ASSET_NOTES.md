# New visual assets for the playable prototype

These images were created with Codex's built-in image generation tool for this prototype. They are original game art, not photographs or reproductions of the White House Rose Garden or any particular painting. The PNG files are the editable source assets. `build_prototype.py` embeds resized JPEG versions in the playable HTML.

| Asset | File | Prompt |
|---|---|---|
| Rose Garden-inspired view | `rose-garden.png` | A softly focused, photorealistic spring garden view from standing eye level: clipped hedges, mature leafy trees, lawn, and pink, ivory, and deep rose flowers; wide landscape, with no window frame, building, people, power lines, text, or watermark. |
| Central portrait | `washington-portrait.png` | An original vertical oil portrait of George Washington from the chest up, powdered hair, dark eighteenth-century coat and ivory cravat, muted background, museum canvas texture; straight-on artwork without frame, wall, text, or watermark. |
| River landscape | `river-landscape.png` | An original late nineteenth-century American landscape oil painting of a broad river valley, wooded banks, distant blue mountains, and cloudy sky; straight-on rectangular artwork without frame, wall, people, text, or watermark. |
| Liberty painting | `statue-liberty.png` | An original vertical oil painting of the Statue of Liberty viewed from below against a muted blue sky, with atmospheric brushwork and aged canvas texture; straight-on artwork without frame, wall, text, or watermark. |

The garden image is a fictional inspired view. It should not be described as an accurate photograph of the real Rose Garden.

## Fictional character portraits

Six square editorial-style portraits were created on 2026-09-26 using the **built-in image_gen tool**, with a separate generation for each fictional character. Source PNGs are in `characters/`: `maya-chen.png`, `elena-ruiz.png`, `daniel-brooks.png`, `samira-bell.png`, `adrian-vale.png`, and `jordan-ellis.png`. The full final prompt for each asset is preserved in [characters/generation.json](characters/generation.json). No source photograph was used.

The game embeds a 256-pixel JPEG derivative of each source during its normal asset build. Names identify invented game characters, not real government officials. The portraits share slate backgrounds and soft lighting. The dashboard itself uses native HTML, CSS and SVG charts driven by simulation values, not a generated interface image.
