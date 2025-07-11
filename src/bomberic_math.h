#pragma once

#include <emmintrin.h>
#include <immintrin.h>

// TODO(Dolphin): Do some kind of a more professional check
#define AVX256_SUPPORTED 1

#define PI 3.14159265359

// TODO(Dolphin): Replace math.h with custom, better implementations
#include <math.h>

internal_fn inline float math_sinf( float value )
{
    float result = sinf( value );
    return result;
}

internal_fn inline int math_floor_f32_to_int( float value )
{
    int result = (int)floorf( value );
    return result;
}

internal_fn inline int math_round_f32_to_int( float value )
{
    int result = (int)roundf(value);
    return result;
}

// SIMD (non-macro functions are slower in debug, but in -O2 will have the same perf as macros)
// static inline __attribute__((always_inline))
internal_fn inline void simd_256i_store_zero( void* dst )
{
    _mm256_storeu_si256( (__m256i *)dst, _mm256_setzero_si256() );
}

internal_fn inline void simd_128i_store_zero( void* dst )
{
    // __m128i pixels_4 = _mm_set1_epi32( 0 );
    // _mm_storeu_si128( (__m128i*)dst, pixels_4 );
    _mm_storeu_si128( (__m128i *)dst, _mm_setzero_si128() );
}

/** 
#define simd_256i_store_zero( dst_ptr ) \
    _mm256_storeu_si256( (__m256i *)dst_ptr, _mm256_setzero_si256() )

#define simd_128i_store_zero( dst_ptr ) \
    _mm_storeu_si128( (__m128i *)dst_ptr, _mm_setzero_si128() )
*/
