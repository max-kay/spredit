# Spredit

A simple sprite editor for 16x16 sprites using 16 colors.


This project is my first nontrivial C project.


## File Format

Two binary formats are supported: **named sprites** (`sprt`) and **unnamed sprites** (`spru`). Both share a common header:

### Header (72 bytes)

* **magic**: `char[4]` — `"sprt"` or `"spru"`
* **sprite_count**: `uint32`
* **color_palette**: `uint32[16]` — 16 RGBA colors

---

## Named Sprites (`sprt`)

Each sprite entry contains a name and a bitmap.

| Field  | Size  | Description          |
| ------ | ----- | -------------------- |
| name   | 64 B  | Sprite name (string) |
| bitmap | 128 B | Bitmap data          |

**Total size:** `72 + 192 * sprite_count` bytes

---

## Unnamed Sprites (`spru`)

Each sprite entry contains only a bitmap.

| Field  | Size  | Description |
| ------ | ----- | ----------- |
| bitmap | 128 B | Bitmap data |

**Total size:** `72 + 128 * sprite_count` bytes
