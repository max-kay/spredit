#include <math.h>
#include <stdio.h>
#define NOB_IMPLEMENTATION
#include "nob.h"
#include <raylib.h>

#define DEBUG

#ifdef DEBUG
#define DEBUG_RECT(rect) DrawRectangleLinesEx(rect, 2, RED);
#endif

#ifndef DEBUG
#define DEBUG_RECT(rect)
#endif

// clang-format off
#define COLOR(x)\
    (Color){\
        (x >> 8 * 3) & 0xFF,\
        (x >> 8 * 2) & 0xFF,\
        (x >> 8 * 1) & 0xFF,\
        (x >> 8 * 0) & 0xFF,\
    }
// clang-format on

const int WIDTH = 1200;
const int HEIGHT = 900;

const Color BACKGROUND = COLOR(0x222222FF);
const Color TEXT_COLOR = WHITE;

const float MARGINS = 30;
const int TITLE_BAR = 60;

const float LARGE_FONT = 50;
const float MEDIUM_FONT = 30;
const float SMALL_FONT = 20;

const float LITTLE_MARGIN = 10;

const int BUTTON_HEIGHT = MEDIUM_FONT * 1.2;

const Color BUTTON_COLOR = GRAY;

const int MARK_LINE_THICK = 5;

const int MAX_PIXE_SCALE = 15;

enum { SPRITE_SIZE = 16 };

// half a byte
enum { NUM_COLORS = 16 };

typedef struct {
    Rectangle r1;
    Rectangle r2;
} RectTuple;

