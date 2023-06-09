#ifndef CONSOLE_H
#define CONSOLE_H

#include "command.h"

typedef enum ConsoleState{
    CLOSED,
    OPEN,
    OPEN_BIG,
} ConsoleState;

#define INPUT_MAX_COUNT KB(4)
#define CONSOLE_HISTORY_MAX KB(1)
#define COMMAND_HISTORY_MAX KB(1)
typedef struct Console{
    ConsoleState state;

    Rect output_rect;
    Rect input_rect;
    Rect cursor_rect;
    v2 history_pos;

    RGBA output_background_color;
    RGBA input_background_color;
    RGBA cursor_color;
    u32  cursor_index;

    // TODO:INCOMPLETE:why not do String8 here?
    u8 input[INPUT_MAX_COUNT];
    u32 input_char_count;

    String8 output_history[CONSOLE_HISTORY_MAX];
    u32 output_history_count;
    u32 output_history_at;

    String8 command_history[COMMAND_HISTORY_MAX];
    u32 command_history_count;
    u32 command_history_at;

    Font output_font;
    Font input_font;
    Font command_history_font;
} Console;
global Console console;

// some size constraints
global f32 input_height  = 28;
global f32 cursor_height = 24;
global f32 cursor_width  = 10;
global f32 cursor_vertical_padding = 2;

// how much/fast to open
global f32 console_speed = 0.5f;
global f32 console_t  = 0.0f;
global f32 y_closed   = 1.0f;
global f32 y_open     = .7f;
global f32 y_open_big = .2f;

static void
init_console(PermanentMemory* pm){
    // everything is positioned relative to the output_rect
    console.state = CLOSED;
    f32 x0 = 0;
    f32 x1 = (f32)resolution.w;
    f32 y0 = (f32)resolution.h;
    f32 y1 = (f32)resolution.h;
    console.output_rect = make_rect(x0, y0, x1, y1);
    console.input_rect  = make_rect(x0, y0, x1, y1 + input_height);
    console.cursor_rect = make_rect(x0 + 10, y0 + cursor_vertical_padding, x0 + 10 + cursor_width, y0 + cursor_height);
    console.history_pos = make_v2(x0 + 30, y0 + 40);

    // some colors
    console.output_background_color = {1/255.0f, 57/255.0f, 90/255.0f, 1.0f};
    console.input_background_color = {0/255.0f, 44/255.0f, 47/255.0f, 1.0f};
    console.cursor_color = {125/255.0f, 125/255.0f, 125/255.0f, 1.0f};

    // init and load fonts
    console.input_font.name = str8_literal("\\GolosText-Regular.ttf");
    console.input_font.size = 24;
    console.input_font.color = TEAL;

    console.output_font.name = str8_literal("\\Inconsolata-Regular.ttf");
    console.output_font.size = 24;
    console.output_font.color = ORANGE;

    console.command_history_font.name = str8_literal("\\GolosText-Regular.ttf");
    console.command_history_font.size = 24;
    console.command_history_font.color = LIGHT_GRAY;

    bool succeed;
    succeed = load_font_ttf(&pm->arena, pm->fonts_dir, &console.input_font);
    assert(succeed);
    load_font_glyphs(&pm->arena, &console.input_font);

    succeed = load_font_ttf(&pm->arena, pm->fonts_dir, &console.output_font);
    assert(succeed);
    load_font_glyphs(&pm->arena, &console.output_font);

    succeed = load_font_ttf(&pm->arena, pm->fonts_dir, &console.command_history_font);
    assert(succeed);
    load_font_glyphs(&pm->arena, &console.command_history_font);
}

static bool
console_is_open(){
    return(console.state != CLOSED);
}

static bool
console_is_visible(){
    return(console.output_rect.y0 < (f32)resolution.h);
}

static void
console_cursor_reset(){
    console.cursor_rect.x0 = console.output_rect.x0 + 10;
    console.cursor_rect.x1 = console.output_rect.x0 + 10 + cursor_width;
    console.cursor_index = 0;
}

static void
console_clear_input(){
    console.input_char_count = 0;
}

