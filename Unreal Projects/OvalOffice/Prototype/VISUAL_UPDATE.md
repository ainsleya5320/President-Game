# Oval Office visual update — September 23, 2026

The browser game now uses the smooth full-resolution Blender furniture instead of the simplified preview. Static meshes are combined by material for efficient rendering while the room's cutaway controls remain available.

The room includes golden fabric drapes; textured oak and parquet; cream woven upholstery and striped cushions; blue glazed porcelain lamps; a floral arrangement; a detailed marble fireplace with fluted jambs, trim and logs; and individually shaped mantel leaves. The existing paintings and Rose Garden-inspired backdrop are preserved.

Daylight casts furniture and sash shadows, lamps add warm light, and subtle contact shadows anchor the seating. Walking allows looking up and down by dragging. The room fits the browser panel height rather than extending below it.

Build with `build_prototype.py`. Room-specific material and furnishing changes live in `room-polish.js`; layout, views, and gameplay remain in `office-game.template.html`. The builder embeds the model, existing art, and room code into `Four_Years_Prototype.html`. Three.js modules load from the existing pinned CDN.

This is an artistic reconstruction in the browser prototype. The native Unreal level and original Blender file have not been modified by this visual update.

Validation: inspected the walking, fireplace and cutaway views in the running game, checked the saved briefing remained available, and checked browser errors. No decisions were advanced during the visual review.
