#include <math.h>
#include <stdlib.h>
#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
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

const int POPUP_WIDTH = 400;
const int POPUP_HEIGHT = 150;

const Color BACKGROUND = COLOR(0x222222FF);
const Color POPUP_BACKGROUND = COLOR(0x333333FF);
const Color TEXT_COLOR = WHITE;

const float MARGINS = 30;
const int TITLE_BAR = 60;

const float LARGE_FONT = 50;
const float MEDIUM_FONT = 30;
const float SMALL_FONT = 20;

const float LITTLE_MARGIN = 10;

const float TEXT_BOX_FACTOR = 1.2;

const int BUTTON_HEIGHT = MEDIUM_FONT * TEXT_BOX_FACTOR;

const Color BUTTON_COLOR = GRAY;

const int MARK_LINE_THICK = 5;

const int MAX_PIXEL_SCALE = 15;

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

Rectangle get_popup_rect() {
    return (Rectangle){
        .x = (float)(GetScreenWidth() - POPUP_WIDTH) / 2,
        .y = (float)(GetScreenHeight() - POPUP_HEIGHT) / 2,
        .width = POPUP_WIDTH,
        .height = POPUP_HEIGHT,
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

Rectangle setup_popup(const char *text) {
    Rectangle full = get_popup_rect();
    DrawRectangleRec(full, POPUP_BACKGROUND);
    Rectangle inner = shrink(full, MARGINS);
    DrawText(text, inner.x, inner.y, SMALL_FONT, TEXT_COLOR);
    return inner;
}

int button_list_popup(const char *text, int count, char *buttons[],
                      int default_val) {
    bool done = false;
    int result = -1;
    while (!done) {
        BeginDrawing();
        Rectangle rect = setup_popup(text);
        RectTuple split = chop_bottom(rect, SMALL_FONT);
        for (int i = 0; i < count; i++) {
            Rectangle b_view = hsubdivide(split.r2, count, i);
            if (button(buttons[i], b_view, BUTTON_COLOR)) {
                result = i;
                done = true;
            }
        }
        EndDrawing();
        if (default_val >= 0 && IsKeyPressed(KEY_ENTER)) {
            result = default_val;
            done = true;
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            done = true;
        }
    }
    return result;
}

char *string_popup(const char *text, const char *initial_text, int max_len) {
    bool done = false;
    char *input = malloc(max_len);
    strcpy(input, initial_text);
    int letter_count = strlen(input);
    while (!done) {
        BeginDrawing();
        Rectangle rect = setup_popup(text);
        RectTuple split = chop_bottom(rect, LITTLE_MARGIN * 2 + SMALL_FONT);
        DrawRectangleRec(split.r2, WHITE);
        Rectangle text_field = shrink(split.r2, LITTLE_MARGIN);
        DrawText(input, text_field.x, text_field.y, SMALL_FONT, BLACK);

        EndDrawing();
        int key = GetCharPressed();

        while (key > 0) {
            // TODO: Unicode input
            if ((key >= 32) && (key <= 125) && (letter_count < max_len - 1)) {
                input[letter_count] = (char)key;
                letter_count++;
                input[letter_count] = '\0';
            }
            key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE) && letter_count > 0) {
            letter_count--;
            input[letter_count] = '\0';
        }
        if (IsKeyPressed(KEY_ENTER)) {
            if (letter_count == 0) {
                free(input);
                input = NULL;
            }
            done = true;
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            free(input);
            input = NULL;
            done = true;
        }
    }
    return input;
}

typedef struct {
    char *name;
    unsigned char *pixels;
} Sprite;

typedef struct {
    Sprite *items;
    int count;
    int capacity;
} SpriteList;

const int MAX_NAME_LEN = 64;
enum { SPRITE_SIZE = 16 };

// half a byte
enum { NUM_COLORS = 16 };

Color COLORS[NUM_COLORS] = {0};
Color NEW_COLORS[NUM_COLORS] = {0};
Color *DISPLAYCOLORS = &COLORS[0];

SpriteList GLOB_SPRITES = {0};

unsigned char EDIT_BUF[SPRITE_SIZE * SPRITE_SIZE / 2] = {0};

int load_file(const char *path) {
    FILE *file = fopen(path, "rb");
    int result = 0;
    if (!file) {
        result = -1;
        TraceLog(LOG_ERROR, "Error opening file: %s", path);
        goto cleanup;
    }
    char magic[5] = {0};
    if (fread(&magic, 1, 4, file) != 4) {
        result = -1;
        TraceLog(LOG_ERROR, "Error reading: %s", path);
        goto cleanup;
    }
    if (strcmp(magic, "sprt") != 0) {
        result = -1;
        TraceLog(LOG_ERROR, "%s is not a sprite file", path);
        goto cleanup;
    }
    uint32_t r_count = 0;
    if (fread(&r_count, sizeof(uint32_t), 1, file) != 1) {
        result = -1;
        TraceLog(LOG_ERROR, "Error reading: %s", path);
        goto cleanup;
    }
    int count = r_count;
    if (fread(&COLORS, sizeof(Color), NUM_COLORS, file) != NUM_COLORS) {
        result = -1;
        TraceLog(LOG_ERROR, "Error reading: %s", path);
        goto cleanup;
    }
    da_reserve(&GLOB_SPRITES, count);
    for (int i = 0; i < count; i++) {
        char *name = malloc(MAX_NAME_LEN);
        if (fread(name, 1, MAX_NAME_LEN, file) != MAX_NAME_LEN) {
            free(name);
            result = -1;
            TraceLog(LOG_ERROR, "Error reading: %s", path);
            goto cleanup;
        }
        if (realloc(name, strlen(name) + 1) == NULL) {
            free(name);
            result = -1;
            TraceLog(LOG_ERROR,
                     "Error reading: %s\ncould not shrink string buffer", path);
            goto cleanup;
        }

        unsigned char *pixels = malloc(SPRITE_SIZE * SPRITE_SIZE / 2);
        if (fread(pixels, 1, SPRITE_SIZE * SPRITE_SIZE / 2, file) !=
            SPRITE_SIZE * SPRITE_SIZE / 2) {
            free(name);
            free(pixels);
            result = -1;
            TraceLog(LOG_ERROR, "Error reading: %s", path);
            goto cleanup;
        }
        Sprite sprite = {.name = name, .pixels = pixels};
        da_append(&GLOB_SPRITES, sprite);
    }

    memcpy(&NEW_COLORS, &COLORS, NUM_COLORS * sizeof(Color));

cleanup:
    if (file) {
        if (fclose(file) != 0) {
            TraceLog(LOG_ERROR, "Error closing file: %s", path);
            if (result == 0) {
                result = -1;
            }
        }
    }

    return result;
}

void unload_sprites() {
    da_foreach(Sprite, s, &GLOB_SPRITES) {
        free(s->pixels);
        free(s->pixels);
    }
    da_free(GLOB_SPRITES);
    GLOB_SPRITES = (SpriteList){0};
}

int write_file(const char *path) {
    FILE *file = fopen(path, "wb");
    int result = 0;
    if (!file) {
        TraceLog(LOG_ERROR, "Error creating file %s", path);
        result = -1;
        goto cleanup;
    }
    if (fprintf(file, "sprt") != 4) {
        TraceLog(LOG_ERROR, "Error writing file: %s");
        result = -1;
        goto cleanup;
    }
    uint32_t count = GLOB_SPRITES.count;
    if (fwrite(&count, sizeof(uint32_t), 1, file) != 1) {
        TraceLog(LOG_ERROR, "Error writing file: %s");
        result = -1;
        goto cleanup;
    }

    if (fwrite(&COLORS, sizeof(Color), NUM_COLORS, file) != NUM_COLORS) {
        TraceLog(LOG_ERROR, "Error writing file: %s");
        result = -1;
        goto cleanup;
    }
    for (int i = 0; i < GLOB_SPRITES.count; i++) {
        Sprite sprite = GLOB_SPRITES.items[i];
        size_t name_len = strlen(sprite.name);
        size_t padding_len = MAX_NAME_LEN - name_len;
        size_t written_content = fwrite(sprite.name, 1, name_len, file);
        if (written_content != name_len) {
            TraceLog(LOG_ERROR, "Error writing file: %s");
            result = -1;
            goto cleanup;
        }
        if (padding_len > 0) {
            char zero_byte = 0;
            for (size_t i = 0; i < padding_len; i++) {
                if (fwrite(&zero_byte, 1, 1, file) != 1) {
                    TraceLog(LOG_ERROR, "Error writing file: %s");
                    result = -1;
                    goto cleanup;
                }
            }
        }
        size_t bytes_sent =
            fwrite(sprite.pixels, 1, SPRITE_SIZE * SPRITE_SIZE / 2, file);
        if (bytes_sent != SPRITE_SIZE * SPRITE_SIZE / 2) {
            TraceLog(LOG_ERROR, "Error writing file: %s");
            result = -1;
            goto cleanup;
        }
    }

cleanup:
    if (file) {
        if (fclose(file) != 0) {
            TraceLog(LOG_ERROR, "Error closing file: %s", path);
            if (result == 0)
                result = -1;
        }
    }

    return result;
}

void draw_sprite(unsigned char *sprite, int pixel_width, int left, int top) {
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
                      pixel_width, pixel_width, DISPLAYCOLORS[color_idx]);
    }
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

