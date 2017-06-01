#pragma once
#include "cinder/CinderMath.h"

// utility functions to convert sound volume unit between dB and linear (0~1)
inline float linear2dB(float linear)
{
	return 20*log10(linear);
}

inline float dB2linear(float dB)
{
	return pow(10, dB / 20);
}