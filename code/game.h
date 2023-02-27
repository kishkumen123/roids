#ifndef GAME_H
#define GAME_H

#include "math.h"
#include "rect.h"
#include "renderer.h"
#include "entity.h"

typedef struct PermanentMemory{
    Arena arena;
    String8 cwd; // CONSIDER: this might be something we want to be set on the platform side
    String8 dir_assets; // CONSIDER: this might be something we want to be set on the platform side

    u32 generation[100];
    u32 free_entities[100];
    u32 free_entities_at;

    Entity entities[100];
    u32 entities_count;

    Entity* console;
} PermanentMemory;

typedef struct TransientMemory{
    Arena arena;
    Arena *frame_arena;
    Arena *render_command_arena;
} TransientMemory;

static Entity*
entity_from_handle(PermanentMemory* pm, EntityHandle handle){
    Entity *result = 0;
    if(handle.index < ArrayCount(pm->entities)){
        Entity *e = pm->entities + handle.index;
        if(e->generation == handle.generation){
            result = e;
        }
    }
    return(result);
}

static EntityHandle
handle_from_entity(PermanentMemory* pm, Entity *e){
    Assert(e != 0);
    EntityHandle result = {0};
    if((e >= pm->entities) && (e < (pm->entities + ArrayCount(pm->entities)))){
        result.index = e->index;
        result.generation = e->generation;
    }
    return(result);
}

static void
remove_entity(PermanentMemory* pm, Entity* e){
    e->type = EntityType_None;
    pm->free_entities_at++;
    pm->free_entities[pm->free_entities_at] = e->index;
    e->index = 0;
    e->generation = 0;
}

static Entity*
add_entity(PermanentMemory *pm, EntityType type){
    if(pm->free_entities_at >= 0){
        s32 free_entity_index = pm->free_entities[pm->free_entities_at--];
        Entity *e = pm->entities + free_entity_index;
        e->index = free_entity_index;
        pm->generation[e->index] += 1;
        e->generation = pm->generation[e->index];
        e->type = type;

        return(e);
    }
    return(0);
}

static Entity*
add_pixel(PermanentMemory* pm, Rect rect, RGBA color){
    Entity* e = add_entity(pm, EntityType_Pixel);
    e->rect = rect;
    e->color = color;
    return(e);
}

static Entity*
add_segment(PermanentMemory* pm, v2 p0, v2 p1, RGBA color){
    Entity* e = add_entity(pm, EntityType_Segment);
    e->color = color;
    e->p0 = p0;
    e->p1 = p1;
    return(e);
}

static Entity*
add_ray(PermanentMemory* pm, Rect rect, v2 direction, RGBA color){
    Entity* e = add_entity(pm, EntityType_Ray);
    e->rect = rect;
    e->color = color;
    e->direction = direction;
    return(e);
}

static Entity*
add_line(PermanentMemory* pm, Rect rect, v2 direction, RGBA color){
    Entity* e = add_entity(pm, EntityType_Line);
    e->rect = rect;
    e->color = color;
    e->direction = direction;
    return(e);
}

static Entity*
add_rect(PermanentMemory* pm, Rect rect, RGBA color, s32 bsize = 0, RGBA bcolor = {0, 0, 0, 0}, bool bextrudes = false){
    Entity* e = add_entity(pm, EntityType_Rect);
    e->rect =  rect;
    e->color = color;
    e->border_size =  bsize;
    e->border_color = bcolor;
    e->border_extrudes = bextrudes;
    return(e);
}

static Entity*
add_console(PermanentMemory* pm, Rect rect, RGBA color, s32 bsize = 0, RGBA bcolor = {0, 0, 0, 0}, bool bextrudes = false){
    Entity* e = add_entity(pm, EntityType_Rect);
    e->rect =  rect;
    e->color = color;
    e->border_size     = bsize;
    e->border_color    = bcolor;
    e->console_state   = OPEN;
    e->start_position  = e->rect.y0;
    e->border_extrudes = bextrudes;
    e->draw = false;
    return(e);
}

