# Spredit

A simple sprite editor for 16x16 sprites using 16 colors.


This project is my first nontrivial C project.


## File Format

|Name              |Value     |Type       |Length    |
|------------------|----------|-----------|----------|
|Magic             |"sprt"    |char[4]    |4         |
|Number of Sprites |n         |uint32     |4         |
|Colorspalette     |rgba      |uint32[16] |16*4 = 64 |
|Sprite 0          |name      |char (*)   |64        |
|                  |bitmap    |char (*)   |128       |
|...               |...       |...        |...       |
|Sprite (n-1)      |name      |char (*)   |64        |
|                  |bitmap    |char (*)   |128       |

totlen = 72 + 192*n
