#define NOB_IMPLEMENTATION
#include "nob.h"
#include <raylib.h>

// clang-format off
#define COLOR(x)\
    (Color){(x >> 8 * 3) & 0xFF,\
    (x >> 8 * 2) & 0xFF,\
    (x >> 8 * 1) & 0xFF,\
    (x >> 8 * 0) & 0xFF}
// clang-format on

const Color BACKGROUND = COLOR(0x222222FF);
const Color TEXT_COLOR = WHITE;
const Color UI_ELEM_COLOR = GRAY;
const int WIDTH = 1200;
const int HEIGHT = 900;

const int MARGINS = 30;
const int TITLE_BAR = 60;

const int SLIDER_WIDTH = 256;

enum { SPRITE_SIZE = 16 };

// half a byte
enum { NUM_COLORS = 16 };

Color COLORS[NUM_COLORS] = {0};

typedef unsigned char Sprite[SPRITE_SIZE * SPRITE_SIZE / 2];

void sprite_draw(Sprite *sprite, int pixel_width, int left, int top) {
    if (sprite == NULL) {
        return;
    }
    for (int i = 0; i < SPRITE_SIZE * SPRITE_SIZE; i++) {
        int idx = i / 2;
        int color_idx;
        if (i % 2 == 0) {
            color_idx = ((unsigned char *)(sprite))[idx] >> 4 & 0x0F;
        } else {
            color_idx = ((unsigned char *)(sprite))[idx] & 0x0F;
        }
        int x = i % 16;
        int y = i / 16;
        DrawRectangle(left + x * pixel_width, top + y * pixel_width,
                      pixel_width, pixel_width, COLORS[color_idx]);
    }
}

const char *SPRITES_PATH = "res/sprites/";

typedef struct {
    char name[32];
    Sprite *sprite;
} Entry;

typedef struct {
    Entry *items;
    int count;
    int capacity;

} SpriteList;

int read_sprite(Sprite *sprite, FILE *file) {
    int objs_read = fread((void *)sprite, sizeof(char),
                          SPRITE_SIZE * SPRITE_SIZE / 2, file);
    if (objs_read != SPRITE_SIZE * SPRITE_SIZE / 2) {
        return 1;
    }
    return 0;
}

int load_sprites(SpriteList *list) {
    Entry sprite_entry = (Entry){.name = "new sprite", .sprite = NULL};
    nob_da_append(list, sprite_entry);

    DIR *dir;
    struct dirent *dir_entry;

    dir = opendir(SPRITES_PATH);
    if (dir != NULL) {
        while ((dir_entry = readdir(dir))) {
            sprite_entry = (Entry){0};
            if (dir_entry->d_namlen > 32) {
                TraceLog(LOG_TRACE, "Path was to long for: %s\n",
                         dir_entry->d_name);
                continue;
            }
            int dot_idx = -1;
            for (int i = 0; i < 32 && i < dir_entry->d_namlen; i++) {
                char c = dir_entry->d_name[i];
                if (c == '.') {
                    dot_idx = i;
                    break;
                }
                sprite_entry.name[i] = c;
            }
            if (dot_idx == -1) {
                continue;
            }

            if (strcmp(&dir_entry->d_name[dot_idx], ".bin") != 0) {
                continue;
            }

            TraceLog(LOG_INFO, "loading: %s", dir_entry->d_name);

            char full_path[512];
            int len = snprintf(full_path, sizeof(full_path), "%s/%s",
                               SPRITES_PATH, dir_entry->d_name);

            if (len < 0) {
                TraceLog(LOG_TRACE, "Path construction failed for: %s",
                         dir_entry->d_name);
                continue;
            }

            Sprite *ptr = malloc(sizeof(Sprite));
            if (ptr == NULL) {
                TraceLog(LOG_FATAL, "cound not allocate sprite");
                return 1;
            }
            char *path = full_path;
            FILE *file = fopen(path, "rb");
            if (file == NULL) {
                free((void *)ptr);
                continue;
            }
            if (read_sprite(ptr, file) != 0) {
                free((void *)ptr);
                continue;
            };
            sprite_entry.sprite = ptr;
            nob_da_append(list, sprite_entry);
        }

        (void)closedir(dir);
    } else {
        TraceLog(LOG_FATAL, "Couldn't open the directory");
        return 1;
    }

    TraceLog(LOG_INFO, "Loaded %d sprites", list->count);

    return 0;
}