static Entity*
add_box(PermanentMemory* pm, Rect rect, RGBA color){
    Entity* e = add_entity(pm, EntityType_Box);
    e->rect = rect;
    e->color = color;
    return(e);
}

static Entity*
add_quad(PermanentMemory* pm, v2 p0, v2 p1, v2 p2, v2 p3, RGBA color, bool fill){
    Entity* e = add_entity(pm, EntityType_Quad);
    e->color = color;
    e->p0 = p0;
    e->p1 = p1;
    e->p2 = p2;
    e->p3 = p3;
    e->fill = fill;
    return(e);
}

static Entity*
add_triangle(PermanentMemory *pm, v2 p0, v2 p1, v2 p2, RGBA color, bool fill){
    Entity* e = add_entity(pm, EntityType_Triangle);
    e->p0 = p0;
    e->p1 = p1;
    e->p2 = p2;
    e->color = color;
    e->fill = fill;
    return(e);
}

static Entity*
add_circle(PermanentMemory *pm, Rect rect, u8 rad, RGBA color, bool fill){
    Entity* e = add_entity(pm, EntityType_Circle);
    e->rect = rect;
    e->color = color;
    e->fill = fill;
    e->rad = rad;
    return(e);
}

static Entity*
add_bitmap(PermanentMemory* pm, Rect rect, Bitmap image){
    Entity* e = add_entity(pm, EntityType_Bitmap);
    e->rect = make_rect(0, 0, image.width, image.height);
    e->image = image;
    return(e);
}

static void
draw_commands(RenderBuffer *render_buffer, Arena *commands){
    void* at = commands->base;
    void* end = (u8*)commands->base + commands->used;
    while(at != end){
        CommandHeader* base_command = (CommandHeader*)at;

        switch(base_command->type){
            case RenderCommand_ClearColor:{
                ClearColorCommand *command = (ClearColorCommand*)base_command;
                clear_color(render_buffer, command->ch.color);
                at = (u8*)commands->base + command->ch.arena_used;
            } break;
            case RenderCommand_Pixel:{
                PixelCommand *command = (PixelCommand*)base_command;
                draw_pixel(render_buffer, make_v2(command->ch.rect.x0, command->ch.rect.y0), command->ch.color);
                at = (u8*)commands->base + command->ch.arena_used;
            } break;
            case RenderCommand_Segment:{
                SegmentCommand *command = (SegmentCommand*)base_command;
                draw_segment(render_buffer, command->p0, command->p1, command->ch.color);
                at = (u8*)commands->base + command->ch.arena_used;
            } break;
            case RenderCommand_Ray:{
                RayCommand *command = (RayCommand*)base_command;
                draw_ray(render_buffer, command->ch.rect, command->ch.direction, command->ch.color);
                at = (u8*)commands->base + command->ch.arena_used;
            } break;
            case RenderCommand_Line:{
                LineCommand *command = (LineCommand*)base_command;
                draw_line(render_buffer, command->ch.rect, command->ch.direction, command->ch.color);
                at = (u8*)commands->base + command->ch.arena_used;
            } break;
            case RenderCommand_Rect:{
                RectCommand *command = (RectCommand*)base_command;
                draw_rect(render_buffer, command->ch.rect, command->ch.color);
                at = (u8*)commands->base + command->ch.arena_used;
            } break;
            case RenderCommand_Box:{
                BoxCommand *command = (BoxCommand*)base_command;
                draw_box(render_buffer, command->ch.rect, command->ch.color);
                at = (u8*)commands->base + command->ch.arena_used;
            } break;
            case RenderCommand_Quad:{
                QuadCommand *command = (QuadCommand*)base_command;
                draw_quad(render_buffer, command->p0, command->p1, command->p2, command->p3, command->ch.color, command->ch.fill);
                at = (u8*)commands->base + command->ch.arena_used;
            } break;
            case RenderCommand_Triangle:{
                TriangleCommand *command = (TriangleCommand*)base_command;
                draw_triangle(render_buffer, command->p0, command->p1, command->p2, base_command->color, base_command->fill);
                at = (u8*)commands->base + command->ch.arena_used;
            } break;
            case RenderCommand_Circle:{
                CircleCommand *command = (CircleCommand*)base_command;
                draw_circle(render_buffer, command->ch.rect.x0, command->ch.rect.y0, command->ch.rad, command->ch.color, command->ch.fill);
                at = (u8*)commands->base + command->ch.arena_used;
            } break;
            case RenderCommand_Bitmap:{
                BitmapCommand *command = (BitmapCommand*)base_command;
                draw_bitmap(render_buffer, command->ch.rect, command->image);
                at = (u8*)commands->base + command->ch.arena_used;
            } break;
        }
    }
}