static void
input_add_char(u8 c){
    if(console.input_char_count < INPUT_MAX_COUNT){
        if(console.cursor_index < console.input_char_count){
            ScratchArena scratch = begin_scratch(0);
            defer(end_scratch(scratch));

            String8 left = {
                .str = push_array(scratch.arena, u8, console.cursor_index),
                .size = console.cursor_index,
            };
            mem_copy(left.str, console.input, left.size);
            String8 right = {
                .str = push_array(scratch.arena, u8, console.input_char_count - console.cursor_index),
                .size = console.input_char_count - console.cursor_index,
            };
            mem_copy(right.str, console.input + console.cursor_index, right.size);

            u32 index = 0;
            for(u32 i=0; i < left.size; ++i){
                console.input[index++] = left.str[i];
            }
            console.input[index++] = c;
            for(u32 i=0; i < right.size; ++i){
                console.input[index++] = right.str[i];
            }

            console.cursor_index++;
            console.input_char_count++;
            Glyph glyph = console.input_font.glyphs[c];
            console.cursor_rect.x0 += ((f32)glyph.advance_width * console.input_font.scale);
            console.cursor_rect.x1 += ((f32)glyph.advance_width * console.input_font.scale);
        }
        else{
            console.input[console.input_char_count++] = c;
            console.cursor_index++;

            Glyph glyph = console.input_font.glyphs[c];
            console.cursor_rect.x0 += ((f32)glyph.advance_width * console.input_font.scale);
            console.cursor_rect.x1 += ((f32)glyph.advance_width * console.input_font.scale);
        }
    }
}

static void
input_remove_char(){
    if(console.input_char_count > 0 && console.cursor_index > 0){
        if(console.cursor_index < console.input_char_count){
            ScratchArena scratch = begin_scratch(0);
            defer(end_scratch(scratch));

            String8 left = {
                .str = push_array(scratch.arena, u8, console.cursor_index-1),
                .size = console.cursor_index-1,
            };
            mem_copy(left.str, console.input, left.size);
            String8 right = {
                .str = push_array(scratch.arena, u8, console.input_char_count - console.cursor_index),
                .size = console.input_char_count - console.cursor_index,
            };
            mem_copy(right.str, console.input + console.cursor_index, right.size);

            console.input_char_count--;
            u8 c = console.input[--console.cursor_index];
            u32 index = 0;
            for(u32 i=0; i < left.size; ++i){
                console.input[index++] = left.str[i];
            }
            for(u32 i=0; i < right.size; ++i){
                console.input[index++] = right.str[i];
            }

            Glyph glyph = console.input_font.glyphs[c];
            console.cursor_rect.x0 -= ((f32)glyph.advance_width * console.input_font.scale);
            console.cursor_rect.x1 -= ((f32)glyph.advance_width * console.input_font.scale);
        }
        else{
            u8 c = console.input[--console.input_char_count];
            console.cursor_index--;

            Glyph glyph = console.input_font.glyphs[c];
            console.cursor_rect.x0 -= ((f32)glyph.advance_width * console.input_font.scale);
            console.cursor_rect.x1 -= ((f32)glyph.advance_width * console.input_font.scale);
        }
    }
}

static void
console_store_output(String8 str){
    if(console.output_history_count < CONSOLE_HISTORY_MAX){
        console.output_history[console.output_history_count] = str;
        console.output_history_count++;
        console.input_char_count = 0;
    }
}

static void
console_store_command(String8 str){
    if(console.command_history_count < COMMAND_HISTORY_MAX){
        console.command_history[console.command_history_count] = str;
        console.command_history_count++;
        console.input_char_count = 0;
    }
}

static void
push_console(Arena* command_arena){
    if(console_is_visible()){
        // push console rects
        push_rect(command_arena, console.output_rect, console.output_background_color);
        push_rect(command_arena, console.input_rect, console.input_background_color);
        push_rect(command_arena, console.cursor_rect, console.cursor_color);

        // push input string
        if(console.input_char_count > 0){
            String8 input_str = str8(console.input, console.input_char_count);
            push_text(command_arena, make_v2(console.input_rect.x0 + 10, console.input_rect.y0 + 6), &console.input_font, input_str);
            //if(console.command_history_at > 0){
            //    String8 input_str = str8(console.input, console.input_char_count);
            //    push_text(command_arena, make_v2(console.input_rect.x0 + 10, console.input_rect.y0 + 6), &console.command_history_font, input_str);
            //}
            //else{
            //    String8 input_str = str8(console.input, console.input_char_count);
            //    push_text(command_arena, make_v2(console.input_rect.x0 + 10, console.input_rect.y0 + 6), &console.input_font, input_str);
            //}
        }

        // push history in reverse order, but only if its on screen
        f32 unscaled_y_offset = 0.0f;
        for(u32 i=console.output_history_count-1; i < console.output_history_count; --i){
            if(console.history_pos.y + (unscaled_y_offset * console.output_font.scale) < (f32)resolution.h){
                String8 next_string = console.output_history[i];
                v2 new_pos = make_v2(console.history_pos.x, console.history_pos.y + (unscaled_y_offset * console.output_font.scale));
                push_text(command_arena, new_pos, &console.output_font, next_string);
                unscaled_y_offset += (f32)console.output_font.vertical_offset;
            }
        }
    }
}

