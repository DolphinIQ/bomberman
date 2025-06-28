#pragma once

// TODO(Dolphin): Replace math.h with custom, better implementations
#include <math.h>

#define PI 3.14159265359

float m_sinf( float x )
{
    float result = sinf( x );
    return result;
}