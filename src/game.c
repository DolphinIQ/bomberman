
#define DEBUG 1

#include "commons.h"
#include "game.h"

// #define RED_BIT 16
// #define GREEN_BIT 8
// // #define RED_BIT 10
// // #define GREEN_BIT 5
// internal_fn void render_gradient( struct Win32Bitmap* bitmap, u64 frame )
// {
//     u32* pixels = bitmap->memory;
//     // u16* pixels = bitmap->memory;
//     int i = 0;
//     for ( int y = 0; y < bitmap->height; y++ )
//     {
//         for ( int x = 0; x < bitmap->width; x++ )
//         {
//             u8 red = x + frame / 2;
//             u8 green = y + frame / 2;
//             pixels[ i ] = 0 | (green << GREEN_BIT) | (red << RED_BIT);
//             i += 1;
//         }
//     }
// }

internal_fn void
game_clear_screen_buffer( struct GameOffscreenBuffer* bitmap )
{
    u32* buffer = (u32*)bitmap->memory;
    for ( int i = 0; i < bitmap->width * bitmap->height; i++ )
    {
        buffer[ i ] = 0;
    }
}

internal_fn void
game_render_borders( struct GameOffscreenBuffer* bitmap, int border_w )
{
    u32* pixels = bitmap->memory;
    int i = 0;
    for ( int y = 0; y < bitmap->height; y++ )
    {
        for ( int x = 0; x < bitmap->width; x++ )
        {
            if ( x <= border_w || x > bitmap->width - border_w ||
                 y <= border_w || y > bitmap->height - border_w )
            {
                pixels[ i ] = 0x446644;
            }
            i += 1;
        }
    }
}

internal_fn void game_render_player(
    struct GameOffscreenBuffer* bitmap,
    float pos_x, float pos_y,
    float width, float height
)
{
    u32* pixels = bitmap->memory;
    for ( float y = pos_y; y < pos_y + height; y += 1.0f )
    {
        for ( float x = pos_x; x < pos_x + width; x += 1.0f )
        {
            const int i = y * bitmap->width + x;
            pixels[ i ] = 0xFFFFFF;
        }
    }
}

void game_update_and_render(
    struct ThreadContext* thread,
    struct GameMemory* game_memory,
    const struct Input* input,
    struct GameOffscreenBuffer* buffer
)
{
    unused( thread );

    Assert( sizeof( struct GameState ) <= game_memory->permanent_memory_size );
    struct GameState* gs = game_memory->permanent_memory;

    // Initialize game state when this function is ran for the first time
    if ( game_memory->is_initialized == false )
    {
        gs->player = (struct Player){
            .x = 2, .y = 2,
            .speed = 1.0f
        };
        game_memory->is_initialized = true;
    }

    game_clear_screen_buffer( buffer );
    game_render_borders( buffer, 20 );

    // Player
    float dir_x = 0.0f;
    float dir_y = 0.0f;
    if ( input->up.is_down )
    {
        dir_y -= 1.0;
    }
    if ( input->down.is_down )
    {
        dir_y += 1.0;
    }
    if ( input->left.is_down )
    {
        dir_x -= 1.0;
    }
    if ( input->right.is_down )
    {
        dir_x += 1.0;
    }
    // TODO(Dolphin): This results in higher speed diagonally. Fix when vectors
    dir_x *= gs->player.speed;
    dir_y *= gs->player.speed;

    gs->player.x += dir_x;
    gs->player.y += dir_y;

    game_render_player(
        buffer, gs->player.x, gs->player.y, 10, 15
    );
}