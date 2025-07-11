
#define DEBUG 1

#include <stdio.h>
#include <windows.h>
#include <xaudio2.h>

#include "commons.h"
#include "bomberman_math.h"
#include "game.h"

// Procedure type for XAudio2 dll func
typedef HRESULT XAudio2Create_type (
    _Outptr_ IXAudio2** ppXAudio2,
    UINT32 Flags,
    XAUDIO2_PROCESSOR XAudio2Processor
);
#define SAMPLE_RATE 44100
// #define SAMPLE_RATE 48000

// NOTE(Dolphin): Hard coded for now. Maybe do dynamic path finding in the future? :/
const char* game_dll_name = "D:\\0_Coding_0\\C\\0_Games\\bomberman\\game.dll";
const char* game_temp_dll_name = "game_temp.dll";

struct Win32GameCode
{
    HMODULE dll;
    game_update_and_render_fn_type* game_update_and_render;
    // game_update_audio *GameUpdateAudio;

    FILETIME last_write_time;
    bool32 is_valid;
};

struct Win32Dimensions
{
    int width;
    int height;
};

// https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfoheader
struct Win32Bitmap
{
    BITMAPINFO info;
    void* memory;
    int width;
    int height;
    int pitch;
    const int bytes_per_pixel;
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
global_var struct Win32Dimensions render_size = { 160, 90 };
global_var struct Input input = { 0 };
global_var i64 global_perf_frequency = 0; // COUNTS PER SECOND
global_var bool32 app_running = true;

internal_fn i64 win32_get_current_counter()
{
    LARGE_INTEGER result;
    QueryPerformanceCounter( &result );
    return result.QuadPart;
}

internal_fn float win32_get_seconds_elapsed( i64 start, i64 end )
{
    // float result = (double)(end.QuadPart - start.QuadPart) / (double)global_perf_frequency;
    float result = ( (float)(end - start) ) / global_perf_frequency;
    return result;
}

internal_fn FILETIME win32_get_file_write_time( const char *file_path )
{
    WIN32_FILE_ATTRIBUTE_DATA data;
    BOOL success = GetFileAttributesExA( file_path, GetFileExInfoStandard, &data );
    if ( success == false )
    {
        _Post_equals_last_error_ DWORD err = GetLastError();
        printf( "ERROR (%li)! Cant get '%s' file write time \n", err, file_path );
        exit( EXIT_FAILURE );
    }
    return data.ftLastWriteTime;
}

internal_fn struct Win32GameCode win32_load_game_code( HWND window )
{
    // Wait a moment to make sure that the changed source dll is finished compiling.
    // Otherwise its copied unfinished resulting in temp having size of 0 KB.
    Sleep( 16 );

    struct Win32GameCode game_code = { 0 };
    game_code.last_write_time = win32_get_file_write_time( game_dll_name );

    // Copy and run the copied DLL so that we dont have write access violation
    BOOL success = CopyFileA( game_dll_name, game_temp_dll_name, false );
    if ( success == false )
    {
        _Post_equals_last_error_ DWORD err = GetLastError();
        printf( "ERROR (%li)! Cant copy '%s' file \n", err, game_dll_name );
        exit( EXIT_FAILURE );
    }

    game_code.dll = LoadLibraryA( game_temp_dll_name );
    if ( game_code.dll == NULL )
    {
        _Post_equals_last_error_ DWORD err = GetLastError();
        printf(
            "%s %s:%i Error (%li) loading Game Code dll '%s' \n",
            __FILE__, __FUNCTION__, __LINE__,  err, game_temp_dll_name
        );
        MessageBoxW( window, L"Error loading Game Code dll", L"Error", MB_OK );
        exit( EXIT_FAILURE );
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"

    game_code.game_update_and_render = (game_update_and_render_fn_type*)GetProcAddress(
        game_code.dll, game_update_and_render_FN_NAME
    );

#pragma GCC diagnostic pop

    game_code.is_valid = ( true && game_code.game_update_and_render );
    if ( game_code.is_valid == false )
    {
        MessageBoxW( window, L"Cant load functions in module", L"Error", MB_OK );
        exit( EXIT_FAILURE );
    }

    printf( "Reloaded game code \n" );
    return game_code;
}

internal_fn void win32_unload_game_code( struct Win32GameCode* game_code )
{
    if( game_code->dll )
    {
        FreeLibrary( game_code->dll );
        game_code->dll = NULL;
    }
    game_code->is_valid = false;
    game_code->game_update_and_render = NULL;
}

// https://learn.microsoft.com/en-us/windows/win32/xaudio2/xaudio2-key-concepts
internal_fn void win32_init_xaudio2( HWND window )
{
    // TODO(Dolphin): Diagnostics, detect which error happened for what:
    // https://learn.microsoft.com/en-us/windows/win32/xaudio2/xaudio2-error-codes

    // Init XAudio2 (XAudio2)
    // HMODULE xaudio2_lib = LoadLibraryA( "Windows.Media.Audio.dll" );
    HMODULE xaudio2_lib = LoadLibraryA( "XAudio2_9.dll" );
    // HMODULE xaudio2_lib = LoadLibraryA( XAUDIO2_DLL );
    if ( xaudio2_lib == NULL )
    {
        _Post_equals_last_error_ DWORD err = GetLastError();
        printf( "Error loading XAudio2: %li \n", err );
        MessageBoxW( window, L"Error loading XAudio2 dll", L"Error", MB_OK );
        exit( EXIT_FAILURE );
    }

    // MSDN says its necessary to initialize COM object, but it works without it...
    // HRESULT COM_lib_init = CoInitializeEx( NULL, COINIT_MULTITHREADED );
    // if ( COM_lib_init != S_OK )
    // {
    //     MessageBoxW( window, L"Error initializing COM Library", L"Error", MB_OK );
    //     exit( EXIT_FAILURE );
    // }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"

    XAudio2Create_type* XAudio2Create_proc = (XAudio2Create_type*)GetProcAddress(
        xaudio2_lib, "XAudio2Create"
    );

#pragma GCC diagnostic pop

    if ( XAudio2Create_proc == NULL )
    {
        _Post_equals_last_error_ DWORD err = GetLastError();
        printf( "Error loading XAudio2Create() function: %li \n", err );
        Sleep( 4000 );
        exit( EXIT_FAILURE );
    }

    IXAudio2* XAudio2 = NULL;
    HRESULT create_res = XAudio2Create_proc( &XAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR );
    if ( create_res != S_OK )
    {
        MessageBoxW( window, L"Error for XAudio2Create()", L"Error", MB_OK );
        exit( EXIT_FAILURE );
    }

    IXAudio2MasteringVoice* master_voice = NULL;
    if ( XAudio2->lpVtbl->CreateMasteringVoice(
        XAudio2, &master_voice,
        XAUDIO2_DEFAULT_CHANNELS,
        SAMPLE_RATE,
        0,                  // Flags
        NULL,               // Default audio output device
        NULL,               // Dont use any effects
        AudioCategory_Other // Stream category
    ) != S_OK )
    {
        MessageBoxW( window, L"Error creating XAudio2 master voice", L"Error", MB_OK );
        exit( EXIT_FAILURE );
    }

    WAVEFORMATEX wave_format = { 0 };
    wave_format.wFormatTag = WAVE_FORMAT_PCM;
    wave_format.nChannels = 1;
    wave_format.nSamplesPerSec = SAMPLE_RATE;
    wave_format.wBitsPerSample = 16;
    wave_format.nBlockAlign = (wave_format.nChannels * wave_format.wBitsPerSample) / 8;
    wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec * wave_format.nBlockAlign;

    // TODO(Dolphin): Remove once playing WAV files works
    // printf(
    //     "nChannels: %i, wBitsPerSample: %i, nSamplesPerSec: %li, nBlockAlign: %i "
    //     "nAvgBytesPerSec: %li \n",
    //     wave_format.nChannels, wave_format.wBitsPerSample, wave_format.nSamplesPerSec,
    //     wave_format.nBlockAlign, wave_format.nAvgBytesPerSec
    // );

    // IXAudio2VoiceCallback cb = { 0 };
    // cb.lpVtbl->OnBufferEnd 

    IXAudio2SourceVoice* source_voice = NULL;
    HRESULT src_voice_result = XAudio2->lpVtbl->CreateSourceVoice(
        XAudio2, &source_voice, &wave_format,
        0, // Flags
        XAUDIO2_DEFAULT_FREQ_RATIO, // Frequency ratio
        // XAUDIO2_MIN_FREQ_RATIO, // Frequency ratio
        // &cb, // Callback
        NULL, // Callback
        NULL, // Voice sends list
        NULL // Effect chain
    );
    if ( src_voice_result != S_OK )
    {
        printf( "ERROR ( 0x%08lx ) \n", src_voice_result );
        MessageBoxW( window, L"Error createing source voice", L"Error", MB_OK );
        exit( EXIT_FAILURE );
    }

    // Prepare samples to play
    #define DURATION 1 // seconds
    const int samples_count = SAMPLE_RATE * DURATION;
    const int TONE_HZ = 440;
    i16 samples[ SAMPLE_RATE * DURATION ] = { 0 };
    // i16* samples = malloc( samples_count * sizeof( i16 ) );
    for ( int i = 0; i < samples_count; i++ )
    {
        float t = (float)i / SAMPLE_RATE;
        float sample = math_sinf(2 * PI * TONE_HZ * t);
        samples[ i ] = (i16)(sample * 32767);
    }

    // Buffer to send to XAudio2
    XAUDIO2_BUFFER buffer = {
        .AudioBytes = samples_count * sizeof( i16 ),
        .pAudioData = (BYTE*)samples,
        .Flags = XAUDIO2_END_OF_STREAM,
        // .LoopCount = XAUDIO2_LOOP_INFINITE,
    };
    source_voice->lpVtbl->SetVolume(source_voice, 0.02f, 0); // Optional
    HRESULT src_buf_result = source_voice->lpVtbl->SubmitSourceBuffer(
        source_voice, &buffer, NULL
    );
    if ( src_buf_result != S_OK )
    {
        printf( "ERROR( 0x%08lx ) SubmitSourceBuffer \n", src_buf_result );
    }

    HRESULT start_result = source_voice->lpVtbl->Start( source_voice, 0, 0 );
    if ( start_result != S_OK )
    {
        printf( "ERROR( 0x%08lx ) source start buffer \n", start_result );
    }

    // Wait until buffer is done playing
    // XAUDIO2_VOICE_STATE state = { 0 };
    // do {
    //     source_voice->lpVtbl->GetState(
    //         source_voice, &state, XAUDIO2_VOICE_NOSAMPLESPLAYED
    //     );
    //     printf(
    //         "state.BuffersQueued: %u, SamplesPlayed: %llu \n",
    //         state.BuffersQueued, state.SamplesPlayed
    //     );
    //     Sleep(100);
    // } while (state.BuffersQueued > 0);
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
win32_display_buffer_in_window( const struct Win32Bitmap* bitmap, HDC device_ctx )
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
        case WM_ACTIVATEAPP: { // When app gains or loses focus
            // printf( "WM_ACTIVATEAPP\n" );
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
        // https://learn.microsoft.com/en-us/windows/win32/winmsg/window-styles
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
        MessageBoxW(
            NULL, L"Error creating a window", L"Error", MB_OK
        );
        exit( EXIT_FAILURE );
    }

    ShowWindow( window_handle, show_type );

    HDC device_ctx = GetDC( window_handle );
    render_size = window_dimensions;
    win32_resize_dib_section( &bitmap_buffer, render_size.width, render_size.height );
    win32_init_xaudio2( window_handle );
    struct Win32GameCode game_code = win32_load_game_code( window_handle );
    struct PlatformCode platform_code = {
        .get_current_counter = win32_get_current_counter,
        .get_seconds_elapsed = win32_get_seconds_elapsed
    };
    struct ThreadContext thread_context = { 0 };

    // Pack screen buffer into a platform independent struct for game
    struct GameOffscreenBuffer offscreen_buffer = {
        .memory = bitmap_buffer.memory,
        .width = bitmap_buffer.width,
        .height = bitmap_buffer.height,
        .pitch = bitmap_buffer.pitch,
        .bytes_per_pixel = bitmap_buffer.bytes_per_pixel,
        .ratio_w = 16,
        .ratio_h = 9
    };

    struct GameMemory game_memory = { 0 };
    game_memory.permanent_memory_size = 1*MB;
    // This will make all pointer addresses consistent across all runs.
    // Great for debuggging!
#if DEBUG
    const LPVOID base_address = (LPVOID)( 1*TB );
#else
    const LPVOID base_address = 0;
#endif
    game_memory.permanent_memory = VirtualAlloc(
        base_address, game_memory.permanent_memory_size,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE
    );

    // Timing Initialization
    const int win32_refresh_rate = GetDeviceCaps( device_ctx, VREFRESH );
    const int monitor_refresh_Hz = win32_refresh_rate > 1 ? win32_refresh_rate : 60;
    const int game_update_Hz = monitor_refresh_Hz / 2;
    const float target_sec_per_frame = 1.0 / game_update_Hz;
    // const float target_sec_per_frame =  1.0 / 60.0;
    const UINT desired_scheduler_ms = 1;
    bool32 sleep_is_granular = (timeBeginPeriod( desired_scheduler_ms ) == TIMERR_NOERROR);
    printf(
        "Sleep is granular: %i | Monitor refresh rate (Hz): %i | Target ms/frame: %f \n",
        sleep_is_granular, monitor_refresh_Hz, (target_sec_per_frame * 1000)
    );
    LARGE_INTEGER perf_frequency; // COUNTS PER SECOND
    if ( !QueryPerformanceFrequency( &perf_frequency ) )
    {
        MessageBoxA( window_handle, "Cant QueryPerformanceFrequency", "Error!", MB_OK );
        exit( EXIT_FAILURE );
    }
    global_perf_frequency = perf_frequency.QuadPart;

    float delta_time = 0.0f;
    double total_elapsed_time = 0.0;
    i64 start_counter = win32_get_current_counter();
    i64 end_counter = win32_get_current_counter();
    u64 frame_count = 0;

    // TODO(Dolphin): Cleanup the loop
    while ( app_running )
    {
        delta_time = win32_get_seconds_elapsed( start_counter, win32_get_current_counter() );
        total_elapsed_time += delta_time;

        start_counter = win32_get_current_counter();
        // u64 last_cycle_count = ReadTimeStampCounter(); // __rdtsc

        MSG msg; // Process all messages
        while( PeekMessageW( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            if ( msg.message == WM_QUIT ) app_running = false;

            TranslateMessage( &msg );
            DispatchMessageW( &msg );
        }

        FILETIME game_dll_curr_write_time = win32_get_file_write_time( game_dll_name );
        if( CompareFileTime(
            &game_dll_curr_write_time, &game_code.last_write_time
        ) != 0 )
        {
            win32_unload_game_code( &game_code );
            game_code = win32_load_game_code( window_handle );
        }

        game_code.game_update_and_render(
            &thread_context, &platform_code, &game_memory, &input, &offscreen_buffer
        );

        win32_display_buffer_in_window( &bitmap_buffer, device_ctx );

        { // Timing of the frame
            end_counter = win32_get_current_counter();
            // u64 end_cycle_count = ReadTimeStampCounter(); // __rdtsc

            i64 counts_per_frame = end_counter - start_counter;
            i64 frames_per_second = global_perf_frequency / counts_per_frame;
            // u64 cycles_per_frame = end_cycle_count - last_cycle_count;
            // i64 miliseconds_per_frame = 1000 * counts_per_frame / global_perf_frequency;
            // i64 microseconds_per_frame = 1000000 * counts_per_frame / global_perf_frequency;
            // Can use 'frames_per_second * cycles_per_frame' to get processor GHz speed
            float seconds_elapsed_this_frame = win32_get_seconds_elapsed(
                start_counter, win32_get_current_counter()
            );

            // printf(
            //     "| elapsed: %f | delta: %f | ms/f: %f | FPS: %lli | \n", total_elapsed_time,
            //     delta_time, seconds_elapsed_this_frame * 1000, frames_per_second
            // );

            float sleep_duration_s = target_sec_per_frame - seconds_elapsed_this_frame;
            if ( sleep_duration_s > 0 )
            {
                if ( sleep_is_granular )
                {
                    Sleep( (DWORD)(sleep_duration_s * 1000) );
                }
                else // Sleep a fixed target time - 1 milisecond
                {
                    Sleep( (DWORD)(target_sec_per_frame * 1000) - 1 );
                }
            }

            if ( frame_count == 120 )
            {
                printf( "Performance FPS: %lli \n", frames_per_second );
                counts_per_frame = win32_get_current_counter() - start_counter;
                frames_per_second = global_perf_frequency / counts_per_frame;
                printf( "Real FPS: %lli \n", frames_per_second );
            }
        }

        frame_count += 1;
    }

    ReleaseDC( window_handle, device_ctx );
    printf("Exited successfully :) \n");
    return 0;
}