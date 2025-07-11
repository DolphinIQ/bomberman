#pragma once

#include "commons.h"

struct Button
{
    bool8 was_down; // NOTE(Dolphin): not usable right now
    bool8 is_down;
};

struct Input
{
    union
    {
        struct
        {
            struct Button up;
            struct Button down;
            struct Button left;
            struct Button right;
        };
        struct Button buttons[ 4 ];
    };
};

struct GameMemory
{
    bool32 is_initialized;
    void* permanent_memory;
    u64 permanent_memory_size;
};

struct GameOffscreenBuffer
{
    void* memory;
    int width;
    int height;
    int pitch;
    float pixels_per_meters; // Its always used in drawing so I decided to put it here
    const int bytes_per_pixel;
};

struct GameState
{
    struct Player
    {
        float w, h;
        float x, y;
        float speed;
    } player;

    float tile_size_pixels;
    float tile_size_meters;
    u64 frame_count;
};

struct Terrain
{
    u32 map[ 16 * 9 ];
    int map_count_x;
    int map_count_y;
};

struct PlatformCode
{
    float (*get_seconds_elapsed)( i64 start_counter, i64 end_counter );
    i64 (*get_current_counter)();
};

// EXPORTED FUNCTIONS
typedef void game_update_and_render_fn_type(
    struct ThreadContext* thread,
    const struct PlatformCode* platform,
    struct GameMemory* game_memory,
    const struct Input* input,
    struct GameOffscreenBuffer* buffer
);
#define game_update_and_render_FN_NAME "game_update_and_render"