const char *COLOR_PATH = "res/color.bin";

int load_colors() {
    FILE *file = fopen(COLOR_PATH, "rb");
    if (file == NULL) {
        puts("No color file found");
        for (int i = 0; i < NUM_COLORS; i++) {
            COLORS[i] = COLOR(0xFFFFFFFF);
        }
        return 0;
    }

    int objs_read = fread((void *)&COLORS, sizeof(Color), NUM_COLORS, file);
    if (objs_read != NUM_COLORS) {
        puts("color file had incorrect len");
        fclose(file);
        return 1;
    };
    fclose(file);
    return 0;
}

int write_colors() {
    FILE *file = fopen(COLOR_PATH, "wb");
    if (file == NULL) {
        puts("could not write colors");
        return 1;
    }
    fwrite(&COLORS, sizeof(Color), NUM_COLORS, file);
    fflush(file);
    fclose(file);

    return 0;
}

const int MARK_LINE_THICK = 5;

bool clickable_region(Rectangle rect) {
    if (CheckCollisionPointRec(GetMousePosition(), rect)) {
        DrawRectangleLinesEx(rect, MARK_LINE_THICK, WHITE);
        if (IsMouseButtonPressed(0)) {
            return true;
        }
    }
    return false;
}

bool clickable_box(const char *text, Rectangle rect, Color color) {
    DrawRectangleRec(rect, color);
    float fontsize = rect.height * 0.8;
    DrawText(text, rect.x + rect.height * 0.2, rect.y + rect.height * 0.1,
             fontsize, TEXT_COLOR);
    return clickable_region(rect);
}

void rgba_slider(Color *color, int index, Rectangle rect) {
    const int shift_const = index * 8;
    uint32_t org_as_int = *((uint32_t *)color);
    uint32_t c0 = *(&org_as_int);
    c0 &= ~((uint32_t)0xFF << shift_const);
    uint32_t c1 = *(&org_as_int);
    c1 |= (uint32_t)0xFF << shift_const;
    DrawRectangleGradientEx(rect, *(Color *)&c0, *(Color *)&c0, *(Color *)&c1,
                            *(Color *)&c1);
    float t = (float)((org_as_int >> shift_const) & 0xFF) / 0xFF;

    DrawTriangle((Vector2){rect.x + t * rect.width, rect.y + 0.8 * rect.height},
                 (Vector2){rect.x + t * rect.width - 0.4 * rect.height,
                           rect.y + 1.2 * rect.height},
                 (Vector2){rect.x + t * rect.width + 0.4 * rect.height,
                           rect.y + 1.2 * rect.height},
                 BLACK);
    if (CheckCollisionPointRec(GetMousePosition(), rect) &&
        IsMouseButtonDown(0)) {
        unsigned char new =
            (unsigned char)((GetMouseX() - rect.x) / rect.width * 255.0);
        if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
            new = 0xFF;
        }
        if (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) {
            new = 0x00;
        }
        org_as_int &= ~(0xFF << shift_const);
        org_as_int += (uint32_t)new << shift_const;
        *(uint32_t *)color = org_as_int;
    }
}