RectTuple vsplit(Rectangle rect, float left_fr, float right_fr) {
    float left = left_fr / (left_fr + right_fr);
    float right = right_fr / (left_fr + right_fr);
    float tot_width = rect.width - MARGINS;
    return (RectTuple){
        {
            .x = rect.x,
            .y = rect.y,
            .width = tot_width * left,
            .height = rect.height,
        },
        {
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
        {
            .x = rect.x,
            .y = rect.y,
            .width = rect.width,
            .height = tot_height * top,
        },
        {
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
        {
            .x = rect.x,
            .y = rect.y,
            .width = rect.width,
            .height = height,
        },
        {
            .x = rect.x,
            .y = rect.y + height + MARGINS,
            .width = rect.width,
            .height = rect.height - height - MARGINS,
        },
    };
}

RectTuple chop_bottom(Rectangle rect, float height) {
    return (RectTuple){
        {
            .x = rect.x,
            .y = rect.y,
            .width = rect.width,
            .height = rect.height - height - MARGINS,
        },
        {
            .x = rect.x,
            .y = rect.y + rect.height - height,
            .width = rect.width,
            .height = height,
        },
    };
}

Rectangle shrink(Rectangle rect, float radius) {
    return (Rectangle){
        .x = rect.x + radius,
        .y = rect.y + radius,
        .width = rect.width - 2 * radius,
        .height = rect.height - 2 * radius,
    };
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

Rectangle setup_screen(const char *text) {
    DrawText(text, MARGINS, MARGINS, TITLE_BAR * 18 / 20, TEXT_COLOR);
    Rectangle screen = shrink(get_window_rect(), MARGINS);
    screen.y += TITLE_BAR + MARGINS;
    screen.height -= TITLE_BAR + MARGINS;
    return screen;
}

bool clickable_region(Rectangle rect) {
    if (CheckCollisionPointRec(GetMousePosition(), rect)) {
        DrawRectangleLinesEx(rect, MARK_LINE_THICK, TEXT_COLOR);
        if (IsMouseButtonPressed(0)) {
            return true;
        }
    }
    return false;
}

bool button(const char *text, Rectangle rect, Color color) {
    DrawRectangleRec(rect, color);
    float fontsize = rect.height * 0.8;
    DrawText(text, rect.x + rect.height * 0.2, rect.y + rect.height * 0.1,
             fontsize, TEXT_COLOR);
    return clickable_region(rect);
}

int button_list(Rectangle *rect, char *names[], int count) {
    int selected = -1;

    for (int i = 0; i < count; i++) {
        Rectangle r_button = {
            .x = rect->x,
            .y = rect->y,
            .width = rect->width,
            .height = BUTTON_HEIGHT,
        };
        if (button(names[i], r_button, BUTTON_COLOR)) {
            selected = i;
        };
        rect->y += BUTTON_HEIGHT + MARGINS;
        rect->height -= BUTTON_HEIGHT + MARGINS;
    }

    return selected;
}

bool pixel(Rectangle rect, Color color) {
    if (CheckCollisionPointRec(GetMousePosition(), rect)) {
        DrawRectangleRec(rect, color);
        if (IsMouseButtonDown(0)) {
            return true;
        }
    }
    return false;
}

float slider_region(Rectangle rect, float t) {
    DrawTriangle((Vector2){rect.x + t * rect.width, rect.y + 0.8 * rect.height},
                 (Vector2){rect.x + t * rect.width - 0.4 * rect.height,
                           rect.y + 1.2 * rect.height},
                 (Vector2){rect.x + t * rect.width + 0.4 * rect.height,
                           rect.y + 1.2 * rect.height},
                 BLACK);
    if (CheckCollisionPointRec(GetMousePosition(), rect) &&
        IsMouseButtonDown(0)) {
        float new = (GetMouseX() - rect.x) / rect.width;
        if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
            new = 1.0;
        }
        if (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) {
            new = 0.0;
        }
        return new;
    }
    return -1;
}

typedef struct {
    char name[32];
    unsigned char *sprite;
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

int read_sprite(unsigned char *sprite, FILE *file) {
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

            unsigned char *ptr =
                malloc(sizeof(char) * SPRITE_SIZE * SPRITE_SIZE / 2);
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

        closedir(dir);
    } else {
        TraceLog(LOG_FATAL, "Couldn't open the directory %s", SPRITES_PATH);
        return 1;
    }

    TraceLog(LOG_INFO, "Loaded %d sprites", GLOB_SPRITES.count);

    return 0;
}

void sprite_draw(unsigned char *sprite, int pixel_width, int left, int top) {
    if (sprite == NULL) {
        return;
    }
    for (int i = 0; i < SPRITE_SIZE * SPRITE_SIZE; i++) {
        int idx = i / 2;
        int color_idx;
        if (i % 2 == 0) {
            color_idx = sprite[idx] & 0x0F;
        } else {
            color_idx = sprite[idx] >> 4 & 0x0F;
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

void rgbaslider(Rectangle rect, unsigned char *component, char *name) {
    RectTuple split = vsplit(rect, 1, 4);
    DrawText(name, split.r1.x, split.r1.y, 0.8 * split.r1.height, TEXT_COLOR);
    float r = slider_region(split.r2, *component / 255.0);
    if (r != -1) {
        *component = 255.0 * r;
    }
}

void color_sliders(Color *color, Rectangle rect) {
    RectTuple split = chop_top(rect, TITLE_BAR);
    DrawText(
        TextFormat("#%02X%02X%02X%02X", color->r, color->g, color->b, color->a),
        rect.x, rect.y, TITLE_BAR * 15 / 20, WHITE);

    rgbaslider(vsubdivide(split.r2, 4, 0), &color->r, "R");
    rgbaslider(vsubdivide(split.r2, 4, 1), &color->g, "G");
    rgbaslider(vsubdivide(split.r2, 4, 2), &color->b, "B");
    rgbaslider(vsubdivide(split.r2, 4, 3), &color->a, "A");
}

void color_selector(Rectangle rect, int *selected) {
    rect = fit_square_factor(rect, 8);
    if (IsKeyPressed(KEY_ESCAPE)) {
        *selected = -1;
    }
    for (int i = 0; i < NUM_COLORS; i++) {
        int x = i % 4;
        int y = i / 4;
        Rectangle colorpad = {
            .x = rect.x + x * rect.width / 4,
            .y = rect.y + y * rect.height / 4,
            .width = rect.width / 4,
            .height = rect.height / 4,
        };
        DrawRectangleRec(colorpad, COLORS[i]);

        Color opaque = COLORS[i];
        opaque.a = 0xFF;
        Rectangle opaque_r = (Rectangle){
            rect.x + x * rect.width / 4,
            rect.y + y * rect.height / 4,
            rect.width / 8,
            rect.height / 8,
        };
        DrawRectangleRec(opaque_r, opaque);

        opaque_r.x += opaque_r.width;
        opaque_r.y += opaque_r.height;
        DrawRectangleRec(opaque_r, opaque);

        if (clickable_region(colorpad)) {
            *selected = i;
        }
        if (*selected == i) {
            DrawRectangleLinesEx(colorpad, MARK_LINE_THICK, BLACK);
        }
    }
}

void edit_colors() {
    int selected = -1;
    bool should_exit = false;

    while (!should_exit) {

        BeginDrawing();
        ClearBackground(BACKGROUND);

        Rectangle main_region = setup_screen("Editing Color Palette");
        RectTuple main_split = vsplit(main_region, 3, 2);

        color_selector(main_split.r1, &selected);

        RectTuple edit_split = chop_bottom(main_split.r2, BUTTON_HEIGHT);

        if (selected >= 0) {
            color_sliders(&COLORS[selected], edit_split.r1);
        }

        RectTuple button_split = vsplit(edit_split.r2, 1, 1);

        should_exit = button("exit", button_split.r1, BUTTON_COLOR);

        if (button("save", button_split.r2, BUTTON_COLOR)) {
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
    bool should_exit = false;
    Entry *entry = &GLOB_SPRITES.items[idx];
    int color = -1;
    while (!should_exit) {
        BeginDrawing();
        ClearBackground(BACKGROUND);
        Rectangle main =
            setup_screen(TextFormat("Edit Sprite: %s", entry->name));
        RectTuple main_split = vsplit(main, 3, 2);

        Rectangle sprite_rect = fit_square_factor(main_split.r1, 16);
        int pixel_scale = sprite_rect.width / 16;
        sprite_draw(entry->sprite, pixel_scale, sprite_rect.x, sprite_rect.y);
        for (int i = 0; i < SPRITE_SIZE * SPRITE_SIZE; i++) {
            int x = i % SPRITE_SIZE;
            int y = i / SPRITE_SIZE;
            Rectangle region = {
                .x = sprite_rect.x + x * pixel_scale,
                .y = sprite_rect.y + y * pixel_scale,
                .width = pixel_scale,
                .height = pixel_scale,
            };
            if (color != -1 && pixel(region, COLORS[color])) {
                unsigned char double_pixel =
                    ((unsigned char *)entry->sprite)[i / 2];
                if (i % 2 == 0) {
                    double_pixel &= 0xF0;
                    double_pixel += (unsigned char)color;
                } else {
                    double_pixel &= 0x0F;
                    double_pixel += (unsigned char)color << 4;
                }
                ((unsigned char *)entry->sprite)[i / 2] = double_pixel;
            }
        }

        RectTuple edit_split = chop_bottom(main_split.r2, BUTTON_HEIGHT);

        color_selector(edit_split.r1, &color);

        RectTuple button_split = vsplit(edit_split.r2, 1, 1);
        should_exit = button("exit", button_split.r1, BUTTON_COLOR);

        if (button("save", button_split.r2, BUTTON_COLOR)) {
            // TODO:
        }
        EndDrawing();
    }
    return;
}

bool sprite(Rectangle rect, int sprite) {
    Rectangle sprite_region = {
        .x = rect.x + LITTLE_MARGIN / 2,
        .y = rect.y + LITTLE_MARGIN / 2,
        .width = rect.width - LITTLE_MARGIN,
        .height = rect.width - LITTLE_MARGIN,
    };
    sprite_region = fit_square_factor(sprite_region, 16);
    sprite_draw(GLOB_SPRITES.items[sprite].sprite, sprite_region.width / 16,
                sprite_region.x, sprite_region.y);

    DrawText(GLOB_SPRITES.items[sprite].name, rect.x + LITTLE_MARGIN * 3 / 2,
             rect.y + rect.width - LITTLE_MARGIN / 2, SMALL_FONT, TEXT_COLOR);
    return clickable_region(rect);
}

int sprite_selector(Rectangle rect, int *page, int *num_pages) {
    int sprite_to_edit = -1;
    int row_len = ceil(rect.width / (16 * MAX_PIXE_SCALE + LITTLE_MARGIN));
    float width = rect.width / row_len;
    float height = width + LITTLE_MARGIN + SMALL_FONT;
    int row_count = floor(rect.height / height);
    height = rect.height / row_count;

    *num_pages = ceil((float)GLOB_SPRITES.count / (row_count * row_len));

    if (page > num_pages) {
        *page = 0;
    }

    int offset = *page * row_len * row_count;

    for (int i = offset; i < GLOB_SPRITES.count && i < row_count * row_len;
         i++) {
        int x = (i - offset) % row_len;
        int y = (i - offset) / row_len;

        Rectangle region = {
            .x = rect.x + x * width,
            .y = rect.y + y * height,
            .width = width,
            .height = height,
        };
        if (sprite(region, i)) {
            sprite_to_edit = i;
        }
    }
    return sprite_to_edit;
}

int main() {
    InitWindow(WIDTH, HEIGHT, "Spredit");
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    if (load_colors() != 0) {
        return 1;
    }

    if (load_sprites() != 0) {
        return 1;
    }

    bool should_quit = false;
    int page = 0;
    int num_pages = 0;
    while (!should_quit) {
        BeginDrawing();
        ClearBackground(BACKGROUND);

        Rectangle main_region = setup_screen("Spredit");

        RectTuple main_split = vsplit(main_region, 2, 5);

        char *buttons[] = {
            "Edit Palette",
            "New Sprite",
            "Quit",
        };

        int result = button_list(&main_split.r1, buttons, 3);

        DrawText(TextFormat("%d Sprites\nPage %d/%d", GLOB_SPRITES.count,
                            page + 1, num_pages),
                 main_split.r1.x,
                 main_split.r1.y + main_split.r1.height - 2 * MEDIUM_FONT,
                 MEDIUM_FONT, TEXT_COLOR);

        int sprite_to_edit = sprite_selector(main_split.r2, &page, &num_pages);

        EndDrawing();

        switch (result) {
        case 0:
            edit_colors();
            break;
        case 1:
            unsigned char *ptr =
                malloc(sizeof(char) * SPRITE_SIZE * SPRITE_SIZE / 2);
            for (int i = 0; i < SPRITE_SIZE * SPRITE_SIZE / 2; i++) {
                ptr[i] = 0;
            }

            if (ptr == NULL) {
                TraceLog(LOG_FATAL, "could not allocate new sprite");
                return 1;
            }
            Entry entry = {.name = {0}, .sprite = ptr};
            nob_da_append(&GLOB_SPRITES, entry);
            edit_sprite(GLOB_SPRITES.count - 1);
            break;
        case 2:
            should_quit = true;
            break;
        }

        if (sprite_to_edit != -1) {
            edit_sprite(sprite_to_edit);
        }
    }
    CloseWindow();
}
