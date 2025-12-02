#include <math.h>
#define NOB_IMPLEMENTATION
#include "nob.h"
#include <raylib.h>

// #define DEBUG

#ifdef DEBUG
#define DEBUG_RECT(rect) DrawRectangleLinesEx(rect, 5, RED);
#endif

#ifndef DEBUG
#define DEBUG_RECT(rect)
#endif

// clang-format off
#define COLOR(x)\
    (Color){(x >> 8 * 3) & 0xFF,\
    (x >> 8 * 2) & 0xFF,\
    (x >> 8 * 1) & 0xFF,\
    (x >> 8 * 0) & 0xFF}
// clang-format on

const int WIDTH = 1200;
const int HEIGHT = 900;

const Color BACKGROUND = COLOR(0x222222FF);
const Color TEXT_COLOR = WHITE;
const Color UI_ELEM_COLOR = GRAY;

const int UI_ELEM_HEIGHT = 50;
const float MARGINS = 30;
const int TITLE_BAR = 60;

const int SLIDER_WIDTH = 256;

enum { SPRITE_SIZE = 16 };

// half a byte
enum { NUM_COLORS = 16 };

typedef unsigned char Sprite[SPRITE_SIZE * SPRITE_SIZE / 2];

typedef struct {
    Rectangle r1;
    Rectangle r2;
} RectTuple;

RectTuple vsplit(Rectangle rect, float left_fr, float right_fr) {
    float left = left_fr / (left_fr + right_fr);
    float right = right_fr / (left_fr + right_fr);
    float tot_width = rect.width - MARGINS;
    return (RectTuple){
        (Rectangle){
            .x = rect.x,
            .y = rect.y,
            .width = tot_width * left,
            .height = rect.height,
        },
        (Rectangle){
            .x = rect.x + tot_width * left + MARGINS,
            .y = rect.y,
            .width = tot_width * right,
            .height = rect.height,
        },
    };
}

RectTuple hsplit(Rectangle rect, float top_fr, float bottom_fr) {
    float top = top_fr / (top_fr + bottom_fr);
    float bottom = bottom_fr / (top_fr + bottom_fr);
    float tot_height = rect.height - MARGINS;
    return (RectTuple){
        (Rectangle){
            .x = rect.x,
            .y = rect.y,
            .width = rect.width,
            .height = tot_height * top,
        },
        (Rectangle){
            .x = rect.x,
            .y = rect.y + tot_height * top + MARGINS,
            .width = rect.width,
            .height = tot_height * bottom,
        },
    };
}

Rectangle vsubdivide(Rectangle rect, int count, int index) {
    float height = (rect.height - MARGINS * (count - 1)) / (float)count;
    return (Rectangle){
        .x = rect.x,
        .y = rect.y + (height + MARGINS) * index,
        .width = rect.width,
        .height = height,
    };
}

Rectangle hsubdivide(Rectangle rect, int count, int index) {
    float width = (rect.width - MARGINS * (count - 1)) / (float)count;
    return (Rectangle){
        .x = rect.x + (width + MARGINS) * index,
        .y = rect.y,
        .width = width,
        .height = rect.height,
    };
}

RectTuple chop_top(Rectangle rect, float height) {
    return (RectTuple){
        (Rectangle){
            .x = rect.x,
            .y = rect.y,
            .width = rect.width,
            .height = height,
        },
        (Rectangle){
            .x = rect.x,
            .y = rect.y + height + MARGINS,
            .width = rect.width,
            .height = rect.height - height - MARGINS,
        },
    };
}

RectTuple chop_bottom(Rectangle rect, float height) {
    return (RectTuple){
        (Rectangle){
            .x = rect.x,
            .y = rect.y,
            .width = rect.width,
            .height = rect.height - height - MARGINS,
        },
        (Rectangle){
            .x = rect.x,
            .y = rect.y + rect.height - height,
            .width = rect.width,
            .height = height,
        },
    };
}