void color_selector(Rectangle rect, int *selected, Color *colors) {
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
        DrawRectangleRec(colorpad, colors[i]);

        Color opaque = colors[i];
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

        color_selector(main_split.r1, &selected, (Color *)&NEW_COLORS);

        RectTuple edit_split = chop_bottom(main_split.r2, BUTTON_HEIGHT);

        if (selected >= 0) {
            color_sliders(&NEW_COLORS[selected], edit_split.r1);
        }

        RectTuple button_split = vsplit(edit_split.r2, 1, 1);

        should_exit = button("exit", button_split.r1, BUTTON_COLOR);

        if (button("save", button_split.r2, BUTTON_COLOR)) {
            memcpy(&COLORS, &NEW_COLORS, sizeof(Color) * NUM_COLORS);
        }
        EndDrawing();
    }
    return;
}

void edit_sprite(int idx) {
    memcpy(&EDIT_BUF, GLOB_SPRITES.items[idx].pixels,
           SPRITE_SIZE * SPRITE_SIZE / 2);
    bool was_changed = false;
    char *name = GLOB_SPRITES.items[idx].name;
    int color = -1;
start:
    bool should_exit = false;
    while (!should_exit) {
        BeginDrawing();
        ClearBackground(BACKGROUND);
        Rectangle main = setup_screen(TextFormat("Edit Sprite: %s", name));
        RectTuple main_split = vsplit(main, 3, 2);

        Rectangle sprite_rect = fit_square_factor(main_split.r1, 16);
        int pixel_scale = sprite_rect.width / 16;
        draw_sprite((unsigned char *)&EDIT_BUF, pixel_scale, sprite_rect.x,
                    sprite_rect.y);
        for (int i = 0; i < SPRITE_SIZE * SPRITE_SIZE; i++) {
            int x = i % SPRITE_SIZE;
            int y = i / SPRITE_SIZE;
            Rectangle region = {
                .x = sprite_rect.x + x * pixel_scale,
                .y = sprite_rect.y + y * pixel_scale,
                .width = pixel_scale,
                .height = pixel_scale,
            };
            if (color != -1 && pixel(region, DISPLAYCOLORS[color])) {
                was_changed = true;
                unsigned char double_pixel = EDIT_BUF[i / 2];
                if (i % 2 == 0) {
                    double_pixel &= 0xF0;
                    double_pixel += (unsigned char)color;
                } else {
                    double_pixel &= 0x0F;
                    double_pixel += (unsigned char)color << 4;
                }
                EDIT_BUF[i / 2] = double_pixel;
            }
        }

        RectTuple edit_split = chop_bottom(main_split.r2, BUTTON_HEIGHT);
        color_selector(edit_split.r1, &color, DISPLAYCOLORS);

        RectTuple buttons = vsplit(edit_split.r2, 1, 1);

        if (button("save", buttons.r1, BUTTON_COLOR)) {
            memcpy(GLOB_SPRITES.items[idx].pixels, &EDIT_BUF,
                   SPRITE_SIZE * SPRITE_SIZE / 2);
            was_changed = false;
        }
        if (button("exit", buttons.r2, BUTTON_COLOR)) {
            should_exit = true;
        }
        EndDrawing();
    }

    if (was_changed) {
        char *opts[] = {
            "Discard",
            "Keep",
        };

        switch (button_list_popup("Discard Changes?", 2, opts, 1)) {
        case -1:
            goto start;
        case 1:
            memcpy(GLOB_SPRITES.items[idx].pixels, &EDIT_BUF,
                   SPRITE_SIZE * SPRITE_SIZE / 2);

        case 0:
        }
    }
    return;
}

