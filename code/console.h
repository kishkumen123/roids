#ifndef CONSOLE_H
#define CONSOLE_H

typedef enum ConsoleState{
    CLOSED,
    OPEN,
    OPEN_BIG,
} ConsoleState;


global Rect output_rect;
global Rect input_rect;
global Rect cursor_rect;
global v2 history_pos;

//global String8 output_font_name = str8_literal("Roboto-Regular.ttf");
global String8 input_font_name = str8_literal("GolosText-Regular.ttf");
global String8 output_font_name = str8_literal("Inconsolata-Regular.ttf");

u32 input_font_size;
u32 output_font_size;

global Font output_font;
global Font input_font;

global RGBA output_color;
global RGBA input_color;
global RGBA cursor_color;

global ConsoleState console_state = OPEN_BIG;

global f32 console_t  = 0.0f;
global f32 y_closed   = 1.0f;
global f32 y_open     = .7f;
global f32 y_open_big = .2f;
global f32 open_speed = 0.5f;

global f32 input_height  = 28;
global f32 cursor_height = 24;
global f32 cursor_width  = 10;
global f32 cursor_vertical_padding = 2;

#define INPUT_MAX_COUNT KB(4)
global u8 console_input[INPUT_MAX_COUNT] = {0};
global u32 input_char_count = 0;

static String8 console_history[KB(1)] = {0};
static u32 history_count = 0;
static u32 history_index = 0;

static void
input_add_char(u8 c){
    assert(input_char_count < INPUT_MAX_COUNT);

    console_input[input_char_count++] = c;

    Glyph glyph = input_font.glyphs[c];
    cursor_rect.x0 += (glyph.advance_width * input_font.scale);
    cursor_rect.x1 += (glyph.advance_width * input_font.scale);
}

static void
input_remove_char(){
    if(input_char_count > 0){
        u8 c = console_input[--input_char_count];
        Glyph glyph = input_font.glyphs[c];
        cursor_rect.x0 -= (glyph.advance_width * input_font.scale);
        cursor_rect.x1 -= (glyph.advance_width * input_font.scale);
    }
}

static void
init_console(PermanentMemory* pm){
    f32 x0 = 0;
    f32 x1 = (f32)resolution.w;
    f32 y0 = (f32)resolution.h;
    f32 y1 = (f32)resolution.h;
    output_rect = make_rect(x0, y0, x1, y1);
    input_rect  = make_rect(x0, y0, x1, y1 + input_height);
    cursor_rect = make_rect(x0 + 10, y0 + cursor_vertical_padding, x0 + 10 + cursor_width, y0 + cursor_height);
    history_pos = make_v2(x0 + 30, y0 + 30);

    output_color = {1/255.0f, 57/255.0f, 90/255.0f, 1.0f};
    input_color = {0/255.0f, 44/255.0f, 47/255.0f, 1.0f};
    cursor_color = {125/255.0f, 125/255.0f, 125/255.0f, 1.0f};

    input_char_count = 0;

    input_font_size = 24;
    output_font_size = 24;
    bool succeed;
    succeed = load_font_ttf(&pm->arena, pm->fonts_dir, input_font_name, &input_font);
    assert(succeed);
    load_font_glyphs(&pm->arena, input_font_size, ORANGE, &input_font);

    succeed = load_font_ttf(&pm->arena, pm->fonts_dir, output_font_name, &output_font);
    assert(succeed);
    load_font_glyphs(&pm->arena, output_font_size, ORANGE, &output_font);
}

static bool
console_is_open(){
    return(console_state != CLOSED);
}

static bool
console_is_visable(){
    return(output_rect.y0 < resolution.h);
}

static void
console_set_state(ConsoleState s){
    console_state = s;
}


// maybe draw_console should just draw? What is imm in this case
static void
push_console(Arena* command_arena){
    push_rect(command_arena, output_rect, output_color);
    push_rect(command_arena, input_rect, input_color);
    push_rect(command_arena, cursor_rect, cursor_color);

    if(input_char_count > 0){
        String8 input_str = str8(console_input, input_char_count);
        push_text(command_arena, make_v2(input_rect.x0 + 10, input_rect.y0 + 6), &input_font, input_str);
    }

    v2 unscaled_offset = {0, 0};
    for(s32 i=history_index; i >= 0; --i){
        if(history_count > 0){
            String8 next_string = console_history[i];
            v2 new_pos = make_v2(history_pos.x, history_pos.y + (unscaled_offset.y * output_font.scale));
            push_text(command_arena, new_pos, &output_font, next_string);
            unscaled_offset.y += output_font.vertical_offset;
        }
    }
    //push_text(command_arena, String8 console_input_string);
}

// CONSIDER, INCOMPLETE: Maybe we don't want to enlarge the rect, and simple move it down and up as a solid piece?
static void
update_console(){
    f32 lerp_speed =  open_speed * (f32)clock.dt;
    if(console_state == CLOSED){
        if(console_t < 1) {
            console_t += lerp_speed;
            f32 output_rect_bottom = y_closed * (f32)resolution.h;

            output_rect.y0 = lerp(output_rect.y0, output_rect_bottom, (console_t));

            input_rect.y0 = output_rect.y0;
            input_rect.y1 = output_rect.y0 + input_height;

            cursor_rect.y0 = output_rect.y0 + cursor_vertical_padding;
            cursor_rect.y1 = output_rect.y0 + cursor_height;

            history_pos.y = output_rect.y0 + 10;
        }
    }
    else if(console_state == OPEN){
        if(console_t < 1) {
            console_t += lerp_speed;
            f32 output_rect_bottom = y_open * (f32)resolution.h;

            output_rect.y0 = lerp(output_rect.y0, output_rect_bottom, (console_t));

            input_rect.y0 = output_rect.y0;
            input_rect.y1 = output_rect.y0 + input_height;

            cursor_rect.y0 = output_rect.y0 + cursor_vertical_padding;
            cursor_rect.y1 = output_rect.y0 + cursor_height;

            history_pos.y = output_rect.y0 + 10;
        }
    }
    else if(console_state == OPEN_BIG){
        if(console_t < 1) {
            console_t += lerp_speed;
            f32 output_rect_bottom = y_open_big * (f32)resolution.h;

            output_rect.y0 = lerp(output_rect.y0, output_rect_bottom, (console_t));

            input_rect.y0 = output_rect.y0;
            input_rect.y1 = output_rect.y0 + input_height;

            cursor_rect.y0 = output_rect.y0 + cursor_vertical_padding;
            cursor_rect.y1 = output_rect.y0 + cursor_height;

            history_pos.y = output_rect.y0 + 10;
        }
    }
    //s32 x_offset = get_cursor_offset(input_str, &font_incon);
    //cursor_rect.x0 += x_offset;
    //cursor_rect.x1 += x_offset;
}

#endif