Rectangle shrink(Rectangle rect, float radius) {
    return (Rectangle){.x = rect.x + radius,
                       .y = rect.y + radius,
                       .width = rect.width - 2 * radius,
                       .height = rect.height - 2 * radius};
}

Rectangle fit_square_factor(Rectangle rect, int factor) {
    int x = ceil(rect.x);
    int y = ceil(rect.y);
    int width = floor(rect.x + rect.width) - x;
    int height = floor(rect.y + rect.height) - y;
    int size;
    if (width < height) {
        size = (width / factor) * factor;
    } else {
        size = (height / factor) * factor;
    }
    int left = x + (width - size) / 2;
    int top = y + (height - size) / 2;
    return (Rectangle){
        .x = left,
        .y = top,
        .height = size,
        .width = size,
    };
}

Rectangle get_window_rect() {
    return (Rectangle){
        .x = 0,
        .y = 0,
        .width = GetScreenWidth(),
        .height = GetScreenHeight(),
    };
}

Rectangle draw_title(const char *text) {
    DrawText(text, MARGINS, MARGINS, TITLE_BAR * 18 / 20, WHITE);
    Rectangle screen = shrink(get_window_rect(), MARGINS);
    screen.y += TITLE_BAR + MARGINS;
    screen.height -= TITLE_BAR + MARGINS;
    return screen;
}

typedef struct {
    char name[32];
    Sprite *sprite;
} Entry;

typedef struct {
    Entry *items;
    int count;
    int capacity;
} SpriteList;

Color COLORS[NUM_COLORS] = {0};

SpriteList GLOB_SPRITES = {0};

const char *COLOR_PATH = "res/color.bin";

int load_colors() {
    FILE *file = fopen(COLOR_PATH, "rb");
    if (file == NULL) {
        TraceLog(LOG_FATAL, "No color file found");
        for (int i = 0; i < NUM_COLORS; i++) {
            COLORS[i] = COLOR(0xFFFFFFFF);
        }
        return 0;
    }

    int objs_read = fread((void *)&COLORS, sizeof(Color), NUM_COLORS, file);
    if (objs_read != NUM_COLORS) {
        TraceLog(LOG_FATAL, "color file had incorrect len");
        fclose(file);
        return 1;
    };
    fclose(file);
    return 0;
}

const char *SPRITES_PATH = "res/sprites/";

int read_sprite(Sprite *sprite, FILE *file) {
    int objs_read = fread((void *)sprite, sizeof(char),
                          SPRITE_SIZE * SPRITE_SIZE / 2, file);
    if (objs_read != SPRITE_SIZE * SPRITE_SIZE / 2) {
        return 1;
    }
    return 0;
}

int load_sprites() {
    Entry sprite_entry = (Entry){0};

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
                TraceLog(LOG_WARNING, "Path construction failed for: %s",
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
            nob_da_append(&GLOB_SPRITES, sprite_entry);
        }

        (void)closedir(dir);
    } else {
        TraceLog(LOG_FATAL, "Couldn't open the directory %s", SPRITES_PATH);
        return 1;
    }

    TraceLog(LOG_INFO, "Loaded %d sprites", GLOB_SPRITES.count);

    return 0;
}

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

void color_sliders(Color *color, Rectangle rect) {
    RectTuple split = chop_top(rect, TITLE_BAR);
    DrawText(TextFormat("#%08X", *(uint32_t *)color), rect.x, rect.y,
             TITLE_BAR * 15 / 20, WHITE);
    for (int i = 0; i < 4; i++) {
        rgba_slider(color, i, vsubdivide(split.r2, 4, i));
    }
    DEBUG_RECT(split.r1);
    DEBUG_RECT(split.r2);
}