void edit_new() {
    unsigned char *ptr = malloc(sizeof(char) * SPRITE_SIZE * SPRITE_SIZE / 2);
    for (int i = 0; i < SPRITE_SIZE * SPRITE_SIZE / 2; i++) {
        ptr[i] = 0;
    }

    if (ptr == NULL) {
        TraceLog(LOG_FATAL, "could not allocate new sprite");
        // TODO:
    }
    char *name = string_popup("Enter sprite name:", "", MAX_NAME_LEN);
    if (name == NULL) {
        return;
    }
    Sprite entry = {
        .name = name,
        .pixels = ptr,
    };
    da_append(&GLOB_SPRITES, entry);
    edit_sprite(GLOB_SPRITES.count - 1);
}

bool sprite(Rectangle rect, int sprite) {
    Rectangle sprite_region = {
        .x = rect.x + LITTLE_MARGIN / 2,
        .y = rect.y + LITTLE_MARGIN / 2,
        .width = rect.width - LITTLE_MARGIN,
        .height = rect.width - LITTLE_MARGIN,
    };
    sprite_region = fit_square_factor(sprite_region, 16);
    draw_sprite(GLOB_SPRITES.items[sprite].pixels, sprite_region.width / 16,
                sprite_region.x, sprite_region.y);

    DrawText(GLOB_SPRITES.items[sprite].name, rect.x + LITTLE_MARGIN * 3 / 2,
             rect.y + rect.width - LITTLE_MARGIN / 2, SMALL_FONT, TEXT_COLOR);
    return clickable_region(rect);
}