void edit_colors() {
    int selected = -1;
    bool should_exit = false;

    while (!should_exit) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            selected = -1;
        }

        int color_pad_width = (GetScreenHeight() - TITLE_BAR - 3 * MARGINS) / 4;
        int left = MARGINS;
        int top = 2 * MARGINS + TITLE_BAR;

        BeginDrawing();
        ClearBackground(BACKGROUND);

        DrawText("Editing Color Palette", MARGINS, MARGINS, TITLE_BAR * 18 / 20,
                 WHITE);

        for (int i = 0; i < NUM_COLORS; i++) {
            int x = i % 4;
            int y = i / 4;
            Rectangle rect = (Rectangle){
                left + x * color_pad_width,
                top + y * color_pad_width,
                color_pad_width,
                color_pad_width,
            };
            if (clickable_box("", rect, COLORS[i])) {
                selected = i;
            }
            if (selected == i) {
                DrawRectangleLinesEx(rect, MARK_LINE_THICK, BLACK);
            }
        }

        float height = 50;
        float new_left = left + 4 * color_pad_width + MARGINS;
        float width = GetScreenWidth() - new_left - MARGINS;
        if (selected >= 0) {
            DrawText(TextFormat("%08x", *(uint32_t *)&COLORS[selected]),
                     new_left, MARGINS, TITLE_BAR * 15 / 20, WHITE);
            for (int i = 0; i < 4; i++) {
                rgba_slider(
                    &COLORS[selected], i,
                    (Rectangle){.x = new_left,
                                .y = TITLE_BAR + MARGINS * (i + 2) + i * height,
                                .width = width,
                                .height = height});
            }
        }

        int exit_width = 190;
        int exit_height = 40;
        Rectangle rect =
            (Rectangle){.x = GetScreenWidth() - exit_width - MARGINS,
                        .y = GetScreenHeight() - exit_height - MARGINS,
                        .width = exit_width,
                        .height = exit_height};
        should_exit = clickable_box("exit", rect, UI_ELEM_COLOR);

        rect = (Rectangle){.x = new_left,
                           .y = GetScreenHeight() - exit_height - MARGINS,
                           .width = GetScreenWidth() - new_left - 2 * MARGINS -
                                    exit_width,
                           .height = exit_height};
        if (clickable_box("save", rect, UI_ELEM_COLOR)) {
            write_colors();
        }
        EndDrawing();
    }

    return;
}

void edit_sprite(int idx) {
    bool should_close = true;
    while (!should_close) {
        BeginDrawing();
        ClearBackground(WHITE);
        EndDrawing();
    }
    return;
}

int main() {
    SpriteList sprites = {0};
    InitWindow(WIDTH, HEIGHT, "Sprite Edit");
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    SetTargetFPS(60);

    if (load_colors() != 0) {
        return 1;
    }

    if (load_sprites(&sprites) != 0) {
        return 1;
    }

    bool should_quit = false;
    while (!should_quit) {
        bool palette = false;
        int sprite_to_edit = -1;

        BeginDrawing();
        ClearBackground(BACKGROUND);

        DrawText("Sprite Edit", MARGINS, MARGINS, TITLE_BAR * 18 / 20, WHITE);
        int current_y = TITLE_BAR + 2 * MARGINS;

        int elem_height = 50;
        int elem_width = 300;
        palette = clickable_box(
            "Edit Palette",
            (Rectangle){MARGINS, current_y, elem_width, elem_height},
            UI_ELEM_COLOR);
        current_y += elem_height + MARGINS;

        should_quit = clickable_box(
            "quit", (Rectangle){MARGINS, current_y, elem_width, elem_height},
            UI_ELEM_COLOR);

        float sprite_width = 150;

        for (int i = 0; i < sprites.count; i++) {
            int x = i % 4;
            int y = i / 4;

            float sprite_left = elem_width + 2 * MARGINS + x * sprite_width;
            float sprite_top =
                y * (GetScreenWidth() - (float)elem_width + MARGINS);
            sprite_draw(sprites.items[i].sprite, sprite_width / 16, sprite_left,
                        sprite_top);
            if (clickable_region((Rectangle){.x = sprite_left,
                                             .y = sprite_top,
                                             .width = sprite_width,
                                             .height = sprite_width})) {
                sprite_to_edit = i;
            }
        }

        EndDrawing();
        if (palette) {
            edit_colors();
        }
        if (sprite_to_edit != -1) {
            edit_sprite(sprite_to_edit);
        }
    }
    CloseWindow();
}
