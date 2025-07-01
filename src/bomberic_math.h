#pragma once

#define PI 3.14159265359

// TODO(Dolphin): Replace math.h with custom, better implementations
#include <math.h>

float math_sinf( float x )
{
    float result = sinf( x );
    return result;
}