static void
update_console(){
    // lerp to appropriate position based on state. Everything is positioned based on output_rect.
    f32 lerp_speed =  console_speed * (f32)clock.dt;
    f32 output_rect_bottom = 0;
    switch(console.state){
        case CLOSED:{
            output_rect_bottom = y_closed * (f32)resolution.h;
            break;
        }
        case OPEN:{
            output_rect_bottom = y_open * (f32)resolution.h;
            break;
        }
        case OPEN_BIG:{
            output_rect_bottom = y_open_big * (f32)resolution.h;
            break;
        }
    }

    if(console_t < 1) {
        console_t += lerp_speed;

        console.output_rect.y0 = lerp(console.output_rect.y0, output_rect_bottom, (console_t));

        console.input_rect.y0 = console.output_rect.y0;
        console.input_rect.y1 = console.output_rect.y0 + input_height;

        console.cursor_rect.y0 = console.output_rect.y0 + cursor_vertical_padding;
        console.cursor_rect.y1 = console.output_rect.y0 + cursor_height;

        console.history_pos.y = console.output_rect.y0 + 40;
    }
}

static bool
handle_console_event(Event event){
    if(event.type == TEXT_INPUT){
        if(event.keycode != '`' && event.keycode != '~'){
            input_add_char((u8)event.keycode);
            return(true);
        }
    }
    if(event.type == KEYBOARD){
        if(event.key_pressed){
            if(event.keycode == HOME){
                console_cursor_reset();
            }
            if(event.keycode == END){
                for(u32 i=console.cursor_index; i < console.input_char_count; ++i){
                    u8 c = console.input[i];
                    Glyph glyph = console.input_font.glyphs[c];
                    console.cursor_rect.x0 += ((f32)glyph.advance_width * console.input_font.scale);
                    console.cursor_rect.x1 += ((f32)glyph.advance_width * console.input_font.scale);
                    console.cursor_index++;
                }
            }
            if(event.keycode == ARROW_RIGHT){
                if(console.cursor_index < console.input_char_count){
                    u8 c = console.input[console.cursor_index];
                    Glyph glyph = console.input_font.glyphs[c];
                    console.cursor_rect.x0 += ((f32)glyph.advance_width * console.input_font.scale);
                    console.cursor_rect.x1 += ((f32)glyph.advance_width * console.input_font.scale);
                    console.cursor_index++;
                }
            }
            if(event.keycode == ARROW_LEFT){
                if(console.cursor_index > 0){
                    console.cursor_index--;
                    u8 c = console.input[console.cursor_index];
                    Glyph glyph = console.input_font.glyphs[c];
                    console.cursor_rect.x0 -= ((f32)glyph.advance_width * console.input_font.scale);
                    console.cursor_rect.x1 -= ((f32)glyph.advance_width * console.input_font.scale);
                }
            }
            if(event.keycode == ARROW_UP){
                if(console.command_history_at < console.command_history_count){
                    console_cursor_reset();
                    console_clear_input();
                    console.command_history_at++;
                    String8 command = console.command_history[console.command_history_count - console.command_history_at];
                    for(u32 i=0; i < command.size; ++i){
                        u8 c = command.str[i];
                        input_add_char(c);
                    }
                    console.input_char_count = (u32)command.size;
                }
            }
            if(event.keycode == ARROW_DOWN){
                if(console.command_history_at > 0){
                    console_cursor_reset();
                    console_clear_input();
                    console.command_history_at--;
                    String8 command = console.command_history[console.command_history_count - console.command_history_at];
                    for(u32 i=0; i < command.size; ++i){
                        u8 c = command.str[i];
                        input_add_char(c);
                    }
                    console.input_char_count = (u32)command.size;
                    console.input_char_count = (u32)command.size;
                }
            }
            if(event.keycode == BACKSPACE){
                input_remove_char();
                return(true);
            }
            if(event.keycode == ENTER){
                u8* line_u8 = (u8*)push_array(global_arena, u8, console.input_char_count + 1);
                mem_copy(line_u8, console.input, console.input_char_count);

                String8 line_str8 = {line_u8, console.input_char_count};
                line_str8 = str8_eat_spaces(line_str8);

                parse_line(line_str8);
                if(!command_args_count){ return(false); }

                console_store_command(line_str8);
                run_command(line_str8);

                console_cursor_reset();
                console_clear_input();
                console.command_history_at = 0;

                return(true);
            }
        }
    }
    return(false);
}

#endif
