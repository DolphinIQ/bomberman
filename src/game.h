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

struct GameOffscreenBuffer
{
    void* memory;
    int width;
    int height;
    int pitch;
    const int bytes_per_pixel;
};

struct GameState
{
    struct Player
    {
        float x;
        float y;
        float speed;
    } player;
};

// EXPORTED FUNCTIONS
typedef void game_update_and_render_fn_type(
    struct ThreadContext* thread,
    struct GameState* gs,
    const struct Input* input,
    struct GameOffscreenBuffer* buffer
);
#define game_update_and_render_FN_NAME "game_update_and_render"