PermanentMemory* pm;
TransientMemory* tm;


f32 t = 0;
static void
update_game(Memory* memory, RenderBuffer* render_buffer, Events* events, Controller* controller, Clock* clock){
    Assert(sizeof(PermanentMemory) < memory->permanent_size);
    Assert(sizeof(TransientMemory) < memory->transient_size);
    pm = (PermanentMemory*)memory->permanent_base;
    tm = (TransientMemory*)memory->transient_base;

    RGBA RED =     {1.0f, 0.0f, 0.0f,  1.0f};
    RGBA GREEN =   {0.0f, 1.0f, 0.0f,  1.0f};
    RGBA BLUE =    {0.0f, 0.0f, 1.0f,  0.5f};
    RGBA MAGENTA = {1.0f, 0.0f, 1.0f,  0.1f};
    RGBA TEAL =    {0.0f, 1.0f, 1.0f,  1.0f};
    RGBA PINK =    {0.92f, 0.62f, 0.96f, 1.0f};
    RGBA YELLOW =  {0.9f, 0.9f, 0.0f,  1.0f};
    RGBA ORANGE =  {1.0f, 0.5f, 0.15f,  1.0f};
    RGBA DGRAY =   {0.5f, 0.5f, 0.5f,  1.0f};
    RGBA LGRAY =   {0.8f, 0.8f, 0.8f,  1.0f};
    RGBA WHITE =   {1.0f, 1.0f, 1.0f,  1.0f};
    RGBA BLACK =   {0.0f, 0.0f, 0.0f,  1.0f};
    RGBA ARMY_GREEN =   {0.25f, 0.25, 0.23,  1.0f};


    if(!memory->initialized){

        init_arena(&pm->arena, (u8*)memory->permanent_base + sizeof(PermanentMemory), memory->permanent_size - sizeof(PermanentMemory));
        init_arena(&tm->arena, (u8*)memory->transient_base + sizeof(TransientMemory), memory->transient_size - sizeof(TransientMemory));
        pm->cwd = os_get_cwd(&pm->arena);
        pm->dir_assets = str8_concatenate(&pm->arena, pm->cwd, str8_literal("\\data\\"));

        tm->render_command_arena = push_arena(&tm->arena, MB(16));
        tm->frame_arena = push_arena(&tm->arena, MB(100));

        // setup free entities array
        pm->free_entities_at = ArrayCount(pm->free_entities) - 1;
        pm->entities_count = array_count(pm->entities) - 1;
        for(s32 i = ArrayCount(pm->free_entities) - 1; i >= 0; --i){
            pm->free_entities[i] = ArrayCount(pm->free_entities) - 1 - i;
        }

        Entity *zero_entity = add_entity(pm, EntityType_None);
        pm->console = add_console(pm, make_rect(0, .5f, 1, 1), ARMY_GREEN);

        memory->initialized = true;
    }
    arena_free(render_buffer->render_command_arena);
    push_clear_color(render_buffer->render_command_arena, BLACK);
    Entity* console = pm->console;

    // NOTE: Process events.
    while(!events_empty(events)){
        Event event = event_get(events);

        if(event.type == TEXT_INPUT){
            //print("text_input: %i - %c\n", event.keycode, event.keycode);
            //print("-----------------------------\n");
        }
        if(event.type == KEYBOARD){
            if(event.key_pressed){
                //print("key_code: %llu\n", event.keycode);
                if(event.keycode == ESCAPE){
                    print("quiting\n");
                    should_quit = true;
                }
                if(event.keycode == 'Q'){
                    //print("keycode: %llu - repeat: %i\n", event.keycode, event.repeat);
                }
                if(event.keycode == 'Q' && !event.repeat){
                    //count += 1;
                    //print("QQQQQ: %i\n", count);
                }
                if(event.keycode == TILDE && !event.repeat){
                    t = 0;
                    pm->console->start_position = pm->console->rect.y0;
                    if(event.shift_pressed){
                        if(pm->console->console_state == OPEN_BIG){
                            pm->console->console_state = CLOSED;
                        }
                        else{ pm->console->console_state = OPEN_BIG; }
                    }
                    else{
                        if(pm->console->console_state == OPEN){
                            pm->console->console_state = CLOSED;
                        }
                        else{ pm->console->console_state = OPEN; }
                    }
                }
            }
            else{
            }
        }
    }
    //print("open_console: %i\n", controller->open_console);

    f32 y_open = .7f;
    f32 y_open_big = .2f;
    f32 y_closed = 1.0f;
    f32 open_speed = 0.5f;

    f32 lerp_speed =  open_speed * clock->dt;
    //print("console state: %i\n", console->console_state);
    if(console->console_state == CLOSED){
        if(t < 1) {
            t += lerp_speed;
            //console->rect.y0 = lerp(console->rect.y0, y_closed, t);
        }
    }
    else if(console->console_state == OPEN){
        if(t < 1) {
            t += lerp_speed;
            //console->rect.y0 = lerp(console->rect.y0, y_open, t);
        }
    }
    else if(console->console_state == OPEN_BIG){
        if(t < 1) {
            t += lerp_speed;
            //console->rect.y0 = lerp(console->rect.y0, y_open_big, t);
        }
    }


    Arena* render_command_arena = render_buffer->render_command_arena;
    for(u32 entity_index = pm->free_entities_at; entity_index < ArrayCount(pm->entities); ++entity_index){
        Entity *e = pm->entities + pm->free_entities[entity_index];

        switch(e->type){
            case EntityType_Pixel:{
                push_pixel(render_command_arena, e->rect, e->color);
            }break;
            case EntityType_Segment:{
                push_segment(render_command_arena, e->p0, e->p1, e->color);
            }break;
            case EntityType_Line:{
                push_line(render_command_arena, e->rect, e->direction, e->color);
            }break;
            case EntityType_Ray:{
                push_ray(render_command_arena, e->rect, e->direction, e->color);
            }break;
            case EntityType_Rect:{
                if(e->border_extrudes){
                    if(e->border_size){
                        Rect border_rect = rect_get_border_extruding(e->rect, e->border_size);
                        push_rect(render_command_arena, border_rect, e->border_color);
                    }
                    push_rect(render_command_arena, e->rect, e->color);
                }
                else{
                    if(e->border_size){
                        push_rect(render_command_arena, e->rect, e->border_color);
                    }
                    Rect border_rect = rect_get_border_intruding(e->rect, e->border_size);
                    push_rect(render_command_arena, border_rect, e->color);
                }
            }break;
            case EntityType_Box:{
                push_box(render_command_arena, e->rect, e->color);
            }break;
            case EntityType_Quad:{
                push_quad(render_command_arena, e->p0, e->p1, e->p2, e->p3, e->color, e->fill);
            }break;
            case EntityType_Triangle:{
                push_triangle(render_command_arena, e->p0, e->p1, e->p2, e->color, e->fill);
            }break;
            case EntityType_Circle:{
                push_circle(render_command_arena, e->rect, e->rad, e->color, e->fill);
            }break;
            case EntityType_Bitmap:{
                push_bitmap(render_command_arena, e->rect, e->image);
            }break;
            case EntityType_None:{
            }break;
            case EntityType_Object:{
            }break;
        }
    }
}

#endif