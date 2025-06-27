
#include <stdio.h>
#include <windows.h>

#include "commons.h"

struct Win32Dimensions
{
    int width;
    int height;
};

struct Win32Bitmap
{
    BITMAPINFO info;
    void* memory;
    int width;
    int height;
    int pitch;
    const int bytes_per_pixel;
};

struct GameOffscreenBuffer
{
    void* memory;
    int width;
    int height;
    int pitch;
    const int bytes_per_pixel;
};

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

struct GameState
{
    struct Player
    {
        float x;
        float y;
        float speed;
    } player;
};

#define PIXEL_BYTE_SIZE 4
// #define PIXEL_BYTE_SIZE 2
// TODO: This is a global for now
global_var struct Win32Bitmap bitmap_buffer = {
    .info = {
        .bmiHeader = {
            .biSize = sizeof( bitmap_buffer.info.bmiHeader ),
            .biPlanes = 1, // has to be set to 1
            .biBitCount = PIXEL_BYTE_SIZE * 8,
            .biCompression = BI_RGB
        }
    },
    .bytes_per_pixel = PIXEL_BYTE_SIZE
};
global_var struct Win32Dimensions window_dimensions = { 1280, 720 };
global_var struct Win32Dimensions render_size = { 320, 180 };
global_var bool32 app_running = true;
global_var u64 frame_count = 0;

global_var struct Input input = { 0 };
global_var struct GameState game_state =
{
    .player = {
        .x = 2, .y = 2,
        .speed = 1.0f
    }
};

// #define RED_BIT 16
// #define GREEN_BIT 8
// // #define RED_BIT 10
// // #define GREEN_BIT 5
// internal_fn void render_gradient( struct Win32Bitmap* bitmap, u64 frame )
// {
//     u32* pixels = bitmap->memory;
//     // u16* pixels = bitmap->memory;
//     // u8* pixels = bitmap->memory;
//     int i = 0;
//     for ( int y = 0; y < bitmap->height; y++ )
//     {
//         for ( int x = 0; x < bitmap->width; x++ )
//         {
//             u8 red = x + frame / 2;
//             u8 green = y + frame / 2;
//             pixels[ i ] = 0 | (green << GREEN_BIT) | (red << RED_BIT);
//             // pixels[ i ] = i + frame / 2;
//             i += 1;
//         }
//     }
// }

internal_fn void
clear_screen_buffer( struct GameOffscreenBuffer* bitmap )
{
    memset(
        bitmap->memory, 0,
        bitmap->width * bitmap->height * bitmap->bytes_per_pixel
    );
}

internal_fn void
render_borders( struct GameOffscreenBuffer* bitmap, int border_w )
{
    u32* pixels = bitmap->memory;
    // u16* pixels = bitmap->memory;
    int i = 0;
    for ( int y = 0; y < bitmap->height; y++ )
    {
        for ( int x = 0; x < bitmap->width; x++ )
        {
            if ( x <= border_w || x > bitmap->width - border_w ||
                 y <= border_w || y > bitmap->height - border_w )
            {
                pixels[ i ] = 0x446644;
                // pixels[ i ] = 0 | (255 << 10);
            }
            i += 1;
        }
    }
}

