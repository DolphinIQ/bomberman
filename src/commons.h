#pragma once

#include <stdint.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
// code
#pragma GCC diagnostic pop

#define internal_fn     static
#define local_persist   static
#define global_var      static

#define false 0
#define true 1

#define KB (1024)
#define MB (1024 * 1024LLU)
#define GB (1024 * 1024 * 1024LLU)

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef u8 byte;
typedef u8 bool8;
typedef u32 bool32;

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