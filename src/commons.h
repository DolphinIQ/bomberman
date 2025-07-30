#pragma once

#include <stdint.h>
#include <stdio.h>

#define internal_fn     static
#define local_persist   static
#define global_var      static

#define false 0
#define true 1

#define KB (1024)
#define MB (1024 * 1024LLU)
#define GB (1024 * 1024 * 1024LLU)
#define TB (1024 * 1024 *  1024 * 1024LLU)

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

typedef u8 byte;
typedef u8 bool8;
typedef u32 bool32;

// #define i32 int32_t
// #define i64 int64_t
// #define bool8 u8
// #define bool32 u32

// #define UNUSED( x ) x = x
// void unused( int count, ... )
// {
//     va_list list;
//     va_start( list, count );
//     for ( int i = 0; i < count; i++ ) va_arg( list, int );
//     va_end( list );
// }

// For disabling compiler warnings
void unused( ... ) {}

// TODO(Dolphin): expand this once multithreaded is added
struct ThreadContext
{
    int unused;
};

// void assert( bool32 expression )
// {
//     if ( expression != true ) *(int*)0 = 100;
// }

#if DEBUG
    #define Assert( expression ) \
        if ( (expression) != true ) \
        { \
            printf( "Assert( %s ) failed in %s:%i at %s() \n", \
                    #expression, __FILE__, __LINE__, __FUNCTION__ ); \
            *(int*)0 = 100; \
        }
#else
    #define Assert( expression )
#endif