int sprite_selector(Rectangle rect, int *page, int *num_pages) {
    int sprite_to_edit = -1;
    int row_len = ceil(rect.width / (16 * MAX_PIXEL_SCALE + LITTLE_MARGIN));
    float width = rect.width / row_len;
    float height = width + LITTLE_MARGIN + SMALL_FONT;
    int row_count = floor(rect.height / height);
    height = rect.height / row_count;

    *num_pages = ceil((float)GLOB_SPRITES.count / (row_count * row_len));

    if (*page >= *num_pages) {
        *page = 0;
    }
    if (*page < 0) {
        *page = *num_pages - 1;
    }

    int offset = *page * row_len * row_count;

    for (int i = offset;
         i < GLOB_SPRITES.count && i < offset + row_count * row_len; i++) {
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

int main(int argc, char *argv[]) {
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(WIDTH, HEIGHT, "Spredit");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(100);
    // just to disable close on esc
    SetExitKey(KEY_F10);

    char *file_name = 0;

    switch (argc) {
    case 1:
        break;
    case 2:
        file_name = argv[1];
        load_file(argv[1]);
        break;
    case 3:
        TraceLog(LOG_ERROR, "invalid arguments");
        return 1;
    }

    bool should_quit = false;
    int page = 0;
    int num_pages = 0;
    while (!should_quit) {
        should_quit = WindowShouldClose();
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_DOWN)) {
            page += 1;
        }
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_UP)) {
            page -= 1;
        }
        BeginDrawing();
        ClearBackground(BACKGROUND);

        Rectangle main_region = setup_screen("Spredit");

        RectTuple main_split = vsplit(main_region, 2, 5);

        char *buttons[] = {
            "Edit Palette", "New Sprite", "Save Changes", "Save As", "Quit",
        };

        int result = button_list(&main_split.r1, buttons, 5);

        DrawText(TextFormat("%d Sprites\nPage %d/%d", GLOB_SPRITES.count,
                            page + 1, num_pages),
                 main_split.r1.x,
                 main_split.r1.y + main_split.r1.height - 2 * MEDIUM_FONT,
                 MEDIUM_FONT, TEXT_COLOR);

        if (file_name) {
            DrawText(TextFormat("File: %s", file_name), main_split.r1.x,
                     main_split.r1.y + main_split.r1.height - 3 * MEDIUM_FONT,
                     MEDIUM_FONT, TEXT_COLOR);
        }

        int sprite_to_edit = sprite_selector(main_split.r2, &page, &num_pages);

        EndDrawing();

        switch (result) {
        case 0:
            edit_colors();
            break;
        case 1:
            edit_new();
            break;
        case 3:
            file_name = NULL;
        case 2:
            if (file_name == NULL) {
                file_name = string_popup("Enter file name", "", 64);
            }
            if (file_name != NULL && write_file(file_name) != 0) {
                return 1;
            }
            break;
        case 4:
            should_quit = true;
            break;
        }

        if (sprite_to_edit != -1) {
            edit_sprite(sprite_to_edit);
        }
    }
    CloseWindow();
    unload_sprites();
}
