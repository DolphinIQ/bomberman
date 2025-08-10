
#define DEBUG 1

#include "commons.h"
#include "bomberman_math.h"
#include "game.h"

global_var f32 delta_time = 0.0f;
// global_var double total_elapsed_time = 0.0;
global_var i64 start_counter = 0;
global_var i64 end_counter = 0;

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

internal_fn void game_clear_screen_buffer( struct GameOffscreenBuffer* bitmap )
{
    u32* buffer = (u32*)bitmap->memory;

    // SIMD version
#if 1
    int bitmap_length = bitmap->width * bitmap->height;
    if ( bitmap_length % 16 == 0 && AVX256_SUPPORTED )
    { // ( about 600% faster with _mm256_setzero_si256() )
        for ( int i = 0; i < bitmap_length; i += 256 / 32 )
        {
            simd_256i_store_zero( &buffer[ i ] );
        }
    }
    else if ( bitmap_length % 4 == 0 )
    { // ( about 140% faster with _mm_set1_epi32, 333% faster with _mm_setzero_si128() )
        for ( int i = 0; i < bitmap_length; i += 128 / 32 )
        {
            simd_128i_store_zero( &buffer[ i ] );
        }
    }
    else // do a normal loop
#endif
    {
        for ( int i = 0; i < bitmap->width * bitmap->height; i++ )
        {
            buffer[ i ] = 0;
        }
    }
    
}

// internal_fn int pos_to_map_idx( f32 pos_x, f32 pos_y, f32 tile_size, int map_count_x )
// {
//     int x = floor_f32_to_int( pos_x / tile_size );
//     int y = floor_f32_to_int( pos_y / tile_size );
//     int result = y * map_count_x + x;
//     return result;
// }

internal_fn void draw_triangle_px(
    struct GameOffscreenBuffer* bitmap, u32 color,
    int top_left_px_x, int top_left_px_y, int px_h
)
{
    const int px_w = px_h / 2 + (px_h % 2 == 0 ? 0 : 1 );

    u32* pixels = bitmap->memory;
    int bitmap_idx = -1;
    int x_edge_idx = 1;
    int x_edge_increment = 1;
    for ( int pixel_y = top_left_px_y; pixel_y < top_left_px_y + px_h; pixel_y++ )
    {
        for ( int pixel_x = top_left_px_x; pixel_x < top_left_px_x + x_edge_idx; pixel_x++ )
        {
            bitmap_idx = pixel_y * bitmap->width + pixel_x;
            pixels[ bitmap_idx ] = color;
        }
        x_edge_idx += x_edge_increment;
        if ( x_edge_idx == px_w ) x_edge_increment = -1;
    }
}

internal_fn void draw_rectangle_px(
    struct GameOffscreenBuffer* bitmap, u32 color,
    int bottom_left_px_x, int bottom_left_px_y, int px_w, int px_h
)
{
    u32* pixels = bitmap->memory;
    int bitmap_idx = -1;
    for ( int pixel_y = bottom_left_px_y; pixel_y < bottom_left_px_y + px_h; pixel_y++ )
    {
        for ( int pixel_x = bottom_left_px_x; pixel_x < bottom_left_px_x + px_w; pixel_x++ )
        {
            // Flip Y pixel so that Y+ axis becomes up!
            const int pixel_y_flipped = bitmap->height - pixel_y - 1;
            bitmap_idx = pixel_y_flipped * bitmap->width + pixel_x;
            Assert( bitmap_idx < bitmap->width * bitmap->height );
            pixels[ bitmap_idx ] = color;
        }
    }
}

internal_fn void
draw_rectangle_bottom_left(
    struct GameOffscreenBuffer* bitmap, u32 color, f32 bottom_left_x, f32 bottom_left_y, f32 width, f32 height
)
{
    const int width_px = math_round_f32_to_int( width * bitmap->pixels_per_meter );
    const int height_px = math_round_f32_to_int( height * bitmap->pixels_per_meter );
    const int bottom_left_px_x = math_round_f32_to_int( bottom_left_x * bitmap->pixels_per_meter );
    const int bottom_left_px_y = math_round_f32_to_int( bottom_left_y * bitmap->pixels_per_meter );
    draw_rectangle_px( bitmap, color, bottom_left_px_x, bottom_left_px_y, width_px, height_px );
}