void edit_colors() {
    int selected = -1;
    bool should_exit = false;

    while (!should_exit) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            selected = -1;
        }

        BeginDrawing();
        ClearBackground(BACKGROUND);

        Rectangle main_region = draw_title("Editing Color Palette");
        RectTuple main_split = vsplit(main_region, 3, 2);
        Rectangle colors = fit_square_factor(main_split.r1, 4);

        for (int i = 0; i < NUM_COLORS; i++) {
            int x = i % 4;
            int y = i / 4;
            Rectangle rect = (Rectangle){
                colors.x + x * colors.width / 4,
                colors.y + y * colors.height / 4,
                colors.width / 4,
                colors.height / 4,
            };
            DrawRectangleRec(rect, COLORS[i]);
            if (clickable_region(rect)) {
                selected = i;
            }
            if (selected == i) {
                DrawRectangleLinesEx(rect, MARK_LINE_THICK, BLACK);
            }
        }

        RectTuple edit_split = chop_bottom(main_split.r2, UI_ELEM_HEIGHT);

        if (selected >= 0) {
            color_sliders(&COLORS[selected], edit_split.r1);
        }

        RectTuple button_split = vsplit(edit_split.r2, 1, 1);

        should_exit = clickable_box("exit", button_split.r1, UI_ELEM_COLOR);

        if (clickable_box("save", button_split.r2, UI_ELEM_COLOR)) {
            write_colors();
        }
        EndDrawing();
    }

    // TODO: ask if unwritten changes should be saved

    // load colors to make sure they are what is uptodate
    load_colors();
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

int sprite_selector(Rectangle rect) {
    int pixel_scale = (float)(rect.width * 0.8 / 4 / 16);
    int tot_width = pixel_scale * 4 * 16;
    int h_margin = (rect.width - tot_width) / 3;
    int v_margin = 20;

    int sprite_to_edit = -1;

    for (int i = 0; i < GLOB_SPRITES.count; i++) {
        int x = i % 4;
        int y = i / 4;

        float sprite_left = rect.x + (pixel_scale * 16 + h_margin) * x;
        float sprite_top = rect.y + (pixel_scale * 16 + h_margin) * y;
        sprite_draw(GLOB_SPRITES.items[i].sprite, pixel_scale, sprite_left,
                    sprite_top);
        if (clickable_region((Rectangle){.x = sprite_left,
                                         .y = sprite_top,
                                         .width = pixel_scale * 16,
                                         .height = pixel_scale * 16})) {
            sprite_to_edit = i;
        }
    }
    return sprite_to_edit;
}

int main() {
    InitWindow(WIDTH, HEIGHT, "Spredit");
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    SetTargetFPS(60);

    if (load_colors() != 0) {
        return 1;
    }

    if (load_sprites() != 0) {
        return 1;
    }

    bool should_quit = false;
    while (!should_quit) {
        bool palette = false;
        int sprite_to_edit = -1;

        BeginDrawing();
        ClearBackground(BACKGROUND);

        Rectangle main_region = draw_title("Spredit");

        RectTuple main_split = vsplit(main_region, 2, 5);

        int current_y = TITLE_BAR + 2 * MARGINS;

        palette =
            clickable_box("Edit Palette",
                          (Rectangle){main_split.r1.x, current_y,
                                      main_split.r1.width, UI_ELEM_HEIGHT},
                          UI_ELEM_COLOR);
        current_y += UI_ELEM_HEIGHT + MARGINS;

        bool new =
            clickable_box("new sprite",
                          (Rectangle){main_split.r1.x, current_y,
                                      main_split.r1.width, UI_ELEM_HEIGHT},
                          UI_ELEM_COLOR);
        current_y += UI_ELEM_HEIGHT + MARGINS;

        should_quit =
            clickable_box("quit",
                          (Rectangle){main_split.r1.x, current_y,
                                      main_split.r1.width, UI_ELEM_HEIGHT},
                          UI_ELEM_COLOR);

        sprite_to_edit = sprite_selector(main_split.r2);

        EndDrawing();
        if (palette) {
            edit_colors();
        }
        if (new) {
            nob_da_append(&GLOB_SPRITES, (Entry){0});
            edit_sprite(GLOB_SPRITES.count - 1);
        }
        if (sprite_to_edit != -1) {
            edit_sprite(sprite_to_edit);
        }
    }
    CloseWindow();
}