internal_fn void draw_player(
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

internal_fn void
game_update_and_render(
    struct GameState* gs,
    const struct Input* input,
    struct GameOffscreenBuffer* buffer
)
{
    clear_screen_buffer( buffer );
    render_borders( buffer, 20 );

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

    draw_player(
        buffer, game_state.player.x, game_state.player.y, 10, 15
    );
}

internal_fn struct Win32Dimensions
win32_get_window_dimensions( HWND window_handle )
{
    RECT client_rect;
    GetClientRect( window_handle, &client_rect );
    struct Win32Dimensions dimensions = {
        .width = client_rect.right - client_rect.left,
        .height = client_rect.bottom - client_rect.top
    };
    return dimensions;
}

internal_fn void
win32_resize_dib_section(
    struct Win32Bitmap* bitmap, int width, int height
) {
    if ( bitmap->memory ) VirtualFree( bitmap->memory, 0, MEM_RELEASE );

    bitmap->width = width;
    bitmap->height = height;
    bitmap->pitch = bitmap->width * bitmap->bytes_per_pixel;

    bitmap->info.bmiHeader.biWidth = bitmap->width;
    bitmap->info.bmiHeader.biHeight = -bitmap->height;
    // Negative biHeight means top-down DIB (origin in top-left corner)

    u64 memory_size = bitmap->width * bitmap->height * bitmap->bytes_per_pixel;
    bitmap->memory = VirtualAlloc(
        0, memory_size, MEM_COMMIT, PAGE_READWRITE
    );
    printf( "Allocated %llu KB of bitmap memory \n", memory_size / 1024 );
}

internal_fn void
win32_display_buffer_in_window( struct Win32Bitmap* bitmap, HDC device_ctx )
{
    // TODO: Aspect ratio correction
    StretchDIBits(
        device_ctx,
        0, 0, window_dimensions.width, window_dimensions.height, // dest rect
        // 0, 0, bitmap->width, bitmap->height, // dest rect
        0, 0, bitmap->width, bitmap->height, // src rect
        bitmap->memory,
        &bitmap->info,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

internal_fn LRESULT
win32_main_window_callback(
    HWND window_handle,
    UINT message,
    WPARAM w_param,
    LPARAM l_param
)
{
    LRESULT result = 0;

    switch ( message ) {
        case WM_SIZE: { // Window resize
            window_dimensions = win32_get_window_dimensions( window_handle );
            printf( "WM_SIZE\n" );
        } break;
        case WM_CLOSE: { // TODO: Handle this with a message to the user?
            app_running = false;
            printf( "WM_CLOSE\n" );
        } break;
        case WM_DESTROY: { // TODO: Handle this with a message to the user?
            app_running = false;
            printf( "WM_DESTROY\n" );
        } break;
        case WM_ACTIVATEAPP: {
            printf( "WM_ACTIVATEAPP\n" );
        } break;
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            // Virtual code is stored in the WParam
            u32 vcode = w_param;
            // printf( "[%c] key\n", vcode );
            #define KEY_ALT_WAS_DOWN_BIT 29
            #define KEY_WAS_DOWN_BIT 30
            #define KEY_IS_DOWN_BIT 31
            bool32 was_down = ( l_param & (1 << KEY_WAS_DOWN_BIT) ) != 0;
            bool32 is_down = ( l_param & (1 << KEY_IS_DOWN_BIT) ) == 0;
            bool32 alt_was_down = ( l_param & (1 << KEY_ALT_WAS_DOWN_BIT) ) != 0;
            // Filter away held-down repeated keystrokes
            if ( is_down && was_down ) break;

            if ( vcode == 'W' )
            {
                // if ( was_down ) printf( "[W] was_down\n" );
                // if ( is_down ) printf( "[W] is_down\n" );

                if ( is_down )
                {
                    input.up.is_down = true;
                }
                if ( was_down )
                {
                    input.up.is_down = false;
                }
            }
            else if ( vcode == 'S' )
            {
                if ( is_down )
                {
                    input.down.is_down = true;
                }
                if ( was_down )
                {
                    input.down.is_down = false;
                }
            }
            else if ( vcode == 'A' )
            {
                if ( is_down )
                {
                    input.left.is_down = true;
                }
                if ( was_down )
                {
                    input.left.is_down = false;
                }
            }
            else if ( vcode == 'D' )
            {
                if ( is_down )
                {
                    input.right.is_down = true;
                }
                if ( was_down )
                {
                    input.right.is_down = false;
                }
            }
            else if ( vcode == VK_SPACE )
            {
            }
            else if ( vcode == VK_UP )
            {
            }
            else if ( vcode == VK_DOWN )
            {
            }
            else if ( vcode == VK_LEFT )
            {
            }
            else if ( vcode == VK_RIGHT )
            {
            }
            else if ( vcode == VK_ESCAPE )
            {
                app_running = false;
            }
            else if ( vcode == VK_F4 && alt_was_down )
            {
                app_running = false;
            }
        } break; // End of input
        case WM_PAINT: // Runs also on every resize
        {
            printf( "WM_PAINT\n" );

            PAINTSTRUCT paint = {0};
            HDC device_ctx = BeginPaint( window_handle, &paint );
            win32_display_buffer_in_window( &bitmap_buffer, device_ctx );
            EndPaint( window_handle, &paint );
        } break;
        default: {
            result = DefWindowProcW( window_handle, message, w_param, l_param );
        } break;
    }
    return result;
}

int WINAPI WinMain(
    HINSTANCE program_handle,
    HINSTANCE prev_program_handle,
    LPSTR command_line,
    int show_type
)
{
    unused( prev_program_handle, command_line );
    
    const WNDCLASSW window_class = {
        .style = CS_HREDRAW | CS_VREDRAW, // redraw on window resize
        .lpfnWndProc = win32_main_window_callback,
        .hInstance = program_handle,
        // .hIcon = 
        .lpszClassName = L"HandmadeHeroWindowClass"
    };
    RegisterClassW( &window_class );

    RECT win_rect = { 0, 0, window_dimensions.width, window_dimensions.height };
    AdjustWindowRect( &win_rect, WS_OVERLAPPEDWINDOW, false );

    HWND window_handle = CreateWindowExW(
        0,                              // Optional window styles.
        window_class.lpszClassName,     // Window class
        L"Win32",    // Window text (title bar if has one)
        WS_OVERLAPPEDWINDOW,            // Window style
        500, 300,                                   // Pos
        // window_dimensions.width, window_dimensions.height,           // Size
        win_rect.right - win_rect.left, win_rect.bottom - win_rect.top, // Size
        NULL,           // Parent window
        NULL,           // Menu
        program_handle, // Instance handle
        NULL            // Additional application data
    );

    if ( window_handle == NULL )
    {
        _Post_equals_last_error_ DWORD err = GetLastError();
        printf( "Error creating window: %li \n", err );
        exit( EXIT_FAILURE );
    }

    // render_size = window_dimensions;
    win32_resize_dib_section( &bitmap_buffer, render_size.width, render_size.height );

    ShowWindow( window_handle, show_type );

    HDC device_ctx = GetDC( window_handle );
    while ( app_running )
    {
        MSG msg; // Process all messages
        while( PeekMessageW( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            if ( msg.message == WM_QUIT ) app_running = false;

            TranslateMessage( &msg );
            DispatchMessageW( &msg );
        }

        // Pack screen buffer into a platform independent struct for game
        struct GameOffscreenBuffer offscreen_buffer = {
            .memory = bitmap_buffer.memory,
            .width = bitmap_buffer.width,
            .height = bitmap_buffer.height,
            .pitch = bitmap_buffer.pitch,
            .bytes_per_pixel = bitmap_buffer.bytes_per_pixel,
        };

        game_update_and_render( &game_state, &input, &offscreen_buffer );

        win32_display_buffer_in_window( &bitmap_buffer, device_ctx );

        frame_count += 1;
        Sleep( 15 );
    }

    ReleaseDC( window_handle, device_ctx );
    printf("Exited successfully :) \n");
    return 0;
}