internal_fn void
draw_rectangle_bottom_center(
    struct GameOffscreenBuffer* bitmap, u32 color, f32 bottom_center_x, f32 bottom_center_y, f32 width, f32 height
)
{
    const int width_px = math_round_f32_to_int( width * bitmap->pixels_per_meter );
    const int height_px = math_round_f32_to_int( height * bitmap->pixels_per_meter );
    const int bottom_left_px_x = math_round_f32_to_int( bottom_center_x * bitmap->pixels_per_meter - width_px / 2 );
    const int bottom_left_px_y = math_round_f32_to_int( bottom_center_y * bitmap->pixels_per_meter );
    draw_rectangle_px( bitmap, color, bottom_left_px_x, bottom_left_px_y, width_px, height_px );
}

internal_fn void
draw_rectangle_center(
    struct GameOffscreenBuffer* bitmap, u32 color, f32 center_x, f32 center_y, f32 width, f32 height
)
{
    const int width_px = math_round_f32_to_int( width * bitmap->pixels_per_meter );
    const int height_px = math_round_f32_to_int( height * bitmap->pixels_per_meter );
    const int bottom_left_px_x = math_round_f32_to_int( center_x * bitmap->pixels_per_meter - width_px / 2 );
    const int bottom_left_px_y = math_round_f32_to_int( center_y * bitmap->pixels_per_meter - height_px / 2 );
    draw_rectangle_px( bitmap, color, bottom_left_px_x, bottom_left_px_y, width_px, height_px );
}

internal_fn void
game_render_terrain( const struct Terrain* t, f32 tile_size, struct GameOffscreenBuffer* bitmap )
{
    const u32 color = 0x446644;

    for ( int y = 0; y < t->map_count_y; y++ )
    {
        for ( int x = 0; x < t->map_count_x; x++ )
        {
            int tmap_idx = y * t->map_count_x + x;
            if ( t->map[ tmap_idx ] == 1 )
            {
                draw_rectangle_bottom_left(
                    bitmap, color, (f32)x * tile_size, (f32)y * tile_size, tile_size, tile_size
                );
            }
        }
    }
}

internal_fn bool32
rect_contains_point( f32 rect_x, f32 rect_y, f32 rect_w, f32 rect_h, f32 p_x, f32 p_y )
{
    const f32 rect_x_right = rect_x + rect_w;
    const f32 rect_y_bottom = rect_y + rect_h;

    if ( p_x < rect_x || p_x > rect_x_right || p_y < rect_y || p_y > rect_y_bottom )
    {
        return false;
    }
    return true;
}

internal_fn bool32
terrain_check_collision_vs_point( const struct Terrain* terrain, f32 rect_size, f32 p_x, f32 p_y )
{
    for ( int i = 0; i < terrain->map_count_x * terrain->map_count_y; i++ )
    {
        // Only check against blocking tiles, like walls etc.
        if ( terrain->map[ i ] != 1 ) continue;

        const int x = i % terrain->map_count_x;
        const int y = i / terrain->map_count_x;
        const f32 rect_x = (f32)x * rect_size;
        const f32 rect_y = (f32)y * rect_size;

        if ( rect_contains_point( rect_x, rect_y, rect_size, rect_size, p_x, p_y ) )
        {
            return true;
        }
    }
    return false;
}

void game_update_and_render(
    struct ThreadContext* thread,
    const struct PlatformCode* platform,
    struct GameMemory* game_memory,
    const struct Input* input,
    struct GameOffscreenBuffer* buffer
)
{
    unused( thread );

    Assert( sizeof( struct GameState ) <= game_memory->permanent_memory_size );
    struct GameState* gs = game_memory->permanent_memory;

    const struct Terrain terrain = {
        .map_count_x = buffer->ratio_w * 1, // 16x
        .map_count_y = buffer->ratio_h * 1, // 9x
        .map = {
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
            1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
            1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1,
            1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1,
            1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1,
            1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
            1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        }
    };

    // Initialize game state when this function is ran for the first time
    if ( game_memory->is_initialized == false )
    {
        gs->player = (struct Player){
            .x = 3, .y = 3
        };

        start_counter = platform->get_current_counter();
        end_counter = platform->get_current_counter();
        gs->frame_count = 0;

        game_memory->is_initialized = true;
        printf( "Initialized game memory. pixels/meter: %f \n", buffer->pixels_per_meter );
    }

    { // NOTE(Dolphin): These assignments are moved to init once we find right values for them
        gs->tile_size_pixels = buffer->width / terrain.map_count_x; // 80px for 1280px wide buffer
        gs->tile_size_meters = 1.5;
        buffer->pixels_per_meter = gs->tile_size_pixels / gs->tile_size_meters;

        gs->player.w = 1.0;
        gs->player.h = 1.5;
        gs->player.speed = 5.0f;
    }

    delta_time = platform->get_seconds_elapsed( start_counter, platform->get_current_counter() );
    start_counter = platform->get_current_counter();

    game_clear_screen_buffer( buffer );
    // i64 clear_counter = platform->get_current_counter();
    // f32 taken = platform->get_seconds_elapsed( start_counter, clear_counter );
    // if ( gs->frame_count % 60 == 0 )
    // {
    //     printf( "| ms: %f | \n", taken * 1000.0f );
    // }

    game_render_terrain( &terrain, gs->tile_size_meters, buffer );

    const int triangle_x = 7.5f * buffer->pixels_per_meter;
    const int triangle_y = 3.25f * buffer->pixels_per_meter;
    const int triangle_h = 3.0f * buffer->pixels_per_meter;
    draw_triangle_px( buffer, 0xff0000, triangle_x, triangle_y, triangle_h );

    { // Player update and render
        f32 dir_x = 0.0f;
        f32 dir_y = 0.0f;
        if ( input->up.is_down )
        {
            dir_y += 1.0f;
        }
        if ( input->down.is_down )
        {
            dir_y -= 1.0f;
        }
        if ( input->left.is_down )
        {
            dir_x -= 1.0f;
        }
        if ( input->right.is_down )
        {
            dir_x += 1.0f;
        }
        // TODO(Dolphin): This results in higher speed diagonally. Fix when vectors
        dir_x *= gs->player.speed * delta_time;
        dir_y *= gs->player.speed * delta_time;

        const f32 new_player_x = gs->player.x + dir_x;
        const f32 new_player_y = gs->player.y + dir_y;

        // Check terrain collision for 3 bottom points of player rectangle
        bool32 move_allowed = true;
        f32 check_x = new_player_x - gs->player.w * 0.5f;
        f32 check_y = new_player_y;
        const f32 rect_size = gs->tile_size_meters;

        if ( terrain_check_collision_vs_point( &terrain, rect_size, check_x, check_y ) )
        {
            move_allowed = false;
        }

        check_x = new_player_x;
        if ( move_allowed && terrain_check_collision_vs_point( &terrain, rect_size, check_x, check_y ) )
        {
            move_allowed = false;
        }

        check_x = new_player_x + gs->player.w * 0.5f;
        if ( move_allowed && terrain_check_collision_vs_point( &terrain, rect_size, check_x, check_y ) )
        {
            move_allowed = false;
        }

        if ( move_allowed )
        {
            gs->player.x = new_player_x;
            gs->player.y = new_player_y;
        }

        // Render
        draw_rectangle_bottom_center( buffer, 0xFFFFFF, gs->player.x, gs->player.y, gs->player.w, gs->player.h );
        draw_rectangle_center( buffer, 0xFF0000, gs->player.x, gs->player.y, 0.1, 0.1 );
    }

    gs->frame_count += 1;
    end_counter = platform->get_current_